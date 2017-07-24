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

#include "RemoveBranchTrace.h"

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

char RemoveBranchTrace::ID = 0;
static RegisterPass<RemoveBranchTrace> X("rmbranchtrace",
        "Remove stores that were used for branch tracing.", false, false);

bool RemoveBranchTrace::runOnFunction(Function &F)
{
  std::list<llvm::Instruction *> eraseIns;

  for (auto bbi = F.begin(), bbie = F.end();
          bbi != bbie;
          ++bbi) {
      for (auto insi = bbi->begin(), insie = bbi->end();
              insi != insie;
              ++insi) {
          StoreInst *store = dyn_cast<StoreInst>(insi);
          if (!store)
              continue;
          ConstantInt *val = dyn_cast<ConstantInt>(store->getOperand(0));
          if (!val)
              continue;

          if (val->getZExtValue() != 0 &&
                  val->getZExtValue() != 1) {
              continue;
          }
          Value *operand = store->getOperand(1);

          Constant *cst = dyn_cast<Constant>(operand);
          if (!cst)
              continue;
          ConstantExpr *cstExpr = dyn_cast<ConstantExpr>(cst);
          if (!cstExpr)
              continue;

          if (!cstExpr->isCast())
              continue;

          if (Type::getInt8PtrTy(F.getContext()) != operand->getType())
              continue;

          /* XXX: this is far from beeing complete, extra instructions
           * might be removed
           */
          eraseIns.push_back(insi);
      }
  }

  for (auto insi = eraseIns.begin(), insie = eraseIns.end();
          insi != insie;
          ++insi) {
      (*insi)->eraseFromParent();
  }

  if (eraseIns.size() > 0) {
      outs() << "[RemoveBranchTrace] erased " << eraseIns.size() << " instructions\n";
      return true;
  }
  return false;
}
