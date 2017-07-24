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

#ifndef __FIX_OVERLAPPED_BBS_H__
#define __FIX_OVERLAPPED_BBS_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

#include <string>
#include <cstdint>

/*
 * basic blocks can overlap.
 * This pass will fix any overlapping basic blocks by splitting the
 * bigger basic block, and inserting a jump, to the remainder basic
 * block.
 *
 * This module works with the assumption that the basic blocks will pe
 * overlapp at the ending instructions.
 *
 */
struct FixOverlappedBBs: public llvm::ModulePass {
    static char ID;

    FixOverlappedBBs() : llvm::ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &f);
    static uint64_t getPCEndOfFunc(llvm::Function *func);
    static uint64_t getPCStartOfFunc(llvm::Function *func);
    static uint64_t getCurrentPCOfIns(llvm::Instruction *ins);
    static uint64_t getBBStart(llvm::Instruction *ins);

    static std::string hex(uint64_t val);
    static uint64_t getNumberOfASMInstructions(llvm::Function *func);

    static uint64_t getHexMetadataFromIns(
            llvm::Instruction *ins,
            std::string metadataName);
private:
    static uint64_t getHexMetadataFromFunc(
            llvm::Function *func,
            std::string metadataName);
    static void truncateFuncAndLinkWith(llvm::Function *a, llvm::Function *b);
    static void linkWith(llvm::Function *srcFunc, llvm::BasicBlock *srcBB, uint64_t target);
    static bool myCompareStartOfFunc(llvm::Function *a, llvm::Function *b);
};
#endif
