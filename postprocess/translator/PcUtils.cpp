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

#include <algorithm>

#include "PassUtils.h"
#include "PcUtils.h"

using namespace llvm;

unsigned getConstPcOperand(Instruction *inst) {
    if (isInstStart(inst)) {
        if (ConstantInt *con = dyn_cast<ConstantInt>(
                    cast<StoreInst>(inst)->getValueOperand())) {
            return con->getZExtValue();
        }
    }
    return 0;
}

StoreInst *getFirstInstStart(BasicBlock *bb) {
    foreach(inst, *bb) {
        if (isInstStart(inst))
            return cast<StoreInst>(inst);
    }
    return NULL;
}

StoreInst *getLastInstStart(BasicBlock *bb) {
    if (!isRecoveredBlock(bb))
        return NULL;

    StoreInst *last = NULL;

    do {
        for (Instruction &inst : *bb) {
            if (isInstStart(&inst))
                last = cast<StoreInst>(&inst);
        }

        bb = succ_begin(bb) != succ_end(bb) ? *succ_begin(bb) : NULL;
    } while (bb);

    return last;
}

StoreInst *findInstStart(BasicBlock *bb, unsigned pc) {
    foreach(inst, *bb) {
        if (getConstPcOperand(inst) == pc)
            return cast<StoreInst>(inst);
    }

    foreach2(it, succ_begin(bb), succ_end(bb)) {
        // XXX: would loop infinitely in cyclic CFG
        if (StoreInst *found = findInstStart(*it, pc))
            return found;
    }

    return NULL;
}

unsigned getLastPc(BasicBlock *bb) {
    MDNode *md = bb->getTerminator()->getMetadata("lastpc");
    assert(md && "expected lastpc in metadata node");
    return cast<ConstantInt>(md->getOperand(0))->getZExtValue();
}

unsigned getLastInstStartPc(BasicBlock *bb, bool allowEmpty, bool noRecovered) {
    if (noRecovered && isRecoveredBlock(bb))
        return 0;

    unsigned lastpc = 0;

    for (Instruction &inst : *bb) {
        if (isInstStart(&inst))
            lastpc = getConstPcOperand(&inst);
    }

    foreach2(it, succ_begin(bb), succ_end(bb))
        lastpc = std::max(getLastInstStartPc(*it, allowEmpty, true), lastpc);

    if (!noRecovered && !allowEmpty)
        assert(lastpc);

    return lastpc;
}
