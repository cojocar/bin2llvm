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

#include "ARMMarkCall.h"
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

char ARMMarkCall::ID = 0;
static RegisterPass<ARMMarkCall> X("armmarkcall",
        "Mark call instructions on ARM", false, false);

bool
ARMMarkCall::runOnFunction(llvm::Function &F)
{
    /* search for store to lr and for store to PC
     * if the direct store to lr, points at the end of this BB (func),
     * then we have a call.
     */
    uint64_t pcStart, pcEnd;
    bool modified = false;
    pcStart = FixOverlappedBBs::getPCStartOfFunc(&F);
    pcEnd = FixOverlappedBBs::getPCEndOfFunc(&F);
    for (auto bbi = F.begin(), bbie = F.end();
            bbi != bbie;
            ++bbi) {
        bool hasLRStore = false;
        uint64_t lrVal = 0;
        for (auto insi = bbi->begin(), insie = bbi->end();
                insi != insie;
                ++insi) {
            StoreInst *storeInst = dyn_cast<StoreInst>(insi);
            if (!storeInst)
                continue;
            GlobalVariable *gv = dyn_cast<GlobalVariable>(storeInst->getOperand(1));
            ConstantInt* val = dyn_cast<ConstantInt>(storeInst->getValueOperand());
            if (!gv)
                continue;
            if (gv->getName() == "PC") {
                if (hasLRStore) {
                    if (pcEnd == (lrVal) || pcEnd == lrVal-1) {
                        /* XXX kind of hackish */
                        LLVMContext &ctx = insi->getContext();
                        if (val) {
                            insi->setMetadata("INS_directCall", MDNode::get(ctx, MDString::get(ctx,
                                            FixOverlappedBBs::hex(val->getZExtValue()))));
                            insi->setMetadata("INS_callReturn", MDNode::get(ctx, MDString::get(ctx,
                                            FixOverlappedBBs::hex(lrVal & -2))));
                        } else {
                            insi->setMetadata("INS_indirectCall", MDNode::get(ctx, MDString::get(ctx,
                                            FixOverlappedBBs::hex(0xdeadbeef))));
                            insi->setMetadata("INS_callReturn",
                                    MDNode::get(ctx, MDString::get(ctx,
                                            FixOverlappedBBs::hex(lrVal & -2))));
                        }
                        modified = true;
                    } else {
                        outs() << "[ARMMarkCall] direct store to LR not return: " <<
                            FixOverlappedBBs::hex(lrVal) << "\n";
                    }
                }
            } else if (gv->getName() == "LR") {
                if (val) {
                    lrVal = val->getZExtValue();
                    hasLRStore = true;
                    outs() << "[ARMMarkCall] direct store to LR: " <<
                        FixOverlappedBBs::hex(lrVal) << "\n";
                } else {
                    outs() << "[ARMMarkCall] indirect store to LR" << "\n";
                }
            }
        }
    }
    return modified;
}
