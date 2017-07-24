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

#ifndef __REPLACE_CONSTANT_LOADS_H__
#define __REPLACE_CONSTANT_LOADS_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

#include <list>
#include <fstream>

#include "JumpTableInfo.h"

/*
 * This pass replaces __ldl_mmu(cst) with the actual value.
 *
 * This pass can be used to discover more functions. Hence increase the
 * coverage.
 */
struct ReplaceConstantLoads: public llvm::FunctionPass {
    class MemoryPool {
    public:
        std::ifstream mp_file;
        uint64_t mp_start;
        uint64_t mp_end;
        MemoryPool(std::string, uint64_t);
        bool inside(uint64_t);
        uint64_t read(uint64_t address, uint8_t byteCnt, bool isBigEndian);
    };
    static char ID;
    std::list<MemoryPool *> m_memoryPools;

    ReplaceConstantLoads() : llvm::FunctionPass(ID) {initialize();}
    ~ReplaceConstantLoads();

    virtual bool runOnFunction(llvm::Function &);
private:
    void initialize();
    MemoryPool *getMemoryPool(std::string);
    llvm::Value *getMemoryValue(uint64_t, llvm::IntegerType *, bool isBigEndian);
    uint64_t getMemoryValue(uint64_t address, uint64_t size, bool isBigEndian);

    std::list<JumpTableInfo *> jumpTableInfoList;
    std::map<uint64_t, JumpTableInfo *>  jumpTableInfoMap;
    bool hasJumpTableInfo;
};
#endif
