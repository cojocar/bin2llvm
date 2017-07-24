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

#include "RecursiveDescentDisassembler.h"
#include "SaveTranslatedBBs.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>

#include <iostream>
#include <cstdlib>

#include <llvm/Function.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm-c/Core.h>
#include <llvm/Transforms/Utils/Cloning.h>

using klee::ConstantExpr;
using std::map;

extern "C" int is_valid_code_addr(CPUArchState* env1, target_ulong addr);

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(RecursiveDescentDisassembler, "Translates the given program to TCG/LLVM by traversing it with recursive-descent semantics", "Disassembler",);

static inline std::string toHexString(uint64_t pc)
{
    std::stringstream ss;

    ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << pc;
    return ss.str();
}

void RecursiveDescentDisassembler::initialize()
{
    m_verbose = s2e()->getConfig()->getBool(
            getConfigKey() + ".verbose", false);
    std::string initialAlreadyVisited = s2e()->getConfig()->getString(
            getConfigKey() + ".initialAlreadyVisited", "");
#ifdef TARGET_ARM
    std::string isThumbIn = s2e()->getConfig()->getString(
            getConfigKey() + ".isThumbIn", "");
    std::string isThumbOut = s2e()->getConfig()->getString(
            getConfigKey() + ".isThumbOut", "");
#endif
    std::string jumpTableInfoPath = s2e()->getConfig()->getString(
            getConfigKey() + ".jumpTableInfoPath", "");

    bool ok;
    std::vector<std::string> rangesKeys = s2e()->getConfig()->getListKeys(getConfigKey() + ".allowedPcRanges", &ok);
    if (ok)  {
        for (std::vector<std::string>::iterator rangeKey = rangesKeys.begin(), keyEnd = rangesKeys.end(); rangeKey != keyEnd; rangeKey++)
        {
            std::string configKey = getConfigKey() + ".allowedPcRanges." + *rangeKey;
            bool addrOk;
            bool sizeOk;
            uint64_t addr = s2e()->getConfig()->getInt(configKey + ".address", 0, &addrOk);
            uint64_t size = s2e()->getConfig()->getInt(configKey + ".size", 0, &sizeOk);

            if (!addrOk || !sizeOk || addr == size)  {
                s2e()->getWarningsStream() << "[RecursiveDescentDisassembler] Error reading configuration key \""
                    << getConfigKey() + ".pc_ranges." + *rangeKey << "\"" << '\n';
                continue;
            }

            m_allowedPcRanges.push_back(std::make_pair(addr, size));
        }
    }

    if (initialAlreadyVisited != "") {
        std::istream *stream = new
            std::ifstream(initialAlreadyVisited.c_str(), std::ios::in |
                    std::ios::binary);
        assert(stream);

        json::Array root;
        json::Reader::Read(root, *stream);
        json::Array::const_iterator entry(root.Begin()),
            entriesEnd(root.End());
        for (; entry != entriesEnd; ++entry) {
            const json::Array &pcs = *entry;
            assert(pcs.Size() == 2);
            json::Array::const_iterator pcIt(pcs.Begin());

            const json::Number &pc_start_json = *pcIt;
            const json::Number &pc_end_json = *(++pcIt);
            uint64_t pc_start = (uint64_t)static_cast<uint64_t>(pc_start_json);
            uint64_t pc_end = (uint64_t)static_cast<uint64_t>(pc_end_json);

            // optimize, usually we will get a hit in the hash table
            //
            m_visitedPC[pc_start] = true;
            m_visitedPCIntervals.push_back(std::make_pair(pc_start, pc_end));
        }
    }

    if (jumpTableInfoPath != "") {
            this->jptInfo = JumpTableInfoFactory::loadFromFile(jumpTableInfoPath);
            std::list<JumpTableInfo *>::iterator it = jptInfo.begin();
            std::list<JumpTableInfo *>::iterator ite = jptInfo.end();
            for (; it != ite; ++it)
                mapJumpTableInfo[(*it)->bb_start] = *it;
            this->hasJumpTableInfo = true;
    } else {
            this->hasJumpTableInfo = false;
    }


#ifdef TARGET_ARM
    if (isThumbIn != "") {
        std::istream *stream = new
            std::ifstream(isThumbIn.c_str(), std::ios::in |
                    std::ios::binary);
        assert(stream);
        bool failed = false;

        json::Array root;
        try {
            json::Reader::Read(root, *stream);
        } catch (json::Exception e) {
            s2e()->getDebugStream() << "[RecursiveDescentDisassembler] " <<
                "Failed to parse json: " << isThumbIn << "\n";
            failed = true;
        }
        if (!failed) {
            json::Array::const_iterator entry(root.Begin()),
                entriesEnd(root.End());
            for (; entry != entriesEnd; ++entry) {
                const json::Number &jpc = *entry;
                uint64_t pc = (uint64_t)static_cast<double>(jpc);
                assert(0 == (pc & 1));
                m_isPCThumb[pc] = true;
            }
            assert(isThumbOut != "");
        }
    }

    if (isThumbOut != "") {
        this->m_isPCThumbOutStream = new
            std::ofstream(isThumbOut.c_str(), std::ios::out |
                    std::ios::binary);
        assert(this->m_isPCThumbOutStream);
    }
#endif

    s2e()->getCorePlugin()->onTranslateBlockStart.connect(
            sigc::mem_fun(*this, &RecursiveDescentDisassembler::slotTranslateBlockStart));
    s2e()->getCorePlugin()->onStateKill.connect(
            sigc::mem_fun(*this, &RecursiveDescentDisassembler::slotStateKill));

    if (m_verbose)  {
        s2e()->getDebugStream() << "[RecursiveDescentDisassembler] Plugin initialized" << '\n';
    }
    initializePasses();
    m_firstTranslation = true;
}

std::vector< std::pair< uint64_t, uint64_t> > RecursiveDescentDisassembler::getConstantMemoryRanges()
{
    std::vector< std::pair< uint64_t, uint64_t> > ranges;
    bool ok;
    std::vector<std::string> keys =
        s2e()->getConfig()->getListKeys(getConfigKey() +
                ".constantMemoryRanges", &ok);
    if (ok) {
        for (std::vector<std::string>::const_iterator itr = keys.begin(); itr != keys.end(); itr++)  {
            bool addr_ok;
            bool size_ok;
            uint64_t address = s2e()->getConfig()->getInt(getConfigKey()
                    + ".constantMemoryRanges." + *itr + ".address", 0,
                    &addr_ok);
            uint64_t size = s2e()->getConfig()->getInt(getConfigKey() +
                    ".constantMemoryRanges." + *itr + ".size", 0,
                    &size_ok);
            if (!addr_ok || !size_ok)  {
                continue;
            }
            ranges.push_back(std::make_pair(address, size));
        }
    }

    return ranges;
}

bool RecursiveDescentDisassembler::isValidCodeAccess(S2EExecutionState*
        state, uint64_t addr)
{
    return is_valid_code_addr(state->getConcreteCpuState(), addr);
}

void RecursiveDescentDisassembler::slotTranslateBlockStart(ExecutionSignal *signal,
                                      S2EExecutionState *state,
                                      TranslationBlock *tb,
                                      uint64_t pc)
{
    if (m_verbose)  {
        s2e()->getDebugStream() <<
            "[RecursiveDescentDisassembler] Translating block "
            << hexval(pc) << '\n';
    }
    signal->connect(sigc::mem_fun(*this, &RecursiveDescentDisassembler::slotExecuteBlockStart));
}

void RecursiveDescentDisassembler::initializePasses()
{
    /* XXX really add each basic block to the main Module */
    mainModule = new llvm::Module("MyModule", llvm::getGlobalContext());
    m_functionPassManager = new llvm::FunctionPassManager(mainModule);
}

void RecursiveDescentDisassembler::slotExecuteBlockStart(
        S2EExecutionState *state, uint64_t pc)
{
#ifdef TARGET_ARM
    bool translatedInThumbMode =
        state->readCpuState(CPU_OFFSET(thumb), sizeof(uint32_t) * 8);
    bool thisIsForSureThumbMode = translatedInThumbMode;

    if (m_isPCThumb.find(REAL_PC(pc)) != m_isPCThumb.end())
        thisIsForSureThumbMode = m_isPCThumb[REAL_PC(pc)];
    //s2e()->getDebugStream() << "translatedInThumbMode@" << hexval(pc) <<
    //    ": " << translatedInThumbMode << " <--> " << thisIsForSureThumbMode <<
    //    "\n";
    if (translatedInThumbMode != thisIsForSureThumbMode) {
            s2e()->getDebugStream() << "[RecursiveDescentDisassembler] " <<
                "forcing retranslation with mode change " <<
                translatedInThumbMode << " " << thisIsForSureThumbMode
                << " @PC: " << hexval(pc) << "\n";
        state->writeCpuState(CPU_OFFSET(thumb),
                thisIsForSureThumbMode == true, sizeof(uint32_t) * 8);
        throw CpuExitException();
    }
#endif
    llvm::Function* bbFunction = static_cast<llvm::Function*>(state->getTb()->llvm_function);

    m_extractPossibleTargetsPass.runOnFunction(*bbFunction);
    m_inlineHelpers.runOnFunction(*bbFunction);
    m_transformPass.runOnFunction(*bbFunction);
    MyTranslationBasicBlock *newBB = new MyTranslationBasicBlock(pc,
            state->getTb()->llvm_first_pc_after_bb, bbFunction);
    m_allBasicBlocks.push_back(newBB);
    m_visitedPC[pc] = true;
    if (m_firstTranslation) {
        m_firstTranslation = false;
        llvm::BasicBlock &bb = bbFunction->getEntryBlock();
        bb.setName("func_entry_point");
    }
#ifdef TARGET_ARM
    m_ARMGetThumbBitPass.runOnFunction(*bbFunction);
#endif

    //s2e()->getDebugStream() << "Module: " << *mainModule;
    //s2e()->getDebugStream() << "Transformed function: " << *bbFunction << '\n';
#ifdef TARGET_ARM
    ARMGetThumbBit::thumb_bit_t thumbBit = m_ARMGetThumbBitPass.getThumbBit();
    s2e()->getDebugStream() << "\tThumb Bit: " << thumbBit << "\n";
#endif

    std::vector<uint64_t>bbTargets = m_extractPossibleTargetsPass.getTargets();
    if (bbTargets.size() > 0) {
        s2e()->getDebugStream() << "bb@" << hexval(pc) << " generated targets: ";
    }

    /* harvest next PCs from targets as well jump table (if needed) */
    std::vector<uint64_t> allPossiblePCs;
    for (std::vector<uint64_t>::iterator it = bbTargets.begin();
            it != bbTargets.end(); ++it) {
        allPossiblePCs.push_back(*it);
    }
    if (mapJumpTableInfo.find(pc) != mapJumpTableInfo.end()) {
        /* start pc hit */
        JumpTableInfo *info = mapJumpTableInfo[pc];
        /*
        s2e()->getDebugStream() << "\tGot start@" <<
            hexval(info->bb_start) << " " << hexval(info->bb_end) <<
            " " << hexval(state->getTb()->llvm_first_pc_after_bb) << "\n";
            */
        if (info->bb_end == state->getTb()->llvm_first_pc_after_bb) {
            s2e()->getDebugStream() << "\tMatched jumptable@" <<
                hexval(info->indirect_jmp_pc) << "\n";
            int cnt_entries = 1+info->idx_stop-info->idx_start;
            /* XXX this has to go into the json */
            int mul = 4;
            assert(cnt_entries >= 1);
            /* we have to load cnt_entries entries and push them as new
             * PCs
             */
            for (int i = 0; i < cnt_entries; ++i) {
                uint32_t loadedPC = 0x0;
                assert(sizeof (loadedPC) <= mul);
                /* XXX no support fo big endianness */
                state->readMemoryConcrete(info->base_table+mul*i,
                        &loadedPC, mul);
                s2e()->getDebugStream() << "\tload_jpt " <<
                    hexval(loadedPC) << "\n";
                allPossiblePCs.push_back((uint64_t)loadedPC);
            }
            allPossiblePCs.push_back(info->default_case_pc);
        }
    }

    /* schedule them */
    for (std::vector<uint64_t>::iterator it = allPossiblePCs.begin();
            it != allPossiblePCs.end(); ++it) {
        s2e()->getDebugStream() << "@" << hexval(*it) << " " << "\n";
        explorePCLater(REAL_PC(*it));

        if (m_extractPossibleTargetsPass.hasImplicitTarget() &&
                *it == m_extractPossibleTargetsPass.getImplicitTarget()) {
            /* Asumme that implicit targets are the same. */
#ifdef TARGET_ARM
            bool isThumb =
                state->readCpuState(CPU_OFFSET(thumb), sizeof(uint32_t) * 8);
            m_isPCThumb[REAL_PC(*it)] = isThumb;
#endif
            continue;
        }
#ifdef TARGET_ARM
        /* XXX: move this to another function,
         * XXX: thumbBit should be per target
         */
        if (thumbBit == ARMGetThumbBit::THUMB_BIT_SET) {
            m_isPCThumb[REAL_PC(*it)] = true;
            s2e()->getDebugStream() << "\tset thumb_bit for: " <<
                hexval(REAL_PC(*it)) << "\n";
        } else if (thumbBit == ARMGetThumbBit::THUMB_BIT_UNSET) {
            m_isPCThumb[REAL_PC(*it)] = false;
        } else if (thumbBit == ARMGetThumbBit::THUMB_BIT_UNDEFINED) {
            /* get the thumb bit from the current mode */
            bool isThumb =
                state->readCpuState(CPU_OFFSET(thumb), sizeof(uint32_t) * 8);
            m_isPCThumb[REAL_PC(*it)] = isThumb;
            if (isThumb)
                s2e()->getDebugStream() << "\tset thumb_bit for (fall): " <<
                    hexval(REAL_PC(*it)) << "\n";
        }
#endif

    }
    s2e()->getDebugStream() << "\n";

    while (moreToExplore()) {
        uint64_t nextPC = getNextRealPC();
        if (prepareStateForNextRealPC(state, nextPC)) {
            /* we need this to retrigger translation */
            throw CpuExitException();
        } else {
            s2e()->getDebugStream() << "PX @" << hexval(nextPC) <<
                " points outside of the memory\n";
        }
    }

    s2e()->getDebugStream() << "done!\n";
    SaveTranslatedBBs sss;
    //sss.saveTranslatedBasicBlocks(&this->m_allBasicBlocks,
    //        s2e()->getOutputFilename("translated_bbs.txt.ll"));
    llvm::Module *newModule =
        sss.createAndSaveTranslatedBasicBlocksAsAModule(&this->m_allBasicBlocks,
                s2e()->getOutputFilename("translated_bbs.bc"));
    delete newModule;
    this->exit();
}

bool RecursiveDescentDisassembler::prepareStateForNextRealPC(
        S2EExecutionState *state, uint64_t realPC)
{
    /* return true if the pc is valid for access */
    bool ret;
    s2e()->getWarningsStream() <<
        "setting PC to: " << toHexString(realPC);
#if defined(TARGET_ARM)
    assert((realPC & 0x1) == 0x0);
    state->setPc(realPC);
    if (m_isPCThumb[realPC] == true) {
        s2e()->getWarningsStream() <<
            " (thumb)\n";
        state->writeCpuState(CPU_OFFSET(thumb), 1, sizeof(uint32_t) * 8);
    } else {
        s2e()->getWarningsStream() <<
            " (arm)\n";
        state->writeCpuState(CPU_OFFSET(thumb), 0, sizeof(uint32_t) * 8);
    }
#elif defined(TARGET_I386)
    state->setPc(realPC);
    s2e()->getWarningsStream() << " (x86)\n";
#elif define(TARGET_X86_64)
    state->setPc(realPC);
    s2e()->getWarningsStream() << " (x86-64)\n";
#else
#   error "Architecture not supported"
#endif
    ret = isValidCodeAccess(state, realPC);
    return ret;
}

bool RecursiveDescentDisassembler::moreToExplore()
{
    return m_scheduledPCsVector.size() > 0;
}

uint64_t RecursiveDescentDisassembler::getNextRealPC()
{
    assert(moreToExplore());
    uint64_t nextPC = m_scheduledPCsVector.back();
    m_scheduledPCsVector.pop_back();
    /* once extracted, we can reexplore the same pc, so we just
     * unschedule it
     */
    m_scheduledPCsMap[nextPC] = false;
    return nextPC;
}

bool RecursiveDescentDisassembler::explorePCLater(
        uint64_t pc)
{
    if (m_visitedPC.find(pc) != m_visitedPC.end() ||
            m_scheduledPCsMap.find(pc) != m_scheduledPCsMap.end())
        return false;

    if (isPCInVisitedIntervalsBsearch(pc))
        return false;
    m_scheduledPCsVector.push_back(pc);
    m_scheduledPCsMap[pc] = true;
    return true;
}

bool RecursiveDescentDisassembler::isPCInVisitedIntervalsBsearch(
        uint64_t pc)
{
    int a, b;
    a = 0;
    b = m_visitedPCIntervals.size()-1;
    // biggst smaller or eq than pc
    int idx = -1;
    int mid = -1;
    if (a > b)
        return false;
    while (a <= b) {
        int mid = (a+b)/2;
        if (m_visitedPCIntervals[mid].first == pc) {
            break;
        } else if (m_visitedPCIntervals[mid].first > pc) {
            b = mid-1;
        } else {
            a = mid+1;
        }
    }

    idx = mid;

    // adjust left
    while (idx >= 0 && m_visitedPCIntervals[idx].first == pc)
        --idx;

    // linear search while we do not surpass pc
    while (idx < m_visitedPCIntervals.size()) {
        if (m_visitedPCIntervals[idx].first <= pc &&
                pc <= m_visitedPCIntervals[idx].second)
            return true;
        if (m_visitedPCIntervals[idx].first > pc)
            break;
        ++idx;
    }
    return false;
}

bool RecursiveDescentDisassembler::isPCInVisitedIntervalsLinear(
        uint64_t pc)
{
    int idx = 0;
    while (idx < m_visitedPCIntervals.size()) {
        if (m_visitedPCIntervals[idx].first <= pc &&
                pc <= m_visitedPCIntervals[idx].second)
            return true;
        if (m_visitedPCIntervals[idx].first > pc)
            break;
        ++idx;
    }
    return false;
}

void RecursiveDescentDisassembler::exit() {
    s2e()->getDebugStream() << "[RecursiveDescentDisassembler] exiting" << '\n';

#ifdef TARGET_ARM
    if (m_isPCThumbOutStream) {
        json::Array root;
        for (std::map<uint64_t, bool>::iterator i = m_isPCThumb.begin(), ie = m_isPCThumb.end();
                i != ie;
                ++i) {
            if (i->second == true)
                root.Insert(json::Number(i->first));
        }
        s2e()->getDebugStream() << "[RecursiveDescentDisassembler] " <<
            "m_isPCThumb has " << root.Size() << " entries" << '\n';
        json::Writer::Write(root, *m_isPCThumbOutStream);
        m_isPCThumbOutStream->close();
        delete m_isPCThumbOutStream;
    }

#endif

    ::exit(0);
}

RecursiveDescentDisassembler::~RecursiveDescentDisassembler()
{
    s2e()->getDebugStream() <<
        "[RecursiveDescentDisassembler] destructor\n" ;
}

void RecursiveDescentDisassembler::slotStateKill(S2EExecutionState *state)
{
    s2e()->getDebugStream() <<
        "[RecursiveDescentDisassembler] state killed, remaining: " <<
        moreToExplore();
}

} // namespace plugins
} // namespace s2e
