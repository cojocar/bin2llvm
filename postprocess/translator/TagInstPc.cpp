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

#include "TagInstPc.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char TagInstPc::ID = 0;
static RegisterPass<TagInstPc> X("tag-inst-pc",
        "S2E Add metadata annotations to PC stores at instruction boundaries",
        false, false);

static unsigned getLastPc(BasicBlock *bb) {
    unsigned lastpc = 0;

    for (Instruction &inst : *bb) {
        if (inst.getMetadata("inststart")) {
            lastpc = cast<ConstantInt>(cast<StoreInst>(&inst)->getValueOperand())->getZExtValue();
        }
    }

    assert(lastpc);
    return lastpc;
}

bool TagInstPc::runOnModule(Module &m) {
    GlobalVariable *PC = m.getNamedGlobal("PC");
    GlobalVariable *s2e_icount = m.getNamedGlobal("s2e_icount");
    assert(PC && s2e_icount && "globals missing");
    MDNode *md = MDNode::get(m.getContext(), NULL);
    LLVMContext &ctx = m.getContext();

    foreach_use_if(s2e_icount, StoreInst, icountStore, {
        BasicBlock::iterator it(icountStore);
        StoreInst *pcStore = NULL;
        while (!pcStore)
            pcStore = dyn_cast<StoreInst>(--it);
        assert(pcStore->getPointerOperand() == PC &&
            "store before icount does not store to PC");
        pcStore->setMetadata("inststart", md);

        // update lastpc annotation on function
        unsigned pc = cast<ConstantInt>(pcStore->getValueOperand())->getZExtValue();
        Function *f = pcStore->getParent()->getParent();

        if (MDNode *mdlast = getBlockMeta(f, "lastpc")) {
            unsigned curlastpc = cast<ConstantInt>(mdlast->getOperand(0))->getZExtValue();
            if (curlastpc >= pc)
                continue;
        }

        setBlockMeta(f, "lastpc", pc);
    });

    return true;
}
