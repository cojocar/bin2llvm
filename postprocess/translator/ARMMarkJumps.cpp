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

#include "ARMMarkJumps.h"
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

#include <map>
#include <list>

using namespace llvm;

char ARMMarkJumps::ID = 0;
static RegisterPass<ARMMarkJumps> X("armmarkjumps",
        "Mark jumps for ARM arch", false, false);

bool
ARMMarkJumps::runOnFunction(llvm::Function &F)
{
    bool modified = false;
    LLVMContext &ctx = F.getContext();
    for (auto bbi = F.begin(), bbie = F.end();
            bbi != bbie;
            ++bbi) {
        for (auto insi = bbi->begin(), insie = bbi->end();
                insi != insie;
                ++insi) {
            StoreInst *storeInst = dyn_cast<StoreInst>(insi);
            if (!storeInst)
                continue;
            GlobalVariable *gv = dyn_cast<GlobalVariable>(storeInst->getOperand(1));
            ConstantInt* val = dyn_cast<ConstantInt>(storeInst->getValueOperand());
            if (!(gv && gv->getName() == "PC"))
                continue;

            if (insi->getMetadata("INS_directCall") ||
                    storeInst->getMetadata("INS_indirectCall") ||
                    storeInst->getMetadata("INS_return"))
                continue;

            /* we got an untagged store to PC */
            if (val) {
                storeInst->setMetadata("INS_directJump",
                        MDNode::get(ctx, MDString::get(ctx,
                                FixOverlappedBBs::hex(val->getZExtValue()))));
            } else {
                storeInst->setMetadata("INS_indirectJump",
                        MDNode::get(ctx, MDString::get(ctx,
                                FixOverlappedBBs::hex(0xdeadbeef))));
                outs() << "[ARMMarkJumps] found indirect call at " <<
                    F.getName() << "\n";
            }
            modified = true;
        }
    }
    return modified;
}
