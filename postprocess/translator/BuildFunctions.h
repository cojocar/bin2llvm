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

#ifndef __BUILDFUNCTIONS_H__
#define __BUILDFUNCTIONS_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

/*
 * This pass builds functions from basic blocks. The basic blocks have
 * been renamed as entry bb and the calls (direct and indirect) are
 * already marked through metadata. Also the retuns instructions are
 * marked with metadata.
 * The new functions are exported to functions.bc
 */
struct BuildFunctions : public llvm::ModulePass {
    static char ID;

    BuildFunctions() : llvm::ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &M);
    static void copyGlobalReferences(
        llvm::Module *module,
        llvm::Function *func,
        llvm::ValueToValueMapTy &valueMap);
private:
    void makeFunctionForward(llvm::Module *newModule,
            llvm::Function *newFunc,
            llvm::Function *startBB,
            std::list<llvm::Function *> &toBeRemoved);
    std::map<uint64_t, llvm::Function *> allBBs;
    void buildBBMap(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap);
    /* return true if we have an indirectJump */
    bool exploreBB(llvm::Function *startBB,
            std::list<llvm::Function *> &BBsForThisFunction);
    /* inline bbs */
    llvm::BasicBlock *inlineBBs(llvm::Function *newFunc,
            std::list<llvm::Function *> &BBsForThisFunction,
            bool hasIndirectJump);
    void fixupCalls(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap);
    void fixupDirectJumps(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap);
    void fixupIndirectJumps(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap,
            llvm::BasicBlock *fakeBB);
    void fixupReturns(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap);
    void cleanupFakeReturns(std::list<llvm::BasicBlock *> &bbToUpdate);
    llvm::BasicBlock *getOrCreateEmptyFunction(llvm::Module *m);
    llvm::Function *m_fakeFunction;
    bool had_one_switch;

    llvm::Value *getValueSwitchedOn(llvm::Instruction *storeToPC);
    llvm::Value *__getValueSwitchedOn(llvm::Value *v, llvm::BasicBlock *origBB);
};

#endif
