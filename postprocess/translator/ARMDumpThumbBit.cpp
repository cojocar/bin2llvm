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

#include "ARMDumpThumbBit.h"
#include "FixOverlappedBBs.h"

#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Metadata.h>
#include <llvm/BasicBlock.h>
#include <llvm/Support/CFG.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/GraphTraits.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

#include <list>
#include <map>
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>

/*
 * save thumb bit to a file
 * we will look only at the basic blocks that contain a store to thumb
 * and a store to PC
 */
using namespace llvm;
char ARMDumpThumbBit::ID = 0;
static RegisterPass<ARMDumpThumbBit> X("armdumpthumbbit",
        "Solve some indirect stores (PC and Thumb for example).", false, false);

static cl::opt<std::string> OutJson(
        "outjson",
        cl::desc("At this PC, the code should be thumb."));

ConstantInt *
ARMDumpThumbBit::getStoreToPCInBB(BasicBlock *block)
{
    for (auto insi = block->begin(), insie = block->end();
            insi != insie;
            ++insi) {
        StoreInst *storeInst = dyn_cast<StoreInst>(insi);
        if (!storeInst)
            continue;
        if (!storeInst->getMetadata("INS_currPC"))
            continue;
        GlobalVariable *gv = dyn_cast<GlobalVariable>(
                storeInst->getOperand(1));
        if (!gv || std::string("thumb") != gv->getName())
            continue;
        ConstantInt *val =
            dyn_cast<ConstantInt>(storeInst->getValueOperand());
        if (!val)
            continue;
        return val;
    }
    return NULL;
}

bool
ARMDumpThumbBit::runOnModule(Module &module)
{
    for (auto funci = module.begin(), funcie = module.end();
            funci != funcie;
            ++funci) {
        for (auto bbi = funci->begin(), bbie = funci->end();
                bbi != bbie;
                ++bbi) {
            for (auto insi = bbi->begin(), insie = bbi->end();
                    insi != insie;
                    ++insi) {
                StoreInst *storeInst = dyn_cast<StoreInst>(insi);
                if (!storeInst)
                    continue;
                if (!storeInst->getMetadata("INS_currPC"))
                    continue;
                GlobalVariable *gv = dyn_cast<GlobalVariable>(
                        storeInst->getOperand(1));
                if (!gv || std::string("thumb") != gv->getName())
                    continue;
                ConstantInt *val =
                    dyn_cast<ConstantInt>(storeInst->getValueOperand());
                if (!val)
                    continue;
                if (val->getZExtValue() != 1)
                    continue;
                /* we have store 1, @thumb */
                ConstantInt *pcVal = getStoreToPCInBB(bbi);
                if (!pcVal)
                    continue;
                m_thumbPCs.push_back(pcVal->getZExtValue() & -2);
            }
        }
    }
    return false;
}

void
ARMDumpThumbBit::initialize()
{
    this->m_thumbPCOutStream = new
        std::ofstream(OutJson.c_str(), std::ios::out |
                std::ios::binary);
    assert(this->m_thumbPCOutStream);
}

ARMDumpThumbBit::~ARMDumpThumbBit()
{
    *m_thumbPCOutStream << "[\n";
    int cnt = m_thumbPCs.size();
    outs() << "[ARMDumpThumbBit] saving " << cnt << " thumb bits\n";
    for (auto p = m_thumbPCs.begin(), pe = m_thumbPCs.end();
            p != pe;
            ++p, --cnt) {
        *m_thumbPCOutStream << *p;
        if (cnt > 1)
            *m_thumbPCOutStream << ',';
    }
    *m_thumbPCOutStream << "]\n";
    m_thumbPCOutStream->close();
    delete this->m_thumbPCOutStream;
}
