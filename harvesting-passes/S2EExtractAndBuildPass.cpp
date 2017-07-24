/* vim: set expandtab ts=4 sw=4: */

/*
 * Copyright 2017 The bin2llvm Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "S2EExtractAndBuildPass.h"
#include "S2EPassUtils.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>

using namespace llvm;

char S2EExtractAndBuildPass::ID = 0;
static RegisterPass<S2EExtractAndBuildPass> X("s2eextract", "S2E Extraction Pass", false, false);

#ifdef TARGET_ARM
static const unsigned int pc_offset = 0xc0;
static const unsigned int lr_offset = 0xbc;
#elif defined(TARGET_I386)
static const unsigned int pc_offset = 12*4;
#endif

llvm::ConstantInt *S2EExtractAndBuildPass::isStoreToPC(StoreInst *store)
{
    // this is a design issue. We have to test if we have store to
    // before applying the transform pass.
#if defined(TARGET_ARM) || defined(TARGET_I386)
    IntToPtrInst *inttoptr = dyn_cast<IntToPtrInst>(store->getOperand(1));
    if (inttoptr) {
        Instruction *add = dyn_cast<Instruction>(inttoptr->getOperand(0));
        if (add) {
            if (add->getOperand(0)->getName().startswith("env_v")) {
                ConstantInt *idx = dyn_cast<ConstantInt>(add->getOperand(1));
                if (!idx)
                    return NULL;
                unsigned int envOffset = idx->getSExtValue();

                if (envOffset != pc_offset) {
                    // not PC
                    return NULL;
                }
                ConstantInt *val = dyn_cast<ConstantInt>(store->getValueOperand());
                if (val)
                    return val;
            }
        }
    }
#endif
    return NULL;
}

//#ifdef TARGET_ARM
llvm::ConstantInt *S2EExtractAndBuildPass::isStoreToLR(StoreInst *store)
{
    // this is a design issue. We have to test if we have store to
    // before applying the transform pass.
#ifdef TARGET_ARM
    IntToPtrInst *inttoptr = dyn_cast<IntToPtrInst>(store->getOperand(1));
    if (inttoptr) {
        Instruction *add = dyn_cast<Instruction>(inttoptr->getOperand(0));
        if (add) {
            if (add->getOperand(0)->getName().startswith("env_v")) {
                ConstantInt *idx = dyn_cast<ConstantInt>(add->getOperand(1));
                if (!idx)
                    return NULL;
                unsigned int envOffset = idx->getSExtValue();

                if (envOffset != lr_offset) {
                    // not PC
                    return NULL;
                }
                ConstantInt *val = dyn_cast<ConstantInt>(store->getValueOperand());
                if (val)
                    return val;
            }
        }
    }
#endif
    return NULL;
}
//#endif

bool S2EExtractAndBuildPass::isIndirectStoreToPC(StoreInst *store)
{
#if defined(TARGET_ARM) || defined(TARGET_I386)
    IntToPtrInst *inttoptr = dyn_cast<IntToPtrInst>(store->getOperand(1));
    if (inttoptr) {
        Instruction *add = dyn_cast<Instruction>(inttoptr->getOperand(0));
        if (add) {
            if (add->getOperand(0)->getName().startswith("env_v")) {
                ConstantInt *idx = dyn_cast<ConstantInt>(add->getOperand(1));
                if (!idx)
                    return false;
                unsigned int envOffset = idx->getSExtValue();

                if (envOffset != pc_offset) {
                    // not PC
                    return false;
                }
                ConstantInt *val = dyn_cast<ConstantInt>(store->getValueOperand());
                return val == NULL;
            }
        }
    }
#endif
    return false;
}


bool S2EExtractAndBuildPass::runOnFunction(Function &F) {
  if (!initialized)
    initialize(F);
  if (getBasicBlock("BB_" + intToString(currBlockAddr)))
      return false;
  targetMap.clear();
  implicitTarget = false;
  std::list<BasicBlock*> terminatorList;
  // Search for terminator blocks / PC use / nextBB use
  for (Function::iterator ibb = F.begin(), ibe = F.end(); ibb != ibe; ++ibb) {
    if (!isa<ReturnInst>(ibb->getTerminator()))
      continue;
    terminatorList.push_back(ibb);
    for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end(); iib != iie; ++iib) {
      StoreInst *storeInst = dyn_cast<StoreInst>(iib);
      if (!storeInst)
        continue;
      ConstantInt *constStoreToPC = isStoreToPC(storeInst);
      bool storeIndirectToPC = isIndirectStoreToPC(storeInst);
#ifdef TARGET_ARM
      ConstantInt *constStoreToLR = isStoreToLR(storeInst);
#endif
      if (constStoreToPC) {
          targetMap[storeInst->getParent()] = constStoreToPC->getZExtValue();
      }
      // indirect store to PC should not be part of the next target
      if (storeIndirectToPC) {
          targetMap.erase(storeInst->getParent());
      }

#ifdef TARGET_ARM
      if (constStoreToLR) {
          if (nextBlockAddr != (unsigned long)-1 &&
                  constStoreToLR &&
                  (constStoreToLR->getZExtValue() & -2) == (nextBlockAddr & -2)) {
              implicitTarget = true;
          }
      }
#endif
    }
  }
  // Find function to insert into
  Function *newF = findParentFunction();
  // Copy global references
  ValueToValueMapTy VMap;
  Argument *arg0 = F.getArgumentList().begin();
  Constant *constant = ConstantPointerNull::get((PointerType*)arg0->getType());
  VMap[arg0] = constant;
  copyGlobalReferences(F, VMap);
  // Inline code into new function
  SmallVector<ReturnInst *, 5> tempList;
  CloneFunctionInto(newF, &F, VMap, true, tempList);
  BasicBlock *firstBlock = dyn_cast<BasicBlock>(VMap[&(F.getEntryBlock())]);
  assert(firstBlock);
  firstBlock->setName("BB_" + intToString(currBlockAddr));
  basicBlocks["BB_" + intToString(currBlockAddr)] = firstBlock;

  // Create link from transition block
  BasicBlock *transBlock = getBasicBlock("Trans_" + intToString(currBlockAddr));
  if (transBlock) {
    /* delete the temporary return instruction */
    transBlock->getTerminator()->eraseFromParent();
    BranchInst::Create(firstBlock, transBlock);
  } else {
    BasicBlock *entryBlock = &(newF->getEntryBlock());
    if (entryBlock->getTerminator() != NULL) {
      errs() << "Unknown linkage of block " << *firstBlock << "\n";
    }
    BranchInst::Create(firstBlock, entryBlock);
  }
  moveBasicBlockToNewFunction(firstBlock, newF);

  // Conditional Branch
  if (terminatorList.size() < targetMap.size())  {
	  errs() << "ERROR: terminatorList.size = " <<  terminatorList.size() << ", targetMap.size = " << targetMap.size() << '\n';
  }
  if (terminatorList.size() > 1) {
    while (terminatorList.size() > 0) {
      BasicBlock *oldBlock = terminatorList.front();
      BasicBlock *newBlock = dyn_cast<BasicBlock>(VMap[oldBlock]);
      if (targetMap.count(oldBlock))
        createDirectEdge(newBlock, targetMap[oldBlock]);
      else {
        //errs() << "Conditional branch with unknown target " << *newBlock << "\n";
        createIndirectEdge(newBlock);
      }
      terminatorList.pop_front();
    }
  }
  // Jump
  else if (terminatorList.size() == 1 && !implicitTarget) {
    BasicBlock *oldBlock = terminatorList.front();
    BasicBlock *newBlock = dyn_cast<BasicBlock>(VMap[oldBlock]);
    if (targetMap.count(oldBlock))
      createDirectEdge(newBlock, targetMap[oldBlock]);
    else {
      createIndirectEdge(newBlock);
    }
    terminatorList.pop_front();
  }
  // Call
  else if (terminatorList.size() == 1 && implicitTarget) {
    BasicBlock *oldBlock = terminatorList.front();
    BasicBlock *newBlock = dyn_cast<BasicBlock>(VMap[oldBlock]);
    if (targetMap.count(oldBlock))
      createCallEdge(newBlock, targetMap[oldBlock], false);
    else {
      createIndirectCallEdge(newBlock);
    }
    terminatorList.pop_front();
  }
  return false;
}

void S2EExtractAndBuildPass::initialize(Function &F) {
  M = new Module("S2E-Module", F.getContext());
  genericFunctionType = FunctionType::get(llvm::Type::getInt1Ty(F.getContext()), false);
  initialized = true;
}

bool S2EExtractAndBuildPass::validateMove(BasicBlock *block)
  /* return true if we can move a BB */
{
  std::list<BasicBlock *> blockList;
  std::set<BasicBlock *> visitedStage1;

  blockList.push_back(block);
  while (blockList.size() > 0) {
    BasicBlock *head = blockList.front();
    blockList.pop_front();
    if (visitedStage1.count(head)) {
      continue;
    }
    visitedStage1.insert(head);
    for (unsigned int i = 0; i < head->getTerminator()->getNumSuccessors(); ++i) {
      blockList.push_back(head->getTerminator()->getSuccessor(i));
    }
  }

  for (std::set<BasicBlock *>::iterator itr = visitedStage1.begin();
      itr != visitedStage1.end();
      ++itr) {
    BasicBlock *bb = *itr;
    for (Value::use_iterator iub = bb->use_begin(), iue = bb->use_end();
        iub != iue;
        ++iub) {
      TerminatorInst *terminator = dyn_cast<TerminatorInst>(*iub);
      assert(terminator);
      if (!visitedStage1.count(terminator->getParent()))
        return false;
    }
  }
  return true;
}

void S2EExtractAndBuildPass::processPCUse(Value *pcUse) {
  Instruction *useInstruction = dyn_cast<Instruction>(pcUse);
  if (!useInstruction) {
    errs() << "Undefined use of PC: " << *pcUse << "\n";
    return;
  }

  // Store instruction uses PC
  // Sink for data-flow analysis
  StoreInst *storeInst = dyn_cast<StoreInst>(useInstruction);
  if (storeInst) {
    ConstantInt *constPC = dyn_cast<ConstantInt>(storeInst->getValueOperand());
    // If store happens outside of return block, skip it
    if (!isa<ReturnInst>(storeInst->getParent()->getTerminator())) {
      return;
    }
    if (!constPC) {
      //errs() << "Non-constant PC change: " << *useInstruction << "\n";
      targetMap.erase(storeInst->getParent());
      return;
    }

    targetMap[storeInst->getParent()] = constPC->getSExtValue();
    return;
  }
  // Load instruction uses PC
  // Sink for data-flow analysis
  LoadInst *loadInst = dyn_cast<LoadInst>(useInstruction);
  if (loadInst) {
    return;
  }
  // Unsupported instruction
  errs() << "Unsupported instruction: " << *useInstruction << "\n";
  return;
}

bool S2EExtractAndBuildPass::searchNextBasicBlockAddr(Function &F)
{
    if (nextBlockAddr == (unsigned long)-1)
        return false;

#ifdef TARGET_ARM
    for (Function::iterator ibb = F.begin(), ibe = F.end(); ibb != ibe; ++ibb) {
        for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end(); iib != iie; ++iib) {
            StoreInst *storeInst = dyn_cast<StoreInst>(iib);
            if (storeInst) {
                ConstantInt *constStoreToLR = isStoreToLR(storeInst);
                if (constStoreToLR && constStoreToLR->getZExtValue() == nextBlockAddr) {
                    implicitTarget = true;
                    return true;
                }
            }
        }
    }
#endif
    return false;
}

Function *S2EExtractAndBuildPass::findParentFunction() {
  Function *foundFunction;
  foundFunction = getFunction("Function_" + intToString(currBlockAddr));
  if (foundFunction)
    return foundFunction;
  BasicBlock *transBlock = getBasicBlock("Trans_" + intToString(currBlockAddr));
  if (transBlock != NULL) {
    return transBlock->getParent();
  }
  return getOrCreateFunction("Function_" + intToString(currBlockAddr));
}

void S2EExtractAndBuildPass::copyGlobalReferences(Function &F, ValueToValueMapTy &VMap) {
  for (Function::iterator ibb = F.begin(), ibe = F.end(); ibb != ibe; ++ibb) {
    for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end(); iib != iie; ++iib)
      for (User::op_iterator iob = iib->op_begin(), ioe = iib->op_end(); iob != ioe; ++iob) {
        GlobalVariable *globalVar = dyn_cast<GlobalVariable>(*iob);
        if (globalVar) {
          Constant *newGlobal = M->getOrInsertGlobal(globalVar->getName(), globalVar->getType()->getElementType());
          VMap[globalVar] = newGlobal;
        }
        Function *globalFun = dyn_cast<Function>(*iob);
        if (globalFun) {
          Constant *newGlobal = M->getOrInsertFunction(globalFun->getName(), globalFun->getFunctionType());
          VMap[globalFun] = newGlobal;
        }
      }
  }
}

void S2EExtractAndBuildPass::createDirectEdge(BasicBlock *block, unsigned long target) {
  // Branch into different function entails tail-call
  Function *head = getFunction("Function_" + intToString(target));
  if (head)  {
    createCallEdge(block, target, true);
    return;
  }
  // Branch into different function entails tail-call
  BasicBlock *dblock = getBasicBlock("BB_" + intToString(target));
  if (dblock && dblock->getParent() != block->getParent()) {
      createCallEdge(block, target, true);
      return;
  }
  dblock = getBasicBlock("Trans_" + intToString(target));
  if (dblock && dblock->getParent() != block->getParent()) {
      createCallEdge(block, target, true);
      return;
  }

  TerminatorInst *terminator = block->getTerminator();
  BasicBlock *explicitTarget = getOrCreateTransBasicBlock(block->getParent(), target);
  BranchInst::Create(explicitTarget, terminator);
  terminator->eraseFromParent();
}

void S2EExtractAndBuildPass::createIndirectEdge(BasicBlock *block) {
  TerminatorInst *terminator = block->getTerminator();
  ReturnInst::Create(M->getContext(), ConstantInt::getFalse(M->getContext()), terminator);
  terminator->eraseFromParent();
}

void S2EExtractAndBuildPass::createCallEdge(BasicBlock *block, unsigned long target, bool isTailCall) {
  TerminatorInst *terminator = block->getTerminator();
  Function *targetFunction = getOrCreateFunction("Function_" + intToString(target));
  CallInst::Create(targetFunction, "", terminator);
  if (!isTailCall) {
    if (NULL == getFunction("Function_" + intToString(nextBlockAddr))) {
      BasicBlock *implicitTarget = getOrCreateTransBasicBlock(block->getParent(), nextBlockAddr);
      assert(implicitTarget);
      BranchInst::Create(implicitTarget, terminator);
    } else {
      createCallEdge(block, nextBlockAddr, true);
      return;
    }
  } else {
    /* we have a tail call */
    ReturnInst::Create(M->getContext(), ConstantInt::getFalse(M->getContext()), terminator);
  }
  terminator->eraseFromParent();
}

BasicBlock *S2EExtractAndBuildPass::getOrCreateTransBasicBlock(Function *parent, unsigned long addr)
{
  BasicBlock *ret = getOrCreateBasicBlock(parent, "Trans_" + intToString(addr));
  assert(NULL == getFunction("Function_" + intToString(addr)));
  if (ret->getTerminator() == NULL) {
    /* basic block is invalid, we should insert a return inst */
    ReturnInst::Create(M->getContext(), ConstantInt::getFalse(M->getContext()), ret);
  }
  return ret;
}

void S2EExtractAndBuildPass::createIndirectCallEdge(BasicBlock *block) {
  TerminatorInst *terminator = block->getTerminator();
  Value *pc = M->getOrInsertGlobal("PC", IntegerType::get(M->getContext(), 32));
  LoadInst *loadPC = new LoadInst(pc, "", terminator);
  llvm::Type* genericFunctionPtrTy = PointerType::get(genericFunctionType, 0);
  CastInst *targetFunction = new IntToPtrInst(loadPC, genericFunctionPtrTy, "", terminator);
  CallInst::Create(targetFunction, "", terminator);

  if (NULL == getFunction("Function_" + intToString(nextBlockAddr))) {
    BasicBlock *implicitTarget = getOrCreateTransBasicBlock(block->getParent(), nextBlockAddr);
    assert(implicitTarget);
    BranchInst::Create(implicitTarget, terminator);
    terminator->eraseFromParent();
  } else {
    createCallEdge(block, nextBlockAddr, true);
  }
}

BasicBlock* S2EExtractAndBuildPass::getBasicBlock(std::string name) {
  bbmap_t::iterator foundBB = basicBlocks.find(name);
  if (foundBB != basicBlocks.end())
    return foundBB->second;
  return NULL;
}

BasicBlock* S2EExtractAndBuildPass::getOrCreateBasicBlock(Function *parent, std::string name) {
  bbmap_t::iterator foundBB = basicBlocks.find(name);
  if (foundBB != basicBlocks.end())
    return foundBB->second;
  BasicBlock *newBB = BasicBlock::Create(M->getContext(), name, parent);
  basicBlocks[name] = newBB;
  return newBB;
}

Function* S2EExtractAndBuildPass::getFunction(std::string name) {
  fmap_t::iterator foundF = functions.find(name);
  if (foundF != functions.end())
    return foundF->second;
  return NULL;
}

Function* S2EExtractAndBuildPass::getOrCreateFunction(std::string name) {
  fmap_t::iterator foundF = functions.find(name);
  if (foundF != functions.end())
    return foundF->second;
  Function *newF = Function::Create(genericFunctionType, GlobalValue::ExternalLinkage, name, M);
  functions[name] = newF;
  BasicBlock::Create(M->getContext(), "entry", newF);
  assert(name.find("Function_") != std::string::npos);

  std::string addr = name.substr(std::strlen("Function_"));
  BasicBlock *head = getBasicBlock("BB_" + addr);
  if (head) {
    /* cleaning unused Trans_ first */
    if (!validateMove(head)) {
      /* jump in middle of func? */
      functions.erase(name);
      functions["FakeFunction_" + addr] = newF;
      ReturnInst::Create(M->getContext(),
          ConstantInt::getFalse(M->getContext()), &newF->getEntryBlock());
      newF->setName("FakeFunction_" + addr);
    } else {
      std::list<Instruction *> terminatorList;
      relinkOldTransBasicBlock(newF, addr, terminatorList);
      moveBasicBlocksToNewFunction(head, newF);

      while (terminatorList.size()) {
        terminatorList.front()->eraseFromParent();
        terminatorList.pop_front();
      }

      BasicBlock *transBlock = getBasicBlock("Trans_" + addr);
      assert(transBlock);
      basicBlocks.erase(transBlock->getName());
      head->removePredecessor(transBlock);
      transBlock->eraseFromParent();
    }
  } else {
    // if trans was created but BB was not yet explored
    std::list<Instruction *> terminatorList;
    BasicBlock *transBlock = getBasicBlock("Trans_" + addr);
    if (transBlock) {
      relinkOldTransBasicBlock(newF, addr, terminatorList);

      while (terminatorList.size()) {
        terminatorList.front()->eraseFromParent();
        terminatorList.pop_front();
      }

      basicBlocks.erase(transBlock->getName());
      transBlock->eraseFromParent();
    }
  }

  return newF;
}

void S2EExtractAndBuildPass::relinkOldTransBasicBlock(
    Function *newF,
    std::string addr,
    std::list<llvm::Instruction *> &terminatorList)
{
    BasicBlock *transBlock = getBasicBlock("Trans_" + addr);
    assert(transBlock);

    for (Value::use_iterator iub = transBlock->use_begin(), iue = transBlock->use_end(); iub != iue; ++iub) {
      TerminatorInst *terminator = dyn_cast<TerminatorInst>(*iub);
      if (!terminator) {
        errs() << "Invalid use of basic-block " << *iub << "\n";
        continue;
      }

      CallInst::Create(newF, "", terminator);
      ReturnInst::Create(M->getContext(), ConstantInt::getFalse(M->getContext()), terminator);
      terminatorList.push_back(terminator);
    }
}

void S2EExtractAndBuildPass::moveBasicBlockToNewFunction(BasicBlock *block, Function *parent) {
  BasicBlock *transBlock;
  // Intermediate BBs are simple to move
  if (block->getName().find("BB_") == std::string::npos) {
    block->moveAfter(&(parent->back()));
  }
  // BBs with no transition edge are just moved after entry
  else if ((transBlock = block->getSinglePredecessor()) == NULL) {
    BasicBlock *entryBlock = &(parent->getEntryBlock());
    BranchInst::Create(block, entryBlock);
    block->moveAfter(entryBlock);
  }
  // BBs with transition edges in the same parent are simple to move
  else if (transBlock->getParent() == parent) {
    block->moveAfter(transBlock);
  }
  // BB changed from jump to call
  else {
    BasicBlock *entryBlock = &(parent->getEntryBlock());
    BranchInst::Create(block, entryBlock);
    block->moveAfter(entryBlock);
  }
}


void S2EExtractAndBuildPass::moveBasicBlocksToNewFunction(BasicBlock *block, Function *parent) {
  //errs() << "[llvm-tran]: Moving basic blocks: " << *block << "\n";
  /* if bb->getName() == "" => anonymous basic block, not a problem */
  if (block->getParent() == parent) {
    /* we don't want to move again an already processed bb */
    return;
  }
  moveBasicBlockToNewFunction(block, parent);
  std::list<BasicBlock *> blockList;
  for (unsigned int i = 0; i < block->getTerminator()->getNumSuccessors(); ++i) {
    blockList.push_back(block->getTerminator()->getSuccessor(i));
  }
  while(blockList.size()) {
    moveBasicBlocksToNewFunction(blockList.front(), parent);
    blockList.pop_front();
  }
}
