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

#include "BuildFunctions.h"
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
#include <llvm/Support/CommandLine.h>
#include <llvm/LLVMContext.h>

#include <list>

using namespace llvm;

static cl::opt<std::string> OutputFilename("save-funcs", cl::desc("Save functions to file"), cl::value_desc("filename"));

char BuildFunctions::ID = 0;
static RegisterPass<BuildFunctions> X("buildfunctions",
        "Build functions from a set of marked BBs.", false, false);

bool BuildFunctions::runOnModule(Module &M)
{
    std::list<Function *> eraseBBs;
    llvm::Module *newModule = new llvm::Module("Functions",
            llvm::getGlobalContext());
    int cnt = 0;
    had_one_switch = false;

    /* build a map with all the entries */
    for (auto funci = M.begin(), funcie = M.end();
            funci != funcie;
            ++funci) {
        if (!std::strstr(funci->getName().data(), "void-tcg-llvm-tb"))
            continue;
        uint64_t pcStart;
        pcStart = FixOverlappedBBs::getPCStartOfFunc(funci);
        allBBs[pcStart] = funci;
    }

    llvm::ValueToValueMapTy valueMap;
    /* search for entry points */
    for (auto funci = M.begin(), funcie = M.end();
            funci != funcie;
            ++funci) {
        if (!std::strstr(funci->getName().data(), "void-tcg-llvm-tb"))
            continue;
        if (!funci->size()) {
            /* function already moved (or removed) */
            continue;
        }
        assert(funci->size());
        BasicBlock &b = funci->front();

        if (b.getName() == "func_entry_point") {
            llvm::Constant *c = newModule->
                getOrInsertFunction("final-func-"+std::string(funci->getName()),
                        funci->getFunctionType());
            llvm::Function *newFunc = cast<llvm::Function>(c);
            makeFunctionForward(newModule, newFunc, funci, eraseBBs);
            ++cnt;
        }
    }

    //outs() << "new-module:\n" << *newModule << "\n";
    //outs() << "----END-NEW\n";
    //outs() << "old-module" << M << "\n";
    //outs() << "----END----\n";

    /* some fixup */
    for (auto funci = newModule->begin(), funce = newModule->end();
            funci != funce;
            ++funci) {
        if (funci->size() == 0)
            continue;
        BasicBlock &entry = funci->getEntryBlock();
        if (entry.getName() != "func_entry_point")
            continue;

        /* XXX this is an ugly fix for when we have a jump to the
         * entry basic block. The current fix just adds a BB (called
         * fixup_entry) which jumps to the original entry. We also have
         * to copy the metadata
         */
        if (!entry.hasNUses(0)) {
            outs() << "[BuildFunctionsV] more uses for entry BB " <<
                funci->getName() << "\n";
            //    funci->getName() << "\n" << *funci << "\n";
            //errs() << "[BuildFunctionsV] more uses for entry BB" <<
            //    funci->getName() << "\n" << *funci << "\n";
            BasicBlock *fixupBB = BasicBlock::Create(funci->getContext(),
                    "fixup_entry", funci, &entry);
            SmallVector< std::pair< unsigned, MDNode *>, 20> md;
            /* copy metadata */
            bool gotMetadata = false;
            if (!entry.empty()) {
                if (entry.front().hasMetadata()) {
                    entry.front().getAllMetadata(md);
                    gotMetadata = true;
                }
            }

            Instruction *ins =
                BranchInst::Create(&entry, fixupBB);
            if (gotMetadata) {
                for (auto ms = md.begin(), me = md.end();
                        ms != me;
                        ++ms) {
                    ins->setMetadata(ms->first, ms->second);
                }
            }
            //errs() << "[BuildFunctionsVF] fixup_entry " <<
            //    funci->getName() << "\n" << *funci << "\n";
        }
        //assert(entry.hasNUses(0));
    }

    outs() << "[BuildFunctions] saving " << cnt << " functions to " <<
        OutputFilename << "\n";

    std::string error;
    llvm::raw_fd_ostream bitcodeOstream(
            OutputFilename.c_str(),
            error, 0);
    llvm::WriteBitcodeToFile(newModule,
            bitcodeOstream);
    bitcodeOstream.close();

    /* cleanup unused functions */
    for (auto funci = eraseBBs.begin(), funce = eraseBBs.end();
            funci != funce;
            ++funci) {
        if ((*funci)->getParent())
            (*funci)->eraseFromParent();
    }

    if (eraseBBs.size() > 0) {
        return true;
    }
    return false;
}

void
BuildFunctions::makeFunctionForward(Module *newModule,
        Function *newFunc,
        Function *startBB,
        std::list<llvm::Function *> &toBeRemoved)
{
    std::list<llvm::Function *> BBsForThisFunction;
    bool hasIndirectJump;
    BasicBlock *fakeBB;

    hasIndirectJump = exploreBB(startBB, BBsForThisFunction);
    fakeBB = inlineBBs(newFunc, BBsForThisFunction, hasIndirectJump);
    outs() << "[BuildFunctions] function " << newFunc->getName() << " has " <<
        BBsForThisFunction.size() << " basic blocks (funcs)\n";

    assert(!hasIndirectJump || (hasIndirectJump && fakeBB));

    std::map<uint64_t, llvm::BasicBlock *> pcToBB;
    buildBBMap(newFunc, pcToBB);

    fixupCalls(newFunc, pcToBB);
    buildBBMap(newFunc, pcToBB);

    fixupDirectJumps(newFunc, pcToBB);
    buildBBMap(newFunc, pcToBB);

    fixupIndirectJumps(newFunc, pcToBB, fakeBB);
    buildBBMap(newFunc, pcToBB);

    fixupReturns(newFunc, pcToBB);
    buildBBMap(newFunc, pcToBB);

    outs() << "[BuildFunctions] function " << newFunc->getName() <<
        " done\n";

    /* schedule visited functions for removal */
    for (auto funci = BBsForThisFunction.begin(), funcie = BBsForThisFunction.end();
            funci != funci;
            ++funci) {
        toBeRemoved.push_back(*funci);
    }
}

void
BuildFunctions::copyGlobalReferences(
        llvm::Module *module,
        llvm::Function *func,
        llvm::ValueToValueMapTy &valueMap)
{
    for (Function::iterator ibb = func->begin(), ibe = func->end();
            ibb != ibe; ++ibb) {
        for (BasicBlock::iterator iib = ibb->begin(), iie = ibb->end();
                iib != iie; ++iib) {
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
}

void
BuildFunctions::buildBBMap(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap)
{
    bbMap.clear();
    for (auto bbi = newFunc->begin(), bbie = newFunc->end();
            bbi != bbie;
            ++bbi) {
        assert(bbi->begin() != bbi->end());

        if (bbi->getName() != "fake_indirect_bb") {
            if (!bbi->begin()->getMetadata("BB_pcStart")) {
                //errs() << "[BuildFunctions] no such metadata " << *bbi->begin() << "\n";
            } else {
                uint64_t pcStart = FixOverlappedBBs::getBBStart(bbi->begin());
                if (bbMap.find(pcStart) != bbMap.end()) {
                    //errs() << "[BuildFunctions] ==========\n";
                    //outs() << "[BuildFunctions] for pc " <<
                    //    FixOverlappedBBs::hex(pcStart) <<
                    //    " we found two basic blocks\n";
                    //errs() << "[BuildFunctions] for pc " <<
                    //    FixOverlappedBBs::hex(pcStart) <<
                    //    " we found two basic blocks\n";
                    //errs() << *bbMap[pcStart] << "\n-------\n";
                    //errs() << *bbi << "\n-----\n";
                    //errs() << "[BuildFunctions] ==========\n";
                    /*
                     * we can ignore this case, as we can safely assume
                     * that there is no binary code that jumps in the
                     * middle of a basic block. This split is llvm
                     * specific.
                     */
                } else {
                    //assert(bbMap.find(pcStart) == bbMap.end());
                    bbMap[pcStart] = bbi;
                }
            }
        }
    }
}

bool
BuildFunctions::exploreBB(llvm::Function *startBB,
            std::list<llvm::Function *> &BBsForThisFunction)
{
    bool hasIndirectJump = false;
    std::list<llvm::Function *> BBsToBeExpanded;
    std::map<llvm::Function *, bool> exploredBBs;

    BBsForThisFunction.clear();

    BBsForThisFunction.push_back(startBB);
    BBsToBeExpanded.push_back(startBB);
    exploredBBs[startBB] = true;

    while (!BBsToBeExpanded.empty()) {
        Function *f = BBsToBeExpanded.front();
        BBsToBeExpanded.pop_front();
        outs() << "[BuildFunctions]\texploring " << f->getName() << "\n";

        for (auto bbi = f->begin(), bbie = f->end();
                bbi != bbie;
                ++bbi) {
            for (auto insi = bbi->begin(), inse = bbi->end();
                    insi != inse;
                    ++insi) {
                StoreInst *storeInst = dyn_cast<StoreInst>(insi);
                if (!storeInst)
                    continue;

                GlobalVariable *gv = dyn_cast<GlobalVariable>(storeInst->getOperand(1));
                if (!(gv && gv->getName() == "PC"))
                    continue;

                /* get next PC */
                uint64_t nextPC;
                std::list<uint64_t> nextPCList;
                bool isNextPCValid = false;

                if (storeInst->getMetadata("INS_directCall") ||
                        storeInst->getMetadata("INS_indirectCall")) {
                    assert(storeInst->getMetadata("INS_callReturn"));
                    uint64_t nextPC =
                        FixOverlappedBBs::getHexMetadataFromIns(insi,
                                "INS_callReturn");
                    nextPCList.push_back(nextPC);
                    isNextPCValid = true;
                } else if (storeInst->getMetadata("INS_directJump")) {
                    uint64_t nextPC =
                        FixOverlappedBBs::getHexMetadataFromIns(insi,
                                "INS_directJump");
                    nextPCList.push_back(nextPC);
                    isNextPCValid = true;
                } else if (storeInst->getMetadata("INS_indirectJump")) {
                    if (storeInst->getMetadata("INS_switch_cnt")) {
                        /* we have this indirect jump solved */
                        uint64_t cnt_entries =
                            FixOverlappedBBs::getHexMetadataFromIns(storeInst,
                                    "INS_switch_cnt");
                        /* populate the list with pcs */
                        for (int i = 0; i < cnt_entries; ++i) {
                            char buf[512];
                            snprintf(buf, sizeof buf, "INS_switch_case%d", i);
                            assert(storeInst->getMetadata(std::string(buf)));
                            uint64_t target_pc =
                                FixOverlappedBBs::getHexMetadataFromIns(storeInst,
                                        std::string(buf));
                            nextPCList.push_back(target_pc);
                        }
                        uint64_t default_pc =
                            FixOverlappedBBs::getHexMetadataFromIns(storeInst,
                                    "INS_switch_default");
                        nextPCList.push_back(default_pc);
                        isNextPCValid = true;
                    }
                    hasIndirectJump = true;
                } else {
                    assert(storeInst->getMetadata("INS_return"));
                }


                if (isNextPCValid) {
                    for (auto pci = nextPCList.begin(), pcie = nextPCList.end();
                            pci != pcie; ++pci) {
                        uint64_t nextPC = *pci;
                        if (allBBs.find(nextPC) == allBBs.end()) {
                            errs() << "[BuildFunctions] call to " <<
                                FixOverlappedBBs::hex(nextPC) <<
                                " not found. Ins: " <<
                                *storeInst << "\n";
                            continue;
                        }
                        assert(allBBs.find(nextPC) != allBBs.end());
                        Function *nextBB = allBBs[nextPC];
                        assert(nextBB);
                        if (exploredBBs.find(nextBB) == exploredBBs.end()) {
                            /* a new bb */
                            BBsToBeExpanded.push_back(nextBB);
                            BBsForThisFunction.push_back(nextBB);
                            exploredBBs[nextBB] = true;
                        }
                    }
                }
            }
        }
    }
    return hasIndirectJump;
}

BasicBlock *
BuildFunctions::inlineBBs(llvm::Function *newFunc,
        std::list<llvm::Function *> &BBsForThisFunction,
        bool hasIndirectJump)
{
    llvm::ValueToValueMapTy valueMap;
    for (auto funci = BBsForThisFunction.begin(), funce = BBsForThisFunction.end();
            funci != funce;
            ++funci) {
        llvm::SmallVector<llvm::ReturnInst *, 5> tempList;
        copyGlobalReferences(newFunc->getParent(), *funci, valueMap);
        llvm::CloneFunctionInto(newFunc, *funci, valueMap, true, tempList);
    }

    if (hasIndirectJump) {
        /* create a fake bb for solving the indirect jumps */
        BasicBlock *fakeBB = BasicBlock::Create(newFunc->getContext(),
                "fake_indirect_bb", newFunc);
        Instruction *ins =
            ReturnInst::Create(newFunc->getContext(), fakeBB);
        return fakeBB;
    }
    return NULL;
}

void
BuildFunctions::fixupCalls(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap)
{
    std::map<BasicBlock *, BasicBlock *> bbToTargetBB;
    std::list<BasicBlock *> bbToUpdate;

    for (auto bbi = newFunc->begin(), bbie = newFunc->end();
            bbi != bbie;
            ++bbi) {
        for (auto insi = bbi->begin(), inse = bbi->end();
                insi != inse;
                ++insi) {
            if (insi->getMetadata("INS_directCall") ||
                    insi->getMetadata("INS_indirectCall")) {
                assert(insi->getMetadata("INS_callReturn"));

                uint64_t nextPC =
                    FixOverlappedBBs::getHexMetadataFromIns(insi,
                            "INS_callReturn");
                BasicBlock *bbTarget = bbMap[nextPC];
                if (!bbTarget) {
                    insi->setMetadata("INS_unresolvedCall",
                            MDNode::get(insi->getContext(),
                                MDString::get(insi->getContext(),
                                    std::string("true"))));
                    continue;
                }
                //if (!bbTarget)
                    //bbTarget = getOrCreateEmptyFunction(newFunc->getParent());
                //assert(bbTarget);

                assert(bbToTargetBB.find(bbi) == bbToTargetBB.end());
                bbToTargetBB[bbi] = bbTarget;
                bbToUpdate.push_back(bbi);
            }
        }
    }

    for (auto bbi = bbToUpdate.begin(), bbie = bbToUpdate.end();
            bbi != bbie;
            ++bbi) {
        BasicBlock *bbTarget = bbToTargetBB[*bbi];
        assert(bbTarget);
        BranchInst *bi = BranchInst::Create(bbTarget, *bbi);
    }

    cleanupFakeReturns(bbToUpdate);

    outs() << "[BuildFunctions]\t\tupdated " << bbToUpdate.size() <<
        " {direct,indirect}Calls\n";
}

void
BuildFunctions::fixupDirectJumps(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap)
{
    std::map<Instruction *, BasicBlock *> insToTargetBB;
    std::list<Instruction *> insToUpdate;
    std::list<BasicBlock *> bbToUpdate;

    for (auto bbi = newFunc->begin(), bbie = newFunc->end();
            bbi != bbie;
            ++bbi) {
        for (auto insi = bbi->begin(), inse = bbi->end();
                insi != inse;
                ++insi) {
            if (insi->getMetadata("INS_directJump")) {
                uint64_t nextPC =
                    FixOverlappedBBs::getHexMetadataFromIns(insi,
                            "INS_directJump");
                if (bbMap.find(nextPC) == bbMap.end()) {
                    errs() << "[BuildFunctions] unable to resolve directJump"
                        << *insi << " " << FixOverlappedBBs::hex(nextPC) << "\n";
                    continue;
                }
                BasicBlock *bbTarget = bbMap[nextPC];
                assert(bbTarget);

                assert(insToTargetBB.find(insi) == insToTargetBB.end());
                insToTargetBB[insi] = bbTarget;

                insToUpdate.push_back(insi);
                bbToUpdate.push_back(bbi);
            }
        }
    }

    for (auto insi = insToUpdate.begin(), insie = insToUpdate.end();
            insi != insie;
            ++insi) {
        BasicBlock *bbTarget = insToTargetBB[*insi];
        assert(bbTarget);

        /* do not use replace inst, we really need to insert at the end
         * of BB
         */
        Instruction *bi = BranchInst::Create(bbTarget, (*insi)->getParent());
        (*insi)->eraseFromParent();
        //ReplaceInstWithInst(*insi, bi);
    }

    cleanupFakeReturns(bbToUpdate);

    outs() << "[BuildFunctions]\t\tupdated " << insToUpdate.size() <<
        " directJumps\n";
}

void BuildFunctions::fixupIndirectJumps(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap,
            llvm::BasicBlock *fakeBB)
{
    std::list<Instruction *> insToUpdate;
    std::list<Instruction *> transformToSwitch;
    std::list<BasicBlock *> bbToUpdate;

    for (auto bbi = newFunc->begin(), bbie = newFunc->end();
            bbi != bbie;
            ++bbi) {
        for (auto insi = bbi->begin(), inse = bbi->end();
                insi != inse;
                ++insi) {
            if (insi->getMetadata("INS_indirectJump")) {
                if (insi->getMetadata("INS_switch_cnt")) {
                    transformToSwitch.push_back(insi);
                } else {
                    insToUpdate.push_back(insi);
                }
                bbToUpdate.push_back(bbi);
            }
        }
    }
    BasicBlock *myfakeBB = fakeBB;

    for (auto insi = transformToSwitch.begin(), insie = transformToSwitch.end();
            insi != insie;
            ++insi) {
        BasicBlock *def = NULL;
        if (!(*insi)->getMetadata("INS_switch_default") ||
                !(*insi)->getMetadata("INS_switch_cnt") ||
                !(*insi)->getMetadata("INS_switch_idx_start")) {
            /* skip this, it is borken */
            errs() << "[BuildFunctionsSW]\tswitch wrong metadata\n";
            insToUpdate.push_back(*insi);
            continue;
        }
        uint64_t default_pc =
            FixOverlappedBBs::getHexMetadataFromIns(*insi,
                    "INS_switch_default");
        uint64_t cnt_entries =
            FixOverlappedBBs::getHexMetadataFromIns(*insi,
                    "INS_switch_cnt");
        uint64_t idx_start =
            FixOverlappedBBs::getHexMetadataFromIns(*insi,
                    "INS_switch_idx_start");
        BasicBlock *bb_default = NULL;
        outs() << "[BuildFunctionsSW]\tcreating switch\n";
        /* insi is a store op to PC */
        Value *op = getValueSwitchedOn(*insi);
        if (!op)
            continue;
        if (bbMap.find(default_pc) == bbMap.end()) {
            /* cannot find default bb */
            outs() << "[BuildFunctionsSW]\tcannot find default bb " <<
                FixOverlappedBBs::hex(default_pc) << "\n";
            /**/
            if (myfakeBB == NULL) {
                myfakeBB = BasicBlock::Create(newFunc->getContext(),
                        "fake_switch", newFunc);
                Instruction *fake_ret =
                    ReturnInst::Create(newFunc->getContext(), myfakeBB);
            }
            bb_default = myfakeBB;
        } else {
            bb_default = bbMap[default_pc];
        }

        /* creating swith insn */
        SwitchInst *sw = SwitchInst::Create(op, bb_default, cnt_entries,
                (*insi)->getParent());
        /* add cases */
        for (int i = 0; i < cnt_entries; ++i) {
            uint64_t value = idx_start+i;
            IntegerType *valueType;
            char buf[512];
            snprintf(buf, sizeof buf, "INS_switch_case%d", i);
            assert((*insi)->getMetadata(std::string(buf)));
            uint64_t target_pc =
                FixOverlappedBBs::getHexMetadataFromIns(*insi,
                        std::string(buf));

            /* compute target bb */
            BasicBlock *bb_target = NULL;
            if (bbMap.find(target_pc) == bbMap.end()) {
                /* cannot find the target bb */
                if (myfakeBB == NULL) {
                    myfakeBB = BasicBlock::Create(newFunc->getContext(),
                            "fake_switch_case", newFunc);
                    Instruction *fake_ret =
                        ReturnInst::Create(newFunc->getContext(), myfakeBB);
                }
                bb_target = myfakeBB;
                outs() << "[BuildFunctionsSW]\tfailed to find switch case bb: " <<
                    i << " " << FixOverlappedBBs::hex(target_pc) << "\n";
            } else {
                bb_target = bbMap[target_pc];
            }

            /* compute on value */
            valueType = llvm::Type::getInt32Ty((*insi)->getContext());
            //valueType = op->getType();
            ConstantInt *onValue = ConstantInt::get(valueType, value, false);

            /* really add the case */
            sw->addCase(onValue, bb_target);
        }
        outs() << "[BuildFunctionsSW]\tadded " << cnt_entries << " cases\n";
        had_one_switch = true;
    }

    for (auto insi = insToUpdate.begin(), insie = insToUpdate.end();
            insi != insie;
            ++insi) {
        Instruction *bi = BranchInst::Create(fakeBB, (*insi)->getParent());
        //(*insi)->eraseFromParent();
    }

    cleanupFakeReturns(bbToUpdate);

    outs() << "[BuildFunctions]\t\tupdated " << insToUpdate.size() <<
        " indirectJumps\n";
}

BasicBlock *
BuildFunctions::getOrCreateEmptyFunction(Module *m)
{
    if (m_fakeFunction)
        return &m_fakeFunction->getEntryBlock();
    LLVMContext &ctx = m->getContext();
    llvm::FunctionType *voidFunctionType =
        FunctionType::get(llvm::Type::getVoidTy(ctx), false);
    m_fakeFunction = dyn_cast<Function>
        (m->getOrInsertFunction("fakeFunction", voidFunctionType));
    BasicBlock *entryBB = BasicBlock::Create(ctx, "entry", m_fakeFunction);
    ReturnInst::Create(ctx, entryBB);
    return entryBB;
}

void BuildFunctions::fixupReturns(llvm::Function *newFunc,
            std::map<uint64_t, llvm::BasicBlock *> &bbMap)
{
    std::list<Instruction *> insToUpdate;
    std::list<BasicBlock *> bbToUpdate;

    for (auto bbi = newFunc->begin(), bbie = newFunc->end();
            bbi != bbie;
            ++bbi) {
        for (auto insi = bbi->begin(), inse = bbi->end();
                insi != inse;
                ++insi) {
            if (insi->getMetadata("INS_return")) {
                insToUpdate.push_back(insi);
                bbToUpdate.push_back(bbi);
            }
        }
    }

    for (auto insi = insToUpdate.begin(), insie = insToUpdate.end();
            insi != insie;
            ++insi) {
        /* do not use replace inst, we really need to insert at the end
         * of BB
         */
        Instruction *bi = ReturnInst::Create((*insi)->getContext(),
                (*insi)->getParent());
        (*insi)->eraseFromParent();
        //ReplaceInstWithInst(*insi, bi);
    }
    cleanupFakeReturns(bbToUpdate);

    outs() << "[BuildFunctions]\t\tupdated " << insToUpdate.size() <<
        " returns\n";
}

void
BuildFunctions::cleanupFakeReturns(std::list<llvm::BasicBlock *> &bbToUpdate)
{
    std::list<Instruction *> removeMe;
    /* remove fake returns */
    for (auto bbi = bbToUpdate.begin(), bbie = bbToUpdate.end();
            bbi != bbie;
            ++bbi) {
        for (auto insi = (*bbi)->begin(), inse = (*bbi)->end();
                insi != inse;
                ++insi) {
            if (insi->getMetadata("INS_fakeReturn")) {
                removeMe.push_back(insi);
            }
        }
    }
    for (auto insi = removeMe.begin(), inse = removeMe.end();
            insi != inse;
            ++insi) {
        (*insi)->eraseFromParent();
    }
}

llvm::Value *
BuildFunctions::getValueSwitchedOn(llvm::Instruction *storeToPC)
{
    Value *ret = __getValueSwitchedOn(storeToPC->getOperand(0),
            storeToPC->getParent());
    if (!ret) {
        outs() << "[BuildFunctionsSW]\t\tfailed to get idx op\n";
    }
    return ret;
}

llvm::Value *
BuildFunctions::__getValueSwitchedOn(Value *v, BasicBlock *origBB)
{
    Instruction *ins = dyn_cast<Instruction>(v);
    if (!ins)
        return NULL;

    /* limit the searchonly to the current BB */
    if (ins->getParent() != origBB)
        return NULL;

    /* match shl i32 %23, 2 (or 1) */
    if (ins->getOpcode() == Instruction::Shl) {
        ConstantInt *w = dyn_cast<ConstantInt>(ins->getOperand(1));
        if (w->getZExtValue() == 1 ||
                w->getZExtValue() == 2) {
            /* we got it */
            outs() << "[BuildFunctionsSW]\t\tmatched idx, op0 from: " <<
                *ins << "\n";
            return ins->getOperand(0);
        }
    }

    for (int i = 0; i < ins->getNumOperands(); ++i) {
        Value *ret = __getValueSwitchedOn(ins->getOperand(i), origBB);
        if (ret)
            return ret;
    }
    return NULL;
}
