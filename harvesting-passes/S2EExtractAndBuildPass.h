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

#ifndef __S2EEXTRACT_AND_BUILD_PASS_H__
#define __S2EEXTRACT_AND_BUILD_PASS_H__ 1

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

  struct S2EExtractAndBuildPass : public llvm::FunctionPass {
    typedef std::map<llvm::BasicBlock*, unsigned long> targetmap_t;
    targetmap_t targetMap; 
    static char ID;
    S2EExtractAndBuildPass() : llvm::FunctionPass(ID) { initialized = false; }

    virtual bool runOnFunction(llvm::Function &F);
    void setBlockAddr(unsigned long startAddr, unsigned long endAddr) { currBlockAddr = startAddr; nextBlockAddr = endAddr; };
    bool hasImplicitTarget() {return implicitTarget;}
    llvm::Module* getBuiltModule() { return M; }
    size_t getFunctionCount() { return functions.size(); }

  private:
    bool initialized;
    llvm::Module *M;
    unsigned long currBlockAddr;
    unsigned long nextBlockAddr;
    bool implicitTarget;
    typedef std::map<std::string, llvm::BasicBlock*> bbmap_t;
    bbmap_t basicBlocks;
    typedef std::map<std::string, llvm::Function*> fmap_t;
    fmap_t functions;
    llvm::FunctionType* genericFunctionType;
    void initialize(llvm::Function &F);
    void processPCUse(llvm::Value *pcUse);
    bool searchNextBasicBlockAddr(llvm::Function &F);
    llvm::Function* findParentFunction();
    void copyGlobalReferences(llvm::Function &F, llvm::ValueToValueMapTy &VMap);
    void createDirectEdge(llvm::BasicBlock *block, unsigned long target);
    void createIndirectEdge(llvm::BasicBlock *block);
    void createCallEdge(llvm::BasicBlock *block, unsigned long target, bool isTailCall = false);
    void createIndirectCallEdge(llvm::BasicBlock *block);
    llvm::BasicBlock* getBasicBlock(std::string name);
    llvm::BasicBlock* getOrCreateBasicBlock(llvm::Function *parent, std::string name);
    llvm::BasicBlock* getOrCreateTransBasicBlock(llvm::Function *parent, unsigned long addr);
    llvm::Function* getFunction(std::string name);
    llvm::Function* getOrCreateFunction(std::string name);
    void moveBasicBlockToNewFunction(llvm::BasicBlock *block, llvm::Function *parent);
    void moveBasicBlocksToNewFunction(llvm::BasicBlock *block, llvm::Function *parent);
    void relinkOldTransBasicBlock(llvm::Function *newF, std::string addr, std::list<llvm::Instruction *> &terminatorList);
    bool validateMove(llvm::BasicBlock *block);
    llvm::ConstantInt *isStoreToPC(llvm::StoreInst *store);
//#ifdef TARGET_ARM cannot enable this, why?
    llvm::ConstantInt *isStoreToLR(llvm::StoreInst *store);
//#endif
    bool isIndirectStoreToPC(llvm::StoreInst *store);
  };

#endif
