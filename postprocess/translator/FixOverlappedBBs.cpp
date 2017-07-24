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

#include "FixOverlappedBBs.h"
#include "PcUtils.h"
#include "MetaUtils.h"

#include <llvm/Bitcode/ReaderWriter.h>
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

#include <list>
#include <map>
#include <string>
#include <cstdint>

using namespace llvm;

char FixOverlappedBBs::ID = 0;
static RegisterPass<FixOverlappedBBs> X("fixoverlappedbbs",
        "Fix overlapped basic blocks", false, false);

bool
FixOverlappedBBs::myCompareStartOfFunc(llvm::Function *a, llvm::Function *b)
{
    uint64_t pcA = getPCStartOfFunc(a);
    uint64_t pcB = getPCStartOfFunc(b);

    return pcA < pcB;
}

void
FixOverlappedBBs::truncateFuncAndLinkWith(llvm::Function *a, llvm::Function *b)
{
    uint64_t pcA = getPCStartOfFunc(a);
    uint64_t pcB = getPCStartOfFunc(b);
    assert(pcA < pcB);
    std::list<BasicBlock *> eraseBBs;
    std::list<Instruction *> eraseInsns;
    bool found = false;

    Function::iterator deleteAfterBB, overlapBB;
    BasicBlock::iterator deleteAfterIns;
    bool hasPrev = false;

    /* align */
    for (auto bbi = a->begin(), bbe = a->end();
            !found && bbi != bbe;
            ++bbi) {
        hasPrev = false;
        for (auto insi = bbi->begin(), inse = bbi->end();
                !found && insi != inse;
                ++insi) {
            uint64_t pc = getCurrentPCOfIns(insi);
            if (pc == pcB) {
                /* found the allignment */
                outs() << "[FixOverlappedBBs] bb " << a->getName() << " and " <<
                    b->getName() << " are aligned at " << hex(pc) << "\n";
                    //<< ": " << *insi << "\n";
                found = true;
                deleteAfterBB = bbi;
                deleteAfterIns = insi;
            }
        }
    }
    assert(found);
    overlapBB = deleteAfterBB;

    ++deleteAfterIns;

    BasicBlock *deadTail = overlapBB->splitBasicBlock(deleteAfterIns);

    overlapBB->getTerminator()->eraseFromParent();

    linkWith(a, overlapBB, pcB);

    DeleteDeadBlock(deadTail);
}

void
FixOverlappedBBs::linkWith(llvm::Function *srcFunc, llvm::BasicBlock
        *srcBB, uint64_t target)
{
    LLVMContext &ctx = srcFunc->getContext();
    Value *PC = srcFunc->getParent()->getOrInsertGlobal("PC",
            IntegerType::get(ctx, 32));
    Value *valuePC = ConstantInt::get(IntegerType::get(ctx, 32),
            target, false);

    /* create store to pc */
    StoreInst *storeToPC = new StoreInst(valuePC, PC, srcBB);
    storeToPC->setMetadata("INS_splitCall",
            MDNode::get(ctx,
                MDString::get(ctx,
                    "true")));

    /* create return void */
    ReturnInst *ret = ReturnInst::Create(ctx, srcBB);
    ret->setMetadata("INS_fakeReturn", MDNode::get(ctx,
                MDString::get(ctx,
                    std::string("true"))));
    srcFunc->setName(srcFunc->getName() +
            std::string("-split-to-")+hex(target));
}

bool
FixOverlappedBBs::runOnModule(Module &M)
{
    std::map<uint64_t, std::list<Function *> > endings;
    std::list<uint64_t> overlaps;

    /* find the overlaps */
    for (auto ifunc = M.begin(), ifunce = M.end();
            ifunc != ifunce;
            ++ifunc) {
        /* TODO: find another way to skip uninteresting functions */
        if (!std::strstr(ifunc->getName().data(), "void-tcg-llvm-tb"))
            continue;

        //uint64_t pcEnd = getPCEndOfFunc(ifunc);
        unsigned pcEnd = getLastPc(&ifunc->getEntryBlock());
        endings[pcEnd].push_back(ifunc);
        if (endings[pcEnd].size() == 2) {
            overlaps.push_back(pcEnd);
        }
    }

    /* sort functions by the PC of the first instruction.
     * We need this for truncateFuncAndLinkWith
     */
    for (auto o = overlaps.begin(), oe = overlaps.end();
            o != oe;
            ++o) {
        auto &funcList = endings[*o];
        int cnt = funcList.size();
        assert(cnt >= 2);
        funcList.sort(myCompareStartOfFunc);
        if (cnt >= 3) {
            outs() << "[FixOverlappedBBs] found more the twice (" << cnt << ") the overlap " << hex(*o) << "\n";
            ///for (auto funci = endings[*o].begin(), funcie = endings[*o].end();
            ///        funci != funcie;
            ///        ++funci) {
            ///    outs() << "[FixOverlappedBBs] F " << **funci << "\n";
            ///}
        }
    }

    for (auto o = overlaps.begin(), oe = overlaps.end();
            o != oe;
            ++o) {
        auto funcList = endings[*o];
        int cnt = funcList.size();
        assert(cnt >= 2);
        auto curr = funcList.begin();
        auto next = funcList.begin();
        auto end = funcList.end();
        ++next;
        do {
            truncateFuncAndLinkWith(*curr, *next);
            ++curr;
            ++next;
        } while (next != end);
    }

    return (overlaps.size() > 0);
}

uint64_t
FixOverlappedBBs::getHexMetadataFromFunc(
        llvm::Function *func,
        std::string metadataName)
{
    assert(!func->empty());
    BasicBlock &b = func->getEntryBlock();

    assert(!b.empty());
    Instruction &ins = b.front();

    return getHexMetadataFromIns(&ins, metadataName);
}

uint64_t
FixOverlappedBBs::getHexMetadataFromIns(
        llvm::Instruction *ins,
        std::string metadataName)
{
    MDNode *pcEndNode = ins->getMetadata(metadataName);
    assert(pcEndNode);

    Value *v = pcEndNode->getOperand(0);
    assert(v);

    MDString *s = dyn_cast<MDString>(v);
    assert(s);
    const char *hexString = s->getString().data();
    //outs() << s->getString().data() << "\n";
    uint64_t pc = std::strtoll(hexString, NULL, 16);
    return pc;
}

uint64_t
FixOverlappedBBs::getPCEndOfFunc(Function *func)
{
    return getHexMetadataFromFunc(func, "BB_pcEnd");
}

uint64_t
FixOverlappedBBs::getPCStartOfFunc(Function *func)
{
    return getHexMetadataFromFunc(func, "BB_pcStart");
}

uint64_t
FixOverlappedBBs::getCurrentPCOfIns(llvm::Instruction *ins)
{
    return getHexMetadataFromIns(ins, "INS_currPC");
}

uint64_t
FixOverlappedBBs::getBBStart(llvm::Instruction *ins)
{
    return getHexMetadataFromIns(ins, "BB_pcStart");
}

std::string
FixOverlappedBBs::hex(uint64_t val)
{
    /* TODO: ugh */
    char s[128];
    sprintf(s, "0x%08lx", val);
    return std::string(s);
}

uint64_t
FixOverlappedBBs::getNumberOfASMInstructions(llvm::Function *func)
{
    std::map<uint64_t, bool> visited;
    for (auto bbi = func->begin(), bbe = func->end();
            bbi != bbe;
            ++bbi) {
        for (auto insi = bbi->begin(), insie = bbi->end();
                insi != insie;
                ++insi) {
            if (insi->getMetadata("INS_currPC")) {
                visited[getCurrentPCOfIns(insi)] = true;
            }
        }
    }
    return visited.size();
}
