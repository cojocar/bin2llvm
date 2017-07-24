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

#include "FunctionRename.h"
#include "FixOverlappedBBs.h"

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
#include <string>
using namespace llvm;

char FunctionRename::ID = 0;
static RegisterPass<FunctionRename> X("funcrename",
        "Rename final function is function@0xADDR", false, false);

bool
FunctionRename::runOnFunction(llvm::Function &F)
{
    if (!std::strstr(F.getName().data(), "linked-final"))
        return false;
    if (F.empty())
        return false;
    BasicBlock &b = F.getEntryBlock();
    if (b.empty())
        return false;
    Instruction &ins = b.front();
    if (!ins.getMetadata("INS_currPC"))
        return false;

    uint64_t pcStart = FixOverlappedBBs::
        getHexMetadataFromIns(&ins, "INS_currPC");
    std::string newName = std::string("Function_") +
        FixOverlappedBBs::hex(pcStart);
    outs() << "[FunctionRename] " << F.getName().data() << " -> " <<
        newName << "\n";
    F.setName(newName);
    return true;
}
