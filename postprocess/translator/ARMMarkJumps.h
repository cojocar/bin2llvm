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

#ifndef __ARM_MARK_JUMPS_H__
#define __ARM_MARK_JUMPS_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

/*
 * This pass marks the jump instructions with INS_{direct,inderct}Jump
 * INS_directJump = target
 * INS_indirectJump = 0xdeadbeef
 *
 * This pass should run after ARMMarkCall and ARMMarkReturn.
 */
struct ARMMarkJumps: public llvm::FunctionPass {
    static char ID;

    ARMMarkJumps() : llvm::FunctionPass(ID) {}

    virtual bool runOnFunction(llvm::Function &F);
};

#endif
