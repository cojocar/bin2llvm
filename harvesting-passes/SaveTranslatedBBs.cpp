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

#include "SaveTranslatedBBs.h"

#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Metadata.h>

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>


#include <iostream>

using namespace llvm;

void
SaveTranslatedBBs::annotateNewFunction(llvm::Function &func,
        s2e::plugins::MyTranslationBasicBlock *bb)
{
    llvm::LLVMContext &ctx = func.getContext();
    BasicBlock *e;
    e = &func.getEntryBlock();

    MDNode *pcStart = MDNode::get(ctx, MDString::get(ctx, hex(bb->m_pcStart)));
    MDNode *pcEnd = MDNode::get(ctx, MDString::get(ctx, hex(bb->m_pcEnd)));

    for (Function::iterator ibb = func.begin(), ibe = func.end();
            ibb != ibe;
            ++ibb) {
        for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
                iib != iie;
                ++iib) {
            Instruction *ins = dyn_cast<Instruction>(iib);
            ins->setMetadata("BB_pcStart", pcStart);
            ins->setMetadata("BB_pcEnd", pcEnd);
        }
    }
}

std::string
SaveTranslatedBBs::hex(uint64_t val)
{
    /* TODO: ugh */
    char s[128];
    sprintf(s, "0x%08lx", val);
    return std::string(s);
}

llvm::Module *
SaveTranslatedBBs::createAndSaveTranslatedBasicBlocksAsAModule(
        std::vector<s2e::plugins::MyTranslationBasicBlock *> *allBBs,
        std::string fileName)
{
    std::string error;
    llvm::Module *newModule = new llvm::Module("ConcatenatedBBs",
            llvm::getGlobalContext());

    llvm::ValueToValueMapTy valueMap;

    for (std::vector<s2e::plugins::MyTranslationBasicBlock *>::iterator bb =
            allBBs->begin();
            bb != allBBs->end(); ++bb) {
        s2e::plugins::MyTranslationBasicBlock *transBB = *bb;

        llvm::Function *oldFunc = transBB->m_bbFunction;
        llvm::Constant *c = newModule->getOrInsertFunction(oldFunc->getName(),
                oldFunc->getFunctionType());

        llvm::SmallVector<llvm::ReturnInst *, 5> tempList;
        llvm::Function *newFunc = cast<llvm::Function>(c);

        llvm::Argument *arg0 = oldFunc->getArgumentList().begin();
        llvm::Constant *constant = llvm::ConstantPointerNull::get((llvm::PointerType*)
                arg0->getType());
        valueMap[arg0] = constant;
        copyGlobalReferences(newModule, oldFunc, valueMap);
        llvm::CloneFunctionInto(newFunc, oldFunc, valueMap, true, tempList);

        /* add metadata */
        annotateNewFunction(*newFunc, transBB);

        /*
        {
            AttrBuilder B;
            B.addAttribute(Attributes::NoUnwind);
            B.addAttribute(Attributes::StackProtect);
            B.addAttribute(Attributes::UWTable);

            AttributeWithIndex awi = AttributeWithIndex::get(0,
                    Attributes::get(newModule->getContext(), B));

            SmallVector<AttributeWithIndex, 1> arr;
            arr.push_back(awi);
            AttrListPtr func_main_PAL;
            func_main_PAL = AttrListPtr::get(newModule->getContext(), arr);
            newFunc->setAttributes(func_main_PAL);
        }
        */
        //transBB->m_pcStart;
    }

    llvm::raw_fd_ostream bitcodeOstream(
            fileName.c_str(),
            error, 0);
    llvm::WriteBitcodeToFile(newModule,
                    bitcodeOstream);
    bitcodeOstream.close();

    return newModule;
}

void
SaveTranslatedBBs::copyGlobalReferences(
        llvm::Module *module,
        llvm::Function *func,
        llvm::ValueToValueMapTy &valueMap)
{
    for (Function::iterator ibb = func->begin(), ibe = func->end();
            ibb != ibe; ++ibb) {
        for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
                iib != iie; ++iib)
            for (User::op_iterator iob = iib->op_begin(), ioe = iib->op_end();
                    iob != ioe; ++iob) {
                GlobalVariable *globalVar = dyn_cast<GlobalVariable>(*iob);
                if (globalVar) {
                    Constant *newGlobal =
                        module->getOrInsertGlobal(globalVar->getName(),
                                globalVar->getType()->getElementType());
                    valueMap[globalVar] = newGlobal;
                }
                Function *globalFun = dyn_cast<Function>(*iob);
                if (globalFun) {
                    Constant *newGlobal =
                        module->getOrInsertFunction(globalFun->getName(),
                                globalFun->getFunctionType());
                    valueMap[globalFun] = newGlobal;
                }
            }
    }
}

void
SaveTranslatedBBs::saveTranslatedBasicBlocks(
        std::vector<s2e::plugins::MyTranslationBasicBlock *> *allBBs,
        std::string fileName)
{
    std::string error;

    /* save to ll file */
    llvm::raw_fd_ostream llvmOstream(
            fileName.c_str(),
            error, 0);
    for (std::vector<s2e::plugins::MyTranslationBasicBlock *>::iterator bb =
            allBBs->begin();
            bb != allBBs->end(); ++bb) {

        llvmOstream << "; bb@" <<
            ((*bb)->m_pcStart)<< "\n";
        llvmOstream << *((*bb)->m_bbFunction) << "\n;@@@@@@@@@\n";
        outs() << "[SaveTranslatedBBs] bb@" << (*bb)->m_pcStart << " -- " << (*bb)->m_pcEnd << "\n";
    }
    llvmOstream.close();
}

