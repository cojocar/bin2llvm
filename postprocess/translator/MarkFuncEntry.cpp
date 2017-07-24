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

#include "MarkFuncEntry.h"
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

#include <map>
#include <list>

using namespace llvm;

char MarkFuncEntry::ID = 0;
static RegisterPass<MarkFuncEntry> X("markfuncentry",
        "Rename the entry basic block to function_entry", false, false);

bool
MarkFuncEntry::runOnModule(llvm::Module &M)
{
    std::list<BasicBlock *> funcEntryBBs;
    std::map<uint64_t, bool> directCallTargets;
    bool modified = false;
    bool thisBBisEntry = false;

    uint64_t pcStart;
    for (auto funci = M.begin(), funcie = M.end();
            funci != funcie;
            ++funci) {
        /* TODO: find another way to skip uninteresting functions */
        if (!std::strstr(funci->getName().data(), "void-tcg-llvm-tb"))
            continue;
        assert(funci->size());
        BasicBlock &b = funci->front();

        for (auto insi = b.begin(), insie = b.end();
                insi != insie;
                ++insi) {
            if (insi->getMetadata("INS_directCall")) {
                uint64_t target = FixOverlappedBBs::getHexMetadataFromIns(insi, "INS_directCall");
                directCallTargets[target] = true;
            }
        }
    }

    for (auto funci = M.begin(), funcie = M.end();
            funci != funcie;
            ++funci) {
        /* TODO: find another way to skip uninteresting functions */
        if (!std::strstr(funci->getName().data(), "void-tcg-llvm-tb"))
            continue;
        assert(funci->size());
        BasicBlock &b = funci->front();
        pcStart = FixOverlappedBBs::getPCStartOfFunc(funci);
        if (directCallTargets.find(pcStart) != directCallTargets.end()) {
            outs() << "[MarkFuncEntry] mark " << funci->getName() << "\n";
            b.setName("func_entry_point");
            modified = true;
        }
    }

    return modified;
}
