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

/* For a function (maybe incomplete, just a set of basic blocks, search
 * if there is a store to thumb bit
 * asume that the TransformPass was applied, i.e. the store is done to a
 * glabal value called thumb
 */
#ifndef __ARM_GET_HUMB_BIT_H
#define __ARM_GET_HUMB_BIT_H 1

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "llvm/Instructions.h"
#include "llvm/Constants.h"

#include <vector>
#include <map>
#include <list>

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>

struct ARMGetThumbBit : public llvm::FunctionPass {
public:
    static char ID;
    ARMGetThumbBit() : llvm::FunctionPass(ID) { reset();}
    virtual bool runOnFunction(llvm::Function &F);
    enum thumb_bit_t {
        THUMB_BIT_UNDEFINED,
        THUMB_BIT_SET,
        THUMB_BIT_UNSET,
        THUMB_BIT_END,
    };

    thumb_bit_t getThumbBit() { return thumbBit; }

private:
    thumb_bit_t thumbBit;
    std::vector<thumb_bit_t> thumbBits;

    void reset();
};
#endif
