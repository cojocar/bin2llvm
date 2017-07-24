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

#include "S2EDeleteInstructionCount.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/GlobalVariable.h"

using namespace llvm;

char S2EDeleteInstructionCountPass::ID = 0;
static RegisterPass<S2EDeleteInstructionCountPass> X("s2edeleteinstructioncount", "S2E delete qemu stores and helper function calls", false, false);

bool S2EDeleteInstructionCountPass::runOnFunction(Function &f) {
  std::list<llvm::Instruction*> eraseList;
  for (Function::iterator bb = f.begin(); bb != f.end(); bb++)  {
      for (BasicBlock::iterator inst = bb->begin(); inst != bb->end(); inst++) {
        StoreInst* storeInst = dyn_cast<StoreInst>(&*inst);
        if (storeInst) {
            GlobalVariable* var = dyn_cast<GlobalVariable>(storeInst->getPointerOperand());
            if (var && var->getName() == "s2e_icount") {
                eraseList.push_back(storeInst);
                continue;
            }
            else if (var && var->getName() == "s2e_current_tb") {
                eraseList.push_back(storeInst);
                continue;
            }
            else if (var && var->getName() == "s2e_icount_before_tb") {
                eraseList.push_back(storeInst);
                continue;
            }
        }

        CallInst* callInst = dyn_cast<CallInst>(&*inst);
        if (callInst && callInst->getCalledFunction()->getName() == "helper_s2e_tcg_execution_handler")  {
            eraseList.push_back(callInst);
            continue;
        }
      }
  }

  for (std::list<llvm::Instruction*>::iterator itr = eraseList.begin(); itr != eraseList.end(); itr++) {
    (*itr)->eraseFromParent();
  }

  return true;
}

