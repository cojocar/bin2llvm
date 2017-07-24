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

#ifndef __TRANSLFORM_BB_TO_VOID_H_
#define __TRANSLFORM_BB_TO_VOID_H_ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

/*
 * This transformation pass removes the extra return instruction
 * introduced by qemu llvm. The return type of the function is chnaged
 * to void. Usually, this pass should runs over the newly extracted BBs
 * (at this level, each BB is represented by a function).
 */
struct TransformBBToVoid: public llvm::ModulePass {
    static char ID;

    TransformBBToVoid() : llvm::ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &f);

private:
    static void copyGlobalReferences(
            llvm::Module *module,
            llvm::Function *func,
            llvm::ValueToValueMapTy &valueMap);
};

#endif
