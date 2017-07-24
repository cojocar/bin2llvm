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

#include "ARMMarkReturn.h"
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
#include <string>

using namespace llvm;

char ARMMarkReturn::ID = 0;
static RegisterPass<ARMMarkReturn> X("armmarkreturn",
        "Mark return instruction on ARM", false, false);

bool
ARMMarkReturn::runOnFunction(llvm::Function &F)
{
    /* on arm, the return instruction is bx lr
     * note, that lr might not be explicitely used
     * pop {lr}
     * bx lr, is a special case in wich, in the llvm code, there is no
     */

    /* search for indirect stores to pc */
    uint64_t pcStart, pcEnd;
    bool modified = false;
    pcStart = FixOverlappedBBs::getPCStartOfFunc(&F);
    pcEnd = FixOverlappedBBs::getPCEndOfFunc(&F);
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
            Value *val = storeInst->getValueOperand();
            if (gv && gv->getName() == "PC" && !isa<ConstantInt>(val)) {
                //outs() << "[ARMMarkReturn] non constant store, return candidate: " << *val
                //    << "\n" ;
                if (usesLR(val)) {
                    /* we should not have marked already as a Call */
                    assert(!insi->getMetadata("INS_directCall"));
                    assert(!insi->getMetadata("INS_indirectCall"));
                    insi->setMetadata("INS_return", MDNode::get(
                                insi->getContext(),
                                MDString::get(insi->getContext(),
                                    std::string("true"))));
                    bbi->setName("func_return_bb_"+std::string(bbi->getName()));
                    modified = true;
                } else {
                //    outs() << "[ARMMarkReturn] no " << "\n";
                }
            }
        }
    }
    return modified;
}

bool
ARMMarkReturn::usesLR(llvm::Value *val)
{
    assert(!isa<ConstantInt>(val));
    BinaryOperator *storeToPcOp = dyn_cast<BinaryOperator>(val);
    /* first case, store LR & -2 to PC */
    if (storeToPcOp) {
        //outs() << "[ARMMarkReturn] bin-op" << "\n";
        /* binary op to PC */
        if (storeToPcOp->getOpcode() == Instruction::And) {
            //outs() << "[ARMMarkReturn] AND " << "\n";
            /* this is an and */
            LoadInst *valueToStoreToPc = dyn_cast<LoadInst>
                (storeToPcOp->getOperand(0));
            ConstantInt *cst = dyn_cast<ConstantInt>
                (storeToPcOp->getOperand(1));
            if (valueToStoreToPc && cst) {
                /* we do have a load */
                GlobalVariable* var = dyn_cast<GlobalVariable>
                    (valueToStoreToPc->getPointerOperand());
                if (var && var->getName() == "LR") {
                    outs() << "[ARMMarkReturn] got direct store to LR: " << val->getName() << "\n";
                    return true;
                }
            } else {
                //outs() << "[ARMMarkReturn] not lr " << "\n";
                /* second case: pop {lr}, bx lr; alias
                */
                Value *ins = storeToPcOp->getOperand(0);
                if (isa<CallInst>(ins)) {
                    /* assert that this call instr is __ldl_mmu via
                     * stack
                     */
                    for (auto ui = ins->use_begin(), uie = ins->use_end();
                            ui != uie;
                            ++ui) {
                        //outs() << "[ARMMarkReturn] use: " << *(*ui) << "\n";
                        StoreInst *store = dyn_cast<StoreInst>(*ui);
                        if (store) {
                            //outs() << "[ARMMarkReturn] got store " << *(*ui) << "\n";
                            GlobalVariable *LR = dyn_cast<GlobalVariable>
                                (store->getOperand(1));
                            if (LR && LR->getName() == "LR") {
                                outs() << "[ARMMarkReturn] alias store to LR: " << val->getName() << "\n";
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}
