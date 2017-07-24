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
#ifndef __MARKFUNCENTRY_H__
#define __MARKFUNCENTRY_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

/*
 * This pass marks the basic blocks (repesented by functions) that are
 * represeting the entry point of a function.
 *
 * This pass uses only direct calls that were introduced in the
 * metadata. There should be another pass that is capable of recognising
 * certain patterns (function preambels).
 */
struct MarkFuncEntry: public llvm::ModulePass {
    static char ID;

    MarkFuncEntry() : llvm::ModulePass(ID) {}

    virtual bool runOnModule(llvm::Module &M);
};

#endif
