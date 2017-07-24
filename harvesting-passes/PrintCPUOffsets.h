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

#ifndef S2E_PLUGINS_PRINT_CPU_OFFSET
#define S2E_PLUGINS_PRINT_CPU_OFFSET

#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>

namespace s2e {
namespace plugins {

class PrintCPUOffsets: public Plugin
{
    S2E_PLUGIN
public:
    PrintCPUOffsets(S2E* s2e): Plugin(s2e) {}

    void initialize();

private:
    void printOneCPUOffset(int offset, std::string name);
#if defined(TARGET_ARM)
    void initializeARM();
#elif defined(TARGET_X86)
    void initializeX86();
#endif
};

} // namespace plugins
} // namespace s2e

#endif
