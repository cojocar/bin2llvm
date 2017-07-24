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

#include "S2ETransformPass.h"

#include "llvm/DataLayout.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>

using namespace llvm;

char S2ETransformPass::ID = 0;
static RegisterPass<S2ETransformPass> X("s2etransform",
        "S2E Transformation Pass. Rename env_* access to accesses to\
        global variables named as the registers for this arch", false,
        false);

static int64_t getGetElementPtrOffset(GetElementPtrInst* gep);

/* this pass transforms the loads and stores via env_ pointer to
 * meaningful global register names
 */

// This pass should run after constant propagation
//
// Stores:
//  %env_v24 = load i64* null
//  %44 = add i64 %env_v24, 192
//  %45 = inttoptr i64 %44 to i32*
//  store i32 16, i32* %45
//
//
// Loads:
//  %0 = add i64 %env_v1, 125144
//  %1 = inttoptr i64 %0 to i64*
//  %tmp6_v = load i64* %1
//
// The goal of this pass is to replace access to env with access to
// global registers. Basically, it is a array identify

bool S2ETransformPass::runOnFunction(Function &F)
{
    /* this is the env use from inlined code */
    /*
    fixupEnvStruct(F);
    fixupStores(F);
    fixupLoads(F);
    */
  M = F.getParent();
  Argument *arg0 = F.getArgumentList().begin();
  bool recursiveSuccess = true;
  for (Value::use_iterator iub = arg0->use_begin(), iue = arg0->use_end(); iub != iue; ++iub) {
      recursiveSuccess &= processArgUse(*iub);
  }

  for (Function::iterator bb = F.begin(), bb_end = F.end(); bb != bb_end; bb++)  {
	  for (BasicBlock::iterator inst = bb->begin(), inst_end = bb->end(); inst != inst_end; inst++)  {
		  if (inst->getOpcode() == Instruction::Load &&
				  dyn_cast<LoadInst>(inst)->getPointerOperand()->getName() == "env")
		  {
			  recursiveSuccess &= processArgUse(inst);
		  }
	  }
  }

  Value *globalEnv = M->getOrInsertGlobal("CPUSTATE", arg0->getType());
  BasicBlock &entry = F.getEntryBlock();
  LoadInst *loadEnv = new LoadInst(globalEnv, "", entry.getFirstInsertionPt());
  arg0->replaceAllUsesWith(loadEnv);
  cleanup();
  if (recursiveSuccess)  {
    loadEnv->eraseFromParent();
  }
  return false;
    return true;
}

int
S2ETransformPass::fixupEnvStruct(Function &F)
{
    std::list<LoadInst *> eraseLoads;
    /* search for loads */
    for (Function::iterator bb = F.begin(), bb_end = F.end();
            bb != bb_end;
            bb++)  {
        for (BasicBlock::iterator inst = bb->begin(), inst_end = bb->end();
                inst != inst_end;
                inst++)  {
            LoadInst *loadInst = dyn_cast<LoadInst>(inst);
            if (!loadInst)
                continue;
            if (loadInst->getPointerOperand()->getName() != "env")
                continue;
            outs() << "[S2ETransformPass] found env: " << *loadInst << "\n";
            eraseLoads.push_back(loadInst);
        }
    }

    for (Function::iterator bb = F.begin(), bb_end = F.end();
            bb != bb_end;
            bb++)  {
        for (BasicBlock::iterator inst = bb->begin(), inst_end = bb->end();
                inst != inst_end;
                inst++)  {
            GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(inst);
            if (!gepInst)
                continue;
            if (gepInst->getNumIndices() != 2)
                continue;
            LoadInst *loadInst = dyn_cast<LoadInst>(gepInst->getPointerOperand());
            if (!loadInst)
                continue;
            if (!loadInst->getPointerOperand())
                continue;
            if (loadInst->getPointerOperand()->getName() != "env")
                continue;
            GetElementPtrInst::op_iterator op = gepInst->idx_begin();
            ConstantInt *firstIndex = dyn_cast<ConstantInt>(*op);
            ++op;
            ConstantInt *secondIndex = dyn_cast<ConstantInt>(*op);
            if (!firstIndex || !secondIndex ||
                    firstIndex->getZExtValue() != 0) {
                errs() <<
                    "[S2ETransformPass] I do not how to deal with " <<
                    *gepInst << "\n";
                continue;
            }
            uint64_t offset = getGetElementPtrOffset(gepInst);
            outs() << "[S2ETransformPass] found GEPInst: " << *gepInst
                << " " << firstIndex->getZExtValue()
                << " " << secondIndex->getZExtValue()
                << " " << offset << "\n";

            for (Value::use_iterator iub = gepInst->use_begin(), iue = gepInst->use_end();
                    iub != iue;
                    ++iub) {
                outs() << "[S2ETransformPass] \tused in" << *(*iub) << "\n";
            }
        }
    }

    /*
    Value *globalEnv = M->getOrInsertGlobal("CPUSTATE", arg0->getType());
    BasicBlock &entry = F.getEntryBlock();
    LoadInst *loadEnv = new LoadInst(globalEnv, "", entry.getFirstInsertionPt());
    arg0->replaceAllUsesWith(loadEnv);
    cleanup();
    if (recursiveSuccess)  {
        loadEnv->eraseFromParent();
    }
    */
    return 0;
}

// Return the number of fixup stores that were performed
int S2ETransformPass::fixupStores(Function &F)
{
    M = F.getParent();
    int cnt = 0;
    for (Function::iterator bb = F.begin(), bb_end = F.end(); bb != bb_end; bb++)  {
        for (BasicBlock::iterator inst = bb->begin(), inst_end = bb->end(); inst != inst_end; inst++)  {
            if (inst->getOpcode() == Instruction::Store) {
                StoreInst *store = dyn_cast<StoreInst>(inst);
                IntToPtrInst *inttoptr = dyn_cast<IntToPtrInst>(store->getOperand(1));
                if (inttoptr) {
                    Instruction *add = dyn_cast<Instruction>(inttoptr->getOperand(0));
                    if (add) {
                        if (add->getOperand(0)->getName().startswith("env_v")) {
                            eraseStoreList.push_back(store);
                            ConstantInt *cst = dyn_cast<ConstantInt>(add->getOperand(1));
                            if (!cst) {
                                errs() << "The index is a variable\n";
                            }
                            unsigned int envOffset = cst->getSExtValue();
                            //errs() << "store: " << add->getOperand(0)->getName() << " "
                            //    << envOffset << " " << getGlobalName(envOffset) << "\n";

                            Value *global =
                                M->getOrInsertGlobal(getGlobalName(envOffset),
                                        store->getValueOperand()->getType());
                            StoreInst *newStore = new StoreInst(store->getValueOperand(),
                                    global, store);
                            replaceMap.insert(std::make_pair(store, newStore));
                        }
                    }
                }
            }
        }
    }

    // cleanup
    while (replaceMap.begin() != replaceMap.end()) {
        replaceMap.begin()->first->replaceAllUsesWith(replaceMap.begin()->second);
        replaceMap.erase(replaceMap.begin()->first);
        ++cnt;
    }

    while (eraseStoreList.begin() != eraseStoreList.end()) {
        (*eraseStoreList.begin())->eraseFromParent();
        eraseStoreList.pop_front();
    }

    return cnt;
}

int S2ETransformPass::fixupLoads(Function &F)
{
    M = F.getParent();
    int cnt = 0;
    for (Function::iterator bb = F.begin(), bb_end = F.end(); bb != bb_end; bb++)  {
        for (BasicBlock::iterator inst = bb->begin(), inst_end = bb->end(); inst != inst_end; inst++)  {
            if (inst->getOpcode() == Instruction::Load) {
                LoadInst *load = dyn_cast<LoadInst>(inst);
                IntToPtrInst *inttoptr = dyn_cast<IntToPtrInst>(load->getOperand(0));
                if (inttoptr) {
                    Instruction *add = dyn_cast<Instruction>(inttoptr->getOperand(0));
                    if (add) {
                        if (add->getOperand(0)->getName().startswith("env_v")) {
                            ConstantInt *cst = dyn_cast<ConstantInt>(add->getOperand(1));
                            if (!cst) {
                                errs() << "The index is a variable\n";
                            }
                            unsigned int envOffset = cst->getSExtValue();
                            //errs() << "load: " << add->getOperand(0)->getName() << " "
                            //    << envOffset << " " << getGlobalName(envOffset) << "\n";

                            Value *global =
                                M->getOrInsertGlobal(getGlobalName(envOffset),
                                        load->getType());
                            LoadInst *newLoad = new LoadInst (global, "", load);

                            replaceMap.insert(std::make_pair(load, newLoad));
                            eraseLoadList.push_back(load);
                        }
                    }
                }
            }
        }
    }

    // cleanup
    while (replaceMap.begin() != replaceMap.end()) {
        replaceMap.begin()->first->replaceAllUsesWith(replaceMap.begin()->second);
        replaceMap.erase(replaceMap.begin()->first);
        ++cnt;
    }

    while (eraseLoadList.begin() != eraseLoadList.end()) {
        (*eraseLoadList.begin())->eraseFromParent();
        eraseLoadList.pop_front();
    }

    return cnt;
}

bool S2ETransformPass::processArgUse(Value *argUse)
{
    bool recursiveSuccess = true;
    if (!argUse) {
        errs() << "arg use is null" << argUse << "\n";
    }
    //errs() << "processing: " << argUse << "\n";
    Instruction *useInstruction = dyn_cast<Instruction>(argUse);
    if (!useInstruction) {
        errs() << "Undefined use of argument: " << *argUse << "\n";
        return false;
    }

    // Cast instructions just copy value
    // Propagate data-flow analysis
    if (useInstruction->isCast()) {
        for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
                iub != iue; ++iub) {
            recursiveSuccess &= processArgUse(*iub);
        }
        if (recursiveSuccess)
            eraseList.push_back(useInstruction);
        return recursiveSuccess;
    }
    // GetElementPtr instructions can copy value
    // Propagate data-flow analysis with new offset
    GetElementPtrInst *GEPInst = dyn_cast<GetElementPtrInst>(useInstruction);
    if (GEPInst) {
        ConstantInt *constOffset = dyn_cast<ConstantInt>(*(GEPInst->idx_begin()));
        if (GEPInst->getNumIndices() != 1 || !constOffset || constOffset->getSExtValue() != 0) {
            errs() << "Unsupported GEP for argument pointer: " << *useInstruction << "\n";
            return false;
        } 
        for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
                iub != iue; ++iub) {
            recursiveSuccess &= processArgUse(*iub);
        }
        if (recursiveSuccess)
            eraseList.push_back(useInstruction);
        return recursiveSuccess;
    }
    // Load instruction uses argument pointer
    // Sink for data-flow analysis
    LoadInst *loadInst = dyn_cast<LoadInst>(useInstruction);
    if (loadInst) {
        for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
                iub != iue; ++iub) {
            recursiveSuccess &= processEnvUse(useInstruction, *iub, 0);
        }
        if (recursiveSuccess)
            eraseList.push_back(useInstruction);
        return recursiveSuccess;
    }
    // Unsupported instruction
    //errs() << "Unsupported instruction: " << *useInstruction << "\n";
    errs() << "Unsupported instruction: " << "\n";
    return false;
}

/**
 * Offset calculation analoguous to EmitGEPOffset in Local.h.
 * Returns offset of structure (and -1 on error).
 */
static int64_t getGetElementPtrOffset(GetElementPtrInst* gep)
{
    assert(gep);

    int elementOffset = 0;

    DataLayout TD(gep->getParent()->getParent()->getParent());

    gep_type_iterator gepTypeItr = gep_type_begin(gep);
    for (GetElementPtrInst::op_iterator i =  gep->op_begin() + 1, op_end = gep->op_end(); i != op_end; i++, gepTypeItr++)
    {
        Value* op = *i;
        if (Constant *opC = dyn_cast<Constant>(op))
        {
            if (opC->isNullValue())
                continue;

            if (StructType * sTy = dyn_cast<StructType>(*gepTypeItr))  {
                if (ConstantVector* opV = dyn_cast<ConstantVector>(opC))
                    opC = opV->getSplatValue();

                uint64_t opV = dyn_cast<ConstantInt>(opC)->getZExtValue();
                elementOffset += TD.getStructLayout(sTy)->getElementOffset(opV);
            }
            else {
                uint64_t scale = TD.getTypeAllocSize(gepTypeItr.getIndexedType());
                int64_t opV = dyn_cast<ConstantInt>(opC)->getSExtValue();
                elementOffset += scale * opV;
            }
        }
        else  {
            errs() << "Increase by non-constant GEP offset for environment pointer: " << *gep
                << "in basic block " << gep->getParent()->getName()
                <<", function " << gep->getParent()->getParent()->getName() << "\n";
            return -1;
        }
    }

    return elementOffset;
}

bool S2ETransformPass::processEnvUse(Value *envPointer, Value *envUse, unsigned int currentOffset) {
  bool recursiveSuccess = true;
  Instruction *useInstruction = dyn_cast<Instruction>(envUse);
  if (!useInstruction) {
    errs() << "Undefined use of environment pointer: " << *envUse << "\n";
    return false;
  }

  // Cast instructions just copy value
  // Propagate data-flow analysis
  if (useInstruction->isCast()) {
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processEnvUse(envUse, *iub, currentOffset);
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // Add instructions increase offset
  // Propagate data-flow analysis with new offset
  if (useInstruction->isBinaryOp() && useInstruction->getOpcode() == Instruction::Add) {
    ConstantInt *constOffset = dyn_cast<ConstantInt>(useInstruction->getOperand(1));
    if (!constOffset) {
      errs() << "Increase by non-constant offset for environment pointer: " << *useInstruction << "\n";
      return false;
    } 
    for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end();
        iub != iue; ++iub) {
      recursiveSuccess &= processEnvUse(envUse, *iub, currentOffset + constOffset->getSExtValue());
    }
    if (recursiveSuccess)
      eraseList.push_back(useInstruction);
    return recursiveSuccess;
  }
  // GetElementPtr instructions increase offset
  // Propagate data-flow analysis with new offset
  if (GetElementPtrInst* GEPInst = dyn_cast<GetElementPtrInst>(useInstruction)) {
	  uint64_t offset = getGetElementPtrOffset(GEPInst);

	  if (offset == -1)  {
		  errs() << "Increase by non-constant GEP offset for environment pointer: " << *useInstruction << "\n";
		  return false;
	  }


	  for (Value::use_iterator iub = useInstruction->use_begin(), iue = useInstruction->use_end(); iub != iue; ++iub) {
		  recursiveSuccess &= processEnvUse(envUse, *iub, currentOffset + offset);
	  }
	  if (recursiveSuccess)  {
		  eraseList.push_back(useInstruction);
	  }
	  return recursiveSuccess;
  }
  // Load instruction uses environment pointer
  // Sink for data-flow analysis
  LoadInst *loadInst = dyn_cast<LoadInst>(useInstruction);
  if (loadInst) {
    Value *global = M->getOrInsertGlobal(getGlobalName(currentOffset), loadInst->getType());
    LoadInst *newLoad = new LoadInst (global, "", loadInst);
    replaceMap.insert(std::make_pair(loadInst, newLoad));
    eraseList.push_back(loadInst);
    return true;
  }
  // Store instruction uses environment pointer
  // Sink for data-flow analysis
  StoreInst *storeInst = dyn_cast<StoreInst>(useInstruction);
  if (storeInst) {
    if (storeInst->getPointerOperand() != envPointer) {
      errs() << "Environment pointer stored in memory: " << *useInstruction << "\n";
      return false;
    }
    Value *global = M->getOrInsertGlobal(getGlobalName(currentOffset), storeInst->getValueOperand()->getType());
    StoreInst *newStore = new StoreInst (storeInst->getValueOperand(), global, storeInst);
    replaceMap.insert(std::make_pair(storeInst, newStore));
    eraseList.push_back(storeInst);
    return true;
  }
  // Call instruction uses environment pointer
  // Sink for data-flow analysis
  CallInst *callInst = dyn_cast<CallInst>(useInstruction);
  if (callInst) {
    return false;
  }
  // Unsupported instruction
  errs() << "Unsupported instruction: " << *useInstruction << "\n";
  return false;
}

void S2ETransformPass::setRegisterName(
        unsigned int offset, std::string name)
{
  nameMap.insert(make_pair(offset, name));
}

#ifdef TARGET_I386
static struct S2ETransformPass::offset_name cpu_offset_names_x86[] = {
    {0 * 4, "R_EAX"},
    {1 * 4, "R_ECX"},
    {2 * 4, "R_EDX"},
    {3 * 4, "R_EBX"},
    {4 * 4, "R_ESP"},
    {5 * 4, "R_EBP"},
    {6 * 4, "R_ESI"},
    {7 * 4, "R_EDI"},
    {12 * 4, "PC"},
};
#endif

#ifdef TARGET_ARM
static struct S2ETransformPass::offset_name cpu_offset_names_arm[] = {
    {0x000000cc, "thumb"},
    {0x00000080, "ZF"},
    {0x00000074, "CF"},
    {0x00000078, "VF"},
    {0x0000007c, "NF"},
    {0x000000c0, "PC"},
    {0x000000bc, "LR"},
    {0x000000b8, "SP"},
    {0x000000b4, "R12"},
    {0x000000b0, "R11"},
    {0x000000ac, "R10"},
    {0x000000a8, "R9"},
    {0x000000a4, "R8"},
    {0x000000a0, "R7"},
    {0x0000009c, "R6"},
    {0x00000098, "R5"},
    {0x00000094, "R4"},
    {0x00000090, "R3"},
    {0x0000008c, "R2"},
    {0x00000088, "R1"},
    {0x00000084, "R0"},
    {0x0001e8d8, "s2e_icount"},
    {0x000004a8, "current_tb"},
    {0x000004b0, "s2e_icount_before_tb"},
};
#endif

void S2ETransformPass::initRegisterNames()
{
#if defined(TARGET_ARM) || defined(TARGET_ARMBE)
    initRegisterNames(&cpu_offset_names_arm[0],
            sizeof (cpu_offset_names_arm)/sizeof (cpu_offset_names_arm[0]));
#elif defined(TARGET_I386)
    initRegisterNames(&cpu_offset_names_x86[0],
            sizeof (cpu_offset_names_x86)/sizeof (cpu_offset_names_x86[0]));
#elif defined(TARGET_X86_64)
    /* TODO */
#else
#warning No register names for this ARCH TARGET_ARCH
#endif
}

void S2ETransformPass::initRegisterNames(
        struct S2ETransformPass::offset_name *regs, size_t count)
{
    int i;

    for (i = 0; i < count; ++i)
        setRegisterName(regs[i].offset, regs[i].name);
}

void S2ETransformPass::cleanup() {
  while(replaceMap.begin() != replaceMap.end()) {
    replaceMap.begin()->first->replaceAllUsesWith(replaceMap.begin()->second);
    replaceMap.erase(replaceMap.begin()->first);
  }
  while(eraseList.begin() != eraseList.end()) {
    (*eraseList.begin())->eraseFromParent();
    eraseList.pop_front();
  }
}

std::string S2ETransformPass::getGlobalName(unsigned int offset)
{
    if (nameMap.count(offset))
        return nameMap[offset];
    std::stringstream ss;
    ss << std::hex << offset;
    return "GLOBAL_@" + ss.str();
}
