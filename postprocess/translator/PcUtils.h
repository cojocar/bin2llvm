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

#ifndef _PCUTILS_H
#define _PCUTILS_H

using namespace llvm;

#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>

unsigned getConstPcOperand(Instruction *inst);
StoreInst *getFirstInstStart(BasicBlock *bb);
StoreInst *getLastInstStart(BasicBlock *bb);
StoreInst *findInstStart(BasicBlock *bb, unsigned pc);
unsigned getLastPc(BasicBlock *bb);
unsigned getLastInstStartPc(BasicBlock *bb, bool allowEmpty = false, bool noRecovered = false);

static inline bool isInstStart(Instruction *inst) {
    return inst->getMetadata("inststart");
}

static inline unsigned getLastPc(Function *f) {
    return getLastPc(&f->getEntryBlock());
}

template<typename T>
static bool blocksOverlap(T *a, T *b) {
    unsigned aStart = getBlockAddress(a);
    unsigned aEnd = getLastPc(a);
    unsigned bStart = getBlockAddress(b);
    unsigned bEnd = getLastPc(b);
    return bStart > aStart && bEnd == aEnd;
}

template<typename T>
static inline bool haveSameEndAddress(T *a, T *b) {
    return getLastPc(a) == getLastPc(b);
}

template<typename T>
static inline bool blockContains(T *bb, unsigned pc) {
    return pc >= getBlockAddress(bb) && pc <= getLastPc(bb);
}

#endif  // _PCUTILS_H
