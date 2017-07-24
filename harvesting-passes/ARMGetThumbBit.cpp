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

#include "ARMGetThumbBit.h"

#include "llvm/DataLayout.h"
#include "llvm/Instructions.h"
#include "llvm/Constants.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/raw_ostream.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>

#include <cajun/json/reader.h>
#include <cajun/json/writer.h>

#include <iostream>

#include <llvm/Function.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Scalar.h>
using namespace llvm;

char ARMGetThumbBit::ID = 0;
static RegisterPass<ARMGetThumbBit> X("armthumb",
        "Get the thumb bit after this basic block is executed", false, false);

bool ARMGetThumbBit::runOnFunction(Function &F)
{
    reset();

    for (Function::iterator ibb = F.begin(), ibe = F.end();
            ibb != ibe; ++ibb) {
        if (!isa<ReturnInst>(ibb->getTerminator()))
            continue;
        /* ok, we have a basic block containing a return instrution
         * is the thumb bit set here? The thumb bit is usually a const
         */
        bool gotConstStoreToThumb = false;
        thumb_bit_t thumbVal = THUMB_BIT_UNDEFINED;
        for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
                iib != iie; ++iib) {
            StoreInst *storeInst = dyn_cast<StoreInst>(iib);
            if (!storeInst)
                continue;
            GlobalVariable* thumbReg = dyn_cast<GlobalVariable>
                (storeInst->getPointerOperand());
            if (thumbReg && (thumbReg->getName() == "thumb")) {
                ConstantInt *val = dyn_cast<ConstantInt>(storeInst->getValueOperand());
                if (val) {
                    gotConstStoreToThumb = true;
                    thumbVal = (1 == val->getZExtValue()) ? THUMB_BIT_SET : THUMB_BIT_UNSET;
                    outs() << "[ARMGetThumbBit] Got thumb store: " << thumbVal << "\n";
                } else {
                    outs() << "[ARMGetThumbBit] Storing non-const to thumb bit\n";
                }
            }
        }
        if (gotConstStoreToThumb) {
            /* push only set or unset */
            thumbBits.push_back(thumbVal);
        }
    }

    /* aggregate the result
     * XXX: if we have multiple exit points (with diferrent return, then
     * it will be a problem
     */
    if (thumbBits.size() == 0) {
        thumbBit = THUMB_BIT_UNDEFINED;
    } else {
        bool allTheSame = true;
        std::vector<thumb_bit_t>::iterator it = thumbBits.begin();
        thumb_bit_t t = *it;
        ++it;
        for (; it != thumbBits.end(); ++it) {
            if (*it != t) {
                allTheSame = false;
            }
        }
        if (!allTheSame) {
            errs() << "Function @" << F.getName() << " returns in Thumb AND ARM\n";
        }
        thumbBit = t;
    }
    return false;
}

void ARMGetThumbBit::reset()
{
    thumbBit = THUMB_BIT_UNDEFINED;
    thumbBits.clear();
}
