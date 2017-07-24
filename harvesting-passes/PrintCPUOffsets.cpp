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

/*
 * Helper plugin for printing cpu offsets in a header file that can be
 * later used by the external llvm passes
 */
#include "PrintCPUOffsets.h"

#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/S2EExecutor.h>
#include <s2e/S2EExecutionState.h>

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(PrintCPUOffsets, "Print CPU offsets from CPUArch", "PrintCPUOffsets",);

void PrintCPUOffsets::printOneCPUOffset(int offset, std::string name)
{
    char offs[32];
    sprintf(offs, "0x%08x", offset);
    s2e()->getDebugStream() << "    {" << offs << ", " << '"' <<
        name << "\"},\n";
}

#if defined(TARGET_ARM)
void PrintCPUOffsets::initializeARM()
{
    printOneCPUOffset(CPU_OFFSET(thumb), "thumb");
    printOneCPUOffset(CPU_OFFSET(ZF), "ZF");
    printOneCPUOffset(CPU_OFFSET(CF), "CF");
    printOneCPUOffset(CPU_OFFSET(VF), "VF");
    printOneCPUOffset(CPU_OFFSET(NF), "NF");
    printOneCPUOffset(CPU_OFFSET(regs[15]), "PC");
    printOneCPUOffset(CPU_OFFSET(regs[14]), "LR");
    printOneCPUOffset(CPU_OFFSET(regs[13]), "SP");
    printOneCPUOffset(CPU_OFFSET(regs[12]), "R12");
    printOneCPUOffset(CPU_OFFSET(regs[11]), "R11");
    printOneCPUOffset(CPU_OFFSET(regs[10]), "R10");
    printOneCPUOffset(CPU_OFFSET(regs[9]), "R9");
    printOneCPUOffset(CPU_OFFSET(regs[8]), "R8");
    printOneCPUOffset(CPU_OFFSET(regs[7]), "R7");
    printOneCPUOffset(CPU_OFFSET(regs[6]), "R6");
    printOneCPUOffset(CPU_OFFSET(regs[5]), "R5");
    printOneCPUOffset(CPU_OFFSET(regs[4]), "R4");
    printOneCPUOffset(CPU_OFFSET(regs[3]), "R3");
    printOneCPUOffset(CPU_OFFSET(regs[2]), "R2");
    printOneCPUOffset(CPU_OFFSET(regs[1]), "R1");
    printOneCPUOffset(CPU_OFFSET(regs[0]), "R0");
    printOneCPUOffset(CPU_OFFSET(s2e_icount), "s2e_icount");
    printOneCPUOffset(CPU_OFFSET(current_tb), "current_tb");
    printOneCPUOffset(CPU_OFFSET(s2e_current_tb), "s2e_icount");
}
#endif

#if defined(TARGET_X86)
void PrintCPUOffsets::initializeX86()
{

    printOneCPUOffset(CPU_OFFSET(cr[0]), "CR0");
    printOneCPUOffset(CPU_OFFSET(s2e_icount), "s2e_icount");
    printOneCPUOffset(CPU_OFFSET(current_tb), "current_tb");
    printOneCPUOffset(CPU_OFFSET(s2e_current_tb), "s2e_icount");
    printOneCPUOffset(CPU_OFFSET(segs[0].base), "Seg0.base");
    printOneCPUOffset(CPU_OFFSET(segs[1].base), "Seg1.base");
    printOneCPUOffset(CPU_OFFSET(segs[2].base), "Seg2.base");
    printOneCPUOffset(CPU_OFFSET(segs[3].base), "Seg3.base");
    printOneCPUOffset(CPU_OFFSET(segs[4].base), "Seg4.base");
    printOneCPUOffset(CPU_OFFSET(segs[5].base), "Seg5.base");
    printOneCPUOffset(CPU_OFFSET(cc_src), "cc_src");
    printOneCPUOffset(CPU_OFFSET(cc_dst), "cc_dst");
    printOneCPUOffset(CPU_OFFSET(cc_tmp), "cc_tmp");
    printOneCPUOffset(CPU_OFFSET(cc_op), "cc_op");
    printOneCPUOffset(CPU_OFFSET(df), "df");
}
#endif


void PrintCPUOffsets::initialize()
{
    s2e()->getDebugStream() << "------cut-here-----X8----\n";
    s2e()->getDebugStream() << "struct offset_name {\n";
    s2e()->getDebugStream() << "    int offset; char *name;\n";
    s2e()->getDebugStream() << "};\n";
    s2e()->getDebugStream() << "struct offset_name cpu_offset_names[] = {\n";
#if defined(TARGET_ARM)
    initializeARM();
#elif defined(TARGET_X86)
    initializeX86();
#endif
    s2e()->getDebugStream() << "};\n";
    s2e()->getDebugStream() << "------cut-here-----X8----\n";
}

}
}
