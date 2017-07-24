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

/*
 * For an llvm function, extract possible visitedPCs. This is the partial
 * replacement of extract and build pass.
 *
 * At this level, the llvm function corresponds to a basic block. This
 * pass should run after the registers were renamed.
 */
#include "ExtractPossibleTargetsPass.h"

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

char ExtractPossibleTargetsPass::ID = 0;
static RegisterPass<ExtractPossibleTargetsPass> X("s2eextract",
        "Extract Possible visitedPCs from a bb (function)", false, false);

bool ExtractPossibleTargetsPass::runOnFunction(Function &F)
{

    reset();
    /* the assumption is the current function contains just one basic
     * block. This is /always/ valid for the code translated by qemu
     */
    int count = 0;
    for (Function::iterator ibb = F.begin(), ibe = F.end(); ibb != ibe; ++ibb) {
        //errs() << "BB_" << count << " : " << *ibb << "\n";
        ++count;
    }
    outs() << "[ExtractPossibleTargets] Basic Block counts: " << count << "\n";

    for (Function::iterator ibb = F.begin(), ibe = F.end();
            ibb != ibe; ++ibb) {
        if (!isa<ReturnInst>(ibb->getTerminator()))
            continue;
        bool gotConstStoreToPC = false;
#ifdef TARGET_ARM
        bool gotConstStoreToLR = false;
        uint64_t lastStoreToLRValue = (uint64_t)-1;
#endif
        uint64_t lastStoreToPCValue = (uint64_t)-1;
        /* this basic block, will do a return, let's search for the last
         * store to PC in this basic block
         */
        for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
                iib != iie; ++iib) {
            StoreInst *storeInst = dyn_cast<StoreInst>(iib);
            if (!storeInst)
                continue;
            ConstantInt *constStoreToPC = isStoreToPC(storeInst);
            if (constStoreToPC) {
                gotConstStoreToPC = true;
                lastStoreToPCValue = constStoreToPC->getZExtValue();
            }
#ifdef TARGET_ARM
            ConstantInt *constStoreToLR = isStoreToLR(storeInst);
            if (constStoreToLR) {
                gotConstStoreToLR = true;
                lastStoreToLRValue = constStoreToLR->getZExtValue();
            }
#endif
        }
        if (gotConstStoreToPC) {
            targetPCs.push_back(lastStoreToPCValue);
        } else {
            outs() << "[ExtractPossibleTargets] Storring non-const to PC\n";
            /* TODO: more advanced data flow to track PC,
             * mark a flag for later exploration
             */
        }
#ifdef TARGET_ARM
        if (gotConstStoreToLR) {
            targetPCs.push_back(lastStoreToLRValue);
            assert(!hasImplicit);
            hasImplicit = true;
            implicitTargetPC = lastStoreToLRValue;
        } else {
            outs() << "[ExtractPossibleTargets] Storring non-const to LR\n";
            /* TODO: more advanced data flow to track LR,
             * mark a flag for later exploration
             */
        }
#endif
    }

    return false;
}

void ExtractPossibleTargetsPass::reset()
{
    targetPCs.clear();
    hasImplicit = false;
}

#ifdef TARGET_ARM
static const unsigned int pc_offset = 0xc0;
static const unsigned int lr_offset = 0xbc;
#elif defined(TARGET_I386)
static const unsigned int pc_offset = 12*4;
#endif

llvm::ConstantInt *ExtractPossibleTargetsPass::isStoreToPC(StoreInst *store)
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
llvm::ConstantInt *ExtractPossibleTargetsPass::isStoreToLR(StoreInst *store)
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

bool ExtractPossibleTargetsPass::isIndirectStoreToPC(StoreInst *store)
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

