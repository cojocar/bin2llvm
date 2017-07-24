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

#include "RemoveExtraStoreToPC.h"

#include "FixOverlappedBBs.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <list>

using namespace llvm;

char RemoveExtraStoreToPC::ID = 0;
static RegisterPass<RemoveExtraStoreToPC> X("rmexterstoretopc",
        "Remove extra stores to PC", false, false);

bool RemoveExtraStoreToPC::runOnFunction(Function &F)
{
  std::list<llvm::Instruction *> eraseIns;

  llvm::LLVMContext& ctx = F.getContext();
  uint64_t pcStart, pcEnd;
  pcStart = FixOverlappedBBs::getPCStartOfFunc(&F);
  pcEnd = FixOverlappedBBs::getPCEndOfFunc(&F);

  MDNode *pcMeta = MDNode::get(ctx, MDString::get(ctx,
              FixOverlappedBBs::hex(
                  pcStart)));

  for (auto bbi = F.begin(), bbe = F.end();
          bbi != bbe;
          ++bbi) {

      int countStoresToPC = 0;
      bool isExitBB = isa<ReturnInst>(bbi->getTerminator());
      /* figure out which is the last store to PC in this BB
       */
      Instruction *lastStoreToPC = NULL;
      for (auto insi = bbi->begin(), inse = bbi->end();
              insi != inse;
              ++insi) {
          insi->setMetadata("INS_currPC", pcMeta);
          StoreInst *storeInst = dyn_cast<StoreInst>(insi);
          if (storeInst) {
              GlobalVariable *PC = dyn_cast<GlobalVariable>(storeInst->getOperand(1));
              ConstantInt* pcValue = dyn_cast<ConstantInt>(storeInst->getValueOperand());
              if (PC && PC->getName() == "PC") {

                  ++countStoresToPC;
                  lastStoreToPC = storeInst;

                  if (pcValue)
                      pcMeta = MDNode::get(ctx, MDString::get(ctx,
                                  FixOverlappedBBs::hex(pcValue->getZExtValue())));
              }
          }
      }

      if (countStoresToPC == 0) {
          errs() << "[RemoveExtraStoreToPC] basic block " <<
              bbi->getName() << " from func " << F.getName() <<
              " has not stores to PC\n";
          /* FIXME: this could be a problem, if we really trust currPC in
           * later stages. Here, we do not track control flow but only
           * linearily iterate through BBs.
           */
          continue;
      }

      assert(countStoresToPC > 0);
      assert(lastStoreToPC);

      /* add the other stores to eraseList */
      for (auto insi = bbi->begin(), inse = bbi->end();
              insi != inse;
              ++insi) {
          StoreInst *storeInst = dyn_cast<StoreInst>(insi);
          if (!storeInst)
              continue;
          GlobalVariable *PC = dyn_cast<GlobalVariable>(storeInst->getOperand(1));
          ConstantInt* pcValue = dyn_cast<ConstantInt>(storeInst->getValueOperand());
          if (PC && PC->getName() == "PC" && pcValue) {
              /* for the BBs that are not exit, remove all pc accesses
               */
              if (lastStoreToPC != storeInst || !isExitBB) {
                  eraseIns.push_back(storeInst);
                  /* test if the removed pc is inside the current BB */
                  if (!(pcValue->getZExtValue() >= pcStart &&
                          pcValue->getZExtValue() <= pcEnd)) {
                      errs() << "[RemoveExtraStoreToPC] outside the current BB " <<
                          FixOverlappedBBs::hex(pcValue->getZExtValue()) << " [" <<
                          FixOverlappedBBs::hex(pcStart) << " " <<
                          FixOverlappedBBs::hex(pcEnd) << "]\n";
                  }
                  assert(pcValue->getZExtValue() >= pcStart && pcValue->getZExtValue() <= pcEnd);
              }
          }
      }
  }

  for (auto ins = eraseIns.begin(), inse = eraseIns.end();
          ins != inse;
          ++ins) {
      (*ins)->eraseFromParent();
  }

  if (eraseIns.size() > 0)
      return true;
  return false;
}
