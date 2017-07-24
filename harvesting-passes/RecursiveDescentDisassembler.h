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

#ifndef S2E_PLUGINS_LLVM_TRANSLATOR_RECURSIVE_DESCENT_DISASSEMBLER_H
#define S2E_PLUGINS_LLVM_TRANSLATOR_RECURSIVE_DESCENT_DISASSEMBLER_H

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

#include <cajun/json/reader.h>
#include <cajun/json/writer.h>

#include "JumpTableInfo.h"

#include <s2e/Plugins/bin2llvm/ExtractPossibleTargetsPass.h>
#include <s2e/Plugins/bin2llvm/S2ETransformPass.h>
#include <s2e/Plugins/bin2llvm/S2EInlineHelpersPass.h>
#ifdef TARGET_ARM
#include <s2e/Plugins/bin2llvm/ARMGetThumbBit.h>
#endif

#include <map> //TODO: Exchange map with unordered_map to boost efficiency, currently too complicated as we are not using c++11

#include <llvm/PassManager.h>

namespace s2e {
namespace plugins {

class MyTranslationBasicBlock
{
    public:
        MyTranslationBasicBlock(uint64_t pcStart,
                uint64_t pcEnd,
                llvm::Function *bbFunction) :
            m_pcStart(pcStart), m_pcEnd(pcEnd), m_bbFunction(bbFunction) {
                assert(m_bbFunction != NULL);
                assert(m_pcStart < m_pcEnd);
            }
public:
    uint64_t m_pcStart, m_pcEnd;
public:
    llvm::Function *m_bbFunction;
};

class RecursiveDescentDisassembler : public Plugin
{
    S2E_PLUGIN
public:
    RecursiveDescentDisassembler(S2E* s2e): Plugin(s2e) {}
    ~RecursiveDescentDisassembler();

    void initialize();
    void initializePasses();
    void slotTranslateBlockStart(ExecutionSignal*, S2EExecutionState *state, 
        TranslationBlock *tb, uint64_t pc);
    void slotExecuteBlockStart(S2EExecutionState* state, uint64_t pc);
    void slotStateKill(S2EExecutionState* state);

private:
    bool m_firstTranslation;
    llvm::Module *mainModule;
    ExtractPossibleTargetsPass m_extractPossibleTargetsPass;
    S2ETransformPass m_transformPass;
    S2EInlineHelpersPass m_inlineHelpers;
#ifdef TARGET_ARM
    ARMGetThumbBit m_ARMGetThumbBitPass;
#endif
    llvm::FunctionPassManager* m_functionPassManager;
    //TODO: Exchange map with unordered_map to boost efficiency, currently too complicated as we are not using c++11
    bool m_verbose;
    std::list< std::pair< uint64_t, uint64_t > > m_allowedPcRanges;

    std::map< uint64_t, std::pair< uint64_t, std::vector< uint64_t > > > m_controlFlowGraph;

    std::vector< std::pair< uint64_t, uint64_t> > getConstantMemoryRanges();
    bool isValidCodeAccess(S2EExecutionState* state, uint64_t addr);

    /* on ARM this holds the real PC, the thumb bit is stored
     * sepparately
     */
    std::map<uint64_t, bool> m_visitedPC;

    std::vector< std::pair<uint64_t, uint64_t> > m_visitedPCIntervals;
    bool isPCInVisitedIntervalsBsearch(uint64_t);
    bool isPCInVisitedIntervalsLinear(uint64_t);

    std::map<uint64_t, bool> m_scheduledPCsMap;
    std::vector<uint64_t> m_scheduledPCsVector;

    /* schedule a PC for later exploration, the scheduled PC does not
     * contain the thumb bit, it is a real PC
     */
    bool explorePCLater(uint64_t);
    /* return true if we have more to explore */
    bool moreToExplore();
    /* return the next real PC, this will NOT contain the thumb bit
     */
    uint64_t getNextRealPC();
    bool prepareStateForNextRealPC(S2EExecutionState *state, uint64_t realPC);

    /* information regarding the jump table, this is optional
     * it is used only for exploration
     */
    bool hasJumpTableInfo;
    std::list<JumpTableInfo *> jptInfo;
    /* hashtable with pc_start of bb, for fast matching */
    std::map<uint64_t, JumpTableInfo *> mapJumpTableInfo;

#ifdef TARGET_ARM
#define REAL_PC(a) ((a) & -2)
#else
#define REAL_PC(a) (a)
#endif

#ifdef TARGET_ARM
    std::map<uint64_t, bool> m_isPCThumb;
    std::ofstream *m_isPCThumbOutStream;
#endif

    void exit();
    std::vector<MyTranslationBasicBlock *>m_allBasicBlocks;
};

} // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_LLVM_TRANSLATOR_RECURSIVE_DESCENT_DISASSEMBLER_H
