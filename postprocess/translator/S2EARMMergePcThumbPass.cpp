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

#include "S2EARMMergePcThumbPass.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

char S2EArmMergePcThumbPass::ID = 0;
static RegisterPass<S2EArmMergePcThumbPass> X("s2earmergepcthumb", "S2E delete stupid qemu stores and helper function calls", false, false);

bool S2EArmMergePcThumbPass::runOnFunction(Function &f) {
//  M = bb.getParent();
  std::list<llvm::Instruction*> eraseList;

  LLVMContext& ctx = f.getContext();

  for (Function::iterator bb = f.begin(); bb != f.end(); bb++)
  {

      for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); inst++)
      {
    	  Instruction* nextInstruction = inst->getNextNode();

    	  if (nextInstruction == bb->end()) {
    		  break;
    	  }

		  StoreInst* pcStore = dyn_cast<StoreInst>(inst);
		  StoreInst* thumbStore = dyn_cast<StoreInst>(nextInstruction);

		  if (!pcStore ||
		      !thumbStore ||
		      !dyn_cast<GlobalVariable>(pcStore->getPointerOperand()) ||
			  !dyn_cast<GlobalVariable>(thumbStore->getPointerOperand()))  {
			  continue;
		  }

		  if (dyn_cast<GlobalVariable>(pcStore->getPointerOperand())->getName() == "thumb" &&
			  dyn_cast<GlobalVariable>(thumbStore->getPointerOperand())->getName() == "PC")
		  {
			  StoreInst* tmp = pcStore;
			  pcStore = thumbStore;
			  thumbStore = tmp;
		  }

		  if (dyn_cast<GlobalVariable>(pcStore->getPointerOperand())->getName() == "PC" &&
			  dyn_cast<GlobalVariable>(thumbStore->getPointerOperand())->getName() == "thumb" &&
			  dyn_cast<ConstantInt>(pcStore->getValueOperand()) &&
			  dyn_cast<ConstantInt>(thumbStore->getValueOperand()))
		  {
			  bool thumb = dyn_cast<ConstantInt>(thumbStore->getValueOperand())->getZExtValue();
			  uint64_t pc = dyn_cast<ConstantInt>(pcStore->getValueOperand())->getZExtValue();

			  assert(thumbStore->getNumUses() == 0);

			  if (thumb)  {
				  pcStore->setMetadata("trans.next_instruction_set", MDNode::get(ctx, MDString::get(ctx, "thumb")));
				  m_instructionSet[pc & 0xfffffffeul] = THUMB;
			  }
			  else  {
				  pcStore->setMetadata("trans.next_instruction_set", MDNode::get(ctx, MDString::get(ctx, "arm")));
				  m_instructionSet[pc & 0xfffffffeul] = ARM;
			  }

//			  pcStore->setOperand(0, ConstantInt::get(llvm::Type::getInt32Ty(bb->getContext()), pc | thumb));
			  eraseList.push_back(thumbStore);
		  }
	  }

      //Add new instructions that replace removed ones
//	  for (std::list< std::pair< llvm::Instruction*, llvm::Instruction*> >::iterator itr = insertList.begin(); itr != insertList.end(); itr++)  {
//		  itr->second->insertBefore(itr->first);
//	  }

	  //Remove old pairs of instructions
  }

  for (std::list< llvm::Instruction* >::iterator itr = eraseList.begin(); itr != eraseList.end(); itr++) {
	  (*itr)->eraseFromParent();
  }

  return true;
}

S2EArmMergePcThumbPass::InstructionSet S2EArmMergePcThumbPass::getInstructionSet(uint64_t pc)
{
	if (m_instructionSet.find(pc) == m_instructionSet.end())  {
		return UNKNOWN;
	}
	else {
		/* always discard thumb bit */
		return m_instructionSet[pc & 0xfffffffeul];
	}
}

void
S2EArmMergePcThumbPass::setInstructionSetToThumb(uint64_t pc)
{
	m_instructionSet[pc & 0xfffffffeul] = THUMB;
}

