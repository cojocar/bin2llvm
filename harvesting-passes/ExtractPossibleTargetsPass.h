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

#ifndef __EXTRACT_POSSIBLE_TARGETS_PASS_H
#define __EXTRACT_POSSIBLE_TARGETS_PASS_H 1

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

struct ExtractPossibleTargetsPass : public llvm::FunctionPass {
public:
    static char ID;
    ExtractPossibleTargetsPass() : llvm::FunctionPass(ID) { reset();}
    virtual bool runOnFunction(llvm::Function &F);

    /* this vector includes the implicite targets */
    std::vector<uint64_t> getTargets() { return targetPCs;}
    uint64_t getImplicitTarget() { return implicitTargetPC; }
    bool hasImplicitTarget() { return hasImplicit; }

private:
    std::vector<uint64_t> targetPCs;
    uint64_t implicitTargetPC;
    bool hasImplicit;

    void reset();

    llvm::ConstantInt *isStoreToPC(llvm::StoreInst *store);
    llvm::ConstantInt *isStoreToLR(llvm::StoreInst *store);
    bool isIndirectStoreToPC(llvm::StoreInst *store);
};

#endif
