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

#ifndef __REMOVE_BRANCH_TRACE_H__
#define __REMOVE_BRANCH_TRACE_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

/*
 * This pass removes instructions like:
 *
 * store i8 0, i8* inttoptr (i64 139938138739640 to i8*)
 *
 * The instructions are introduced by s2e/qemu in order to perform
 * branch tracing.
 */
struct RemoveBranchTrace: public llvm::FunctionPass {
    static char ID;

    RemoveBranchTrace() : llvm::FunctionPass(ID) {}

    virtual bool runOnFunction(llvm::Function &);
};
#endif
