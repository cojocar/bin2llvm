/* vim: set expandtab ts=4 sw=4: */
#include "TransformBBToVoid.h"
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
#include "TransformBBToVoid.h"

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <list>

using namespace llvm;

char TransformBBToVoid::ID = 0;
static RegisterPass<TransformBBToVoid> X("transfrombbtovoid",
        "Transform all functions to void", false, false);

void
TransformBBToVoid::copyGlobalReferences(
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
                                globalFun->getFunctionType(),
                                globalFun->getAttributes());
                    valueMap[globalFun] = newGlobal;
                }
            }
    }
}

bool TransformBBToVoid::runOnModule(Module &M)
{
    std::list<llvm::Instruction *> eraseIns;
    std::list<llvm::Function *>eraseFuncs;
    int count = 0;

    llvm::FunctionType *voidFunctionType =
        FunctionType::get(llvm::Type::getVoidTy(M.getContext()), false);
    llvm::ValueToValueMapTy valueMap;

    for (Module::iterator ifunc = M.begin(), ifunce = M.end();
            ifunc != ifunce;
            ++ifunc) {
        llvm::Function *oldFunc = ifunc;
        if (!std::strstr(oldFunc->getName().data(), "tcg-llvm-tb"))
            /* this is an external function, rewrite only bbs */
            continue;
        if (std::strstr(oldFunc->getName().data(), "void-tcg-llvm-tb"))
            /* already rewrote */
            continue;

        /* crate the function with void type */
        llvm::Constant *c = M.getOrInsertFunction(
                "void-"  + std::string(oldFunc->getName()),
                voidFunctionType);
        llvm::Function *newFunc = cast<llvm::Function>(c);

        assert(newFunc);
        assert(oldFunc->getArgumentList().size() == 1);

        /* prepare map for clone function into */
        Argument *arg0 = oldFunc->getArgumentList().begin();
        llvm::Constant *paramType = llvm::ConstantPointerNull::get((llvm::PointerType*)
                arg0->getType());
        valueMap[arg0] = paramType;

        /* clone with a new type */
        copyGlobalReferences(&M, oldFunc, valueMap);

        llvm::SmallVector<llvm::ReturnInst *, 5> terms;
        //outs() << "[TransformBBToVoid] clone " << oldFunc->getName() << " into " <<
        //    newFunc->getName() << "\n";
        llvm::CloneFunctionInto(newFunc, oldFunc, valueMap, true, terms);
        ++count;

        LLVMContext &ctx = newFunc->getContext();
        /* replace the return instructions with ret void */
        for (auto ins = terms.begin(), inse = terms.end();
                ins != inse;
                ++ins) {
            ReturnInst *ret =
                    ReturnInst::Create((*ins)->getContext());
            ret->setMetadata("INS_fakeReturn", MDNode::get(ctx,
                        MDString::get(ctx,
                            std::string("true"))));
            llvm::ReplaceInstWithInst(*ins, ret);
        }

        /* remove the old function */
        eraseFuncs.push_back(oldFunc);
    }

    /* erase old functions */
    for (auto func = eraseFuncs.begin(), funce = eraseFuncs.end();
            func != funce;
            ++func) {
        (*func)->eraseFromParent();
    }
    outs() << "[TransformBBToVoid] cloned " << count << " functions\n";

    if (eraseFuncs.size() > 0)
        return true;

    return false;
}
