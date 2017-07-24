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

#ifndef __SOLVE_INDIRECT_SINGLE_H__
#define __SOLVE_INDIRECT_SINGLE_H__ 1

#include <llvm/Pass.h>
#include <llvm/Function.h>
#include <llvm/Module.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Constants.h>

#include <string>
#include <cstdint>
#include <map>

struct SolveIndirectSingle: public llvm::FunctionPass {
    static char ID;

    SolveIndirectSingle() : llvm::FunctionPass(ID) {initialize();}

    virtual bool runOnFunction(llvm::Function &f);
    ~SolveIndirectSingle();
private:
    /* the value that is computed. The value is identified by the PC */
    typedef std::map<uint64_t, llvm::ConstantInt *> IndirectValueAtPC;
    /* map for each global (name) to its IndirectValueAtPC */
    std::map<std::string, IndirectValueAtPC *> m_directStores;

    void initialize();
    void expandDirectStoresWithModule(llvm::Module *module);
    std::map<std::string, bool> m_performReplaceOnTheseGlobals;
    std::map<std::string, bool> m_performAnnotationOnTheseGlobals;

    /* return true if function changed */
    bool performAnnotations(llvm::Function *function);
    bool performReplace(llvm::Function *function);
    std::list<uint64_t> m_solvedPCs;
    std::ofstream *m_solvedPCsOutStream;
};

#endif
