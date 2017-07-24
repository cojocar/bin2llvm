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

#include "SolveIndirectSingle.h"
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

using namespace llvm;

/*
 * Input: optimised code (-gdb -basicaa) 
 * Output: INS_computedValue metainfo is added to each store instruction
 * that could be solved to a single value. TODO: should we expand for
 * load instructions and for other operands?
 *
 * opt -solveindirectsingle -optfile opt-func-0.bc -optfile
 * 	opt-remainig-0.bc funcs-0.bc -o
 *  	funcs-0-indirect.bc
 */
char SolveIndirectSingle::ID = 0;
static RegisterPass<SolveIndirectSingle> X("solveindirectsingle",
        "Solve some indirect stores (PC and Thumb for example).", false, false);

static cl::list<std::string> OptFiles(
        "optfile",
        cl::desc("path-to-opt-bc"));

static cl::opt<std::string> SolvedFile(
        "solvedfile",
        cl::desc("At this path, the newly computed targets are saved."
            "(stores to PC)."));

void
SolveIndirectSingle::initialize()
{
    for (auto optfni = OptFiles.begin(), optfnie = OptFiles.end();
            optfni != optfnie;
            ++optfni) {
        SMDiagnostic error;
        Module *m = ParseIRFile(*optfni, error,
                getGlobalContext());
        if (m) {
            expandDirectStoresWithModule(m);
        } else {
            errs() << "[SolveIndirectSingle] unable to load file " <<
                *optfni << "\n";
        }
    }

    /* initialise the behavior */
    m_performReplaceOnTheseGlobals[std::string("PC")] = true;
    m_performReplaceOnTheseGlobals[std::string("thumb")] = true;

    m_performAnnotationOnTheseGlobals[std::string("R0")] = true;
    m_performAnnotationOnTheseGlobals[std::string("PC")] = true;
    m_performAnnotationOnTheseGlobals[std::string("thumb")] = true;

    this->m_solvedPCsOutStream = new
        std::ofstream(SolvedFile.c_str(), std::ios::out |
                std::ios::binary);
    assert(this->m_solvedPCsOutStream);
}

bool
SolveIndirectSingle::performAnnotations(Function *function)
{
    bool changed = false;
    llvm::LLVMContext &ctx = function->getContext();

    for (auto bbi = function->begin(), bbie = function->end();
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
            if (!gv)
                continue;
            ConstantInt *val =
                dyn_cast<ConstantInt>(storeInst->getValueOperand());
            if (val) {
                /* we do not care about constant store */
                continue;
            }
            bool annotate =
                m_performAnnotationOnTheseGlobals.find(gv->getName()) !=
                m_performAnnotationOnTheseGlobals.end();
            bool replace =
                m_performReplaceOnTheseGlobals.find(gv->getName()) !=
                m_performReplaceOnTheseGlobals.end();
            if (!annotate && !replace) {
                /* black listed */
                continue;
            }

            uint64_t pc = FixOverlappedBBs::getCurrentPCOfIns(insi);
            IndirectValueAtPC *valueMap = m_directStores[gv->getName()];

            if (!valueMap || valueMap->find(pc) == valueMap->end()) {
                //outs() << "[SolveIndirectSingle] unable to solve " <<
                //    *insi << "\n";
                continue;
            }
            ConstantInt *replaceWith = (*valueMap)[pc];
            assert(replaceWith);

            outs() << "[SolveIndirectSingle] solved " <<
                *insi << " pc: " << FixOverlappedBBs::hex(pc) <<
                "->" << *replaceWith <<"\n";

            if (std::string("PC") == gv->getName())
                    m_solvedPCs.push_back(replaceWith->getZExtValue());

            if (annotate) {
                insi->setMetadata("INS_computedValue",
                        MDNode::get(ctx, MDString::get(ctx,
                                FixOverlappedBBs::hex(
                                    replaceWith->getZExtValue()))));
            }
            if (replace) {
                Value *value = storeInst->getValueOperand();
                value->replaceAllUsesWith(replaceWith);
            }

            changed = true;
        }
    }

    return changed;
}

void
SolveIndirectSingle::expandDirectStoresWithModule(Module *module)
{
    /* search for the pattern, direct store to global and pupulate
     * solvedIndirects
     *
     * XXX retain ConstantValue here, as we need the width.
     */
    for (auto funci = module->begin(), funcie = module->end();
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
                if (!gv)
                    continue;
                ConstantInt *val =
                    dyn_cast<ConstantInt>(storeInst->getValueOperand());
                if (!val)
                    continue;
                /* insert the new value in directStores */
                IndirectValueAtPC *valueMap = m_directStores[gv->getName()];

                if (valueMap == NULL) {
                    valueMap = new IndirectValueAtPC();
                    m_directStores[gv->getName()] = valueMap;
                    outs() << "[SolveIndirectSingle] " <<
                        "allocating new map for global " << gv->getName() << "\n";
                }
                assert(valueMap);
                uint64_t pc = FixOverlappedBBs::getCurrentPCOfIns(insi);
                (*valueMap)[pc] = val;
            }
        }
    }
}

SolveIndirectSingle::~SolveIndirectSingle()
{
    outs() << "[SolveIndirectSingle] destructor\n";

    int cnt = m_solvedPCs.size();
    *m_solvedPCsOutStream << "[\n";
    for (auto p = m_solvedPCs.begin(), pe = m_solvedPCs.end();
            p != pe;
            ++p, --cnt) {
        *m_solvedPCsOutStream << *p;
        if (cnt > 1)
            *m_solvedPCsOutStream << ',';
    }
    *m_solvedPCsOutStream << "]\n";
    m_solvedPCsOutStream->close();
    delete m_solvedPCsOutStream;
    /* TODO: free m_directStores */
}

bool
SolveIndirectSingle::runOnFunction(llvm::Function &f)
{
    return performAnnotations(&f);
}
