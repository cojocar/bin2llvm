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

#ifndef _PASSUTILS_H
#define _PASSUTILS_H

#include <map>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <cassert>
#include <stdlib.h>
#include <stdint.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Metadata.h>
#include <llvm/IRBuilder.h>
#include <llvm/Constants.h>
#include <llvm/Operator.h>
#include <llvm/InlineAsm.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;

#define foreach2(i, b, e) \
    for (__typeof__(b) i = (b), i##_end = (e); i != i##_end; i++)

#define foreach(i, a) \
    foreach2(i, (a).begin(), (a).end())

#define fori(i, lbnd, ubnd) \
    for (unsigned i = (lbnd), i##_ubnd = (ubnd); i < i##_ubnd; i++)

#define foreach_use(value, usename, action) {                  \
        const auto _val = (value);                             \
        foreach2(usename, _val->use_begin(), _val->use_end())  \
            action;                                            \
    }

#define foreach_use_as(value, usetype, usename, action)        \
    foreach_use(value, _iuse, {                                \
        usetype * const usename = cast<usetype>(*_iuse);       \
        action;                                                \
    })

#define foreach_use_if(value, usetype, usename, action)           \
    foreach_use(value, _iuse, {                                   \
        if (usetype * const usename = dyn_cast<usetype>(*_iuse))  \
            action;                                               \
    })

#define ifcast(ty, name, val) if (ty *name = dyn_cast<ty>(val))

#define foreach_pred(bb, name, action) \
    for (pred_iterator _PI = pred_begin(bb), _E = pred_end(bb); _PI != _E; ++_PI) { \
        BasicBlock *name = *_PI; \
        action; \
    }

#define foreach_succ(bb, name, action) \
    for (succ_iterator _SI = succ_begin(bb), _E = succ_end(bb); _SI != _E; ++_SI) { \
        BasicBlock *name = *_SI; \
        action; \
    }

namespace logging {
    enum Level {ERROR = 0, WARNING, INFO, DEBUG};
    raw_ostream &getStream(Level level);
}

namespace fallback {
    typedef enum {NONE = 0, ERROR, BASIC, EXTENDED} Mode;
}

// optimization: the if-statement makes this work like llvm's DEBUG macro: if
// the logging level is lower than `level` the `message` argument will not be
// evaluated, saving execution time (since stringification is expensive)
#define LOG(level, message) {                       \
        if (logging::level <= logLevel)             \
            logging::getStream(logging::level) << message;  \
    }
#define LOG_LINE(level, message) LOG(level, message << '\n')
#define ERROR(message) LOG_LINE(ERROR, message)
#define WARNING(message) LOG_LINE(WARNING, message)
#define INFO(message) LOG_LINE(INFO, message)
#define DBG(message) LOG_LINE(DEBUG, message)

extern cl::opt<unsigned> debugVerbosity;

extern cl::opt<logging::Level> logLevel;

extern cl::opt<std::string> s2eOutDir;

extern cl::opt<fallback::Mode> fallbackMode;

extern cl::list<unsigned> breakAt;

static inline std::string intToHex(unsigned int num) {
    std::ostringstream s;
    s << std::hex << num;
    return s.str();
}

static inline uint32_t hexToInt(StringRef hexStr) {
    return std::strtoul(hexStr.data(), NULL, 16);
}

static inline bool isRecoveredBlock(Function *f) {
    return f->hasName() && f->getName().startswith("Func_");
}

static inline bool isRecoveredBlock(BasicBlock *bb) {
    return bb->hasName() && bb->getName().startswith("BB_");
}

static inline unsigned getBlockAddress(Function *f) {
    return hexToInt(f->getName().substr(5));
}

static inline unsigned getBlockAddress(BasicBlock *bb) {
    return hexToInt(bb->getName().substr(3));
}

bool checkif(bool condition, const std::string &message);

void failUnless(bool condition, const std::string &message = "");

std::string s2eRoot();

std::string runlibDir();

std::string s2eOutFile(const std::string &basename);

bool fileOpen(std::ifstream &f, const std::string &path,
        bool failIfMissing=true);

bool s2eOpen(std::ifstream &f, const std::string &basename,
        bool failIfMissing=true);

void doInlineAsm(IRBuilder<> &b, StringRef _asm, StringRef constraints,
        bool hasSideEffects, ArrayRef<Value*> args);

Module *loadBitcodeFile(StringRef path);

template<typename T>
static bool vectorContains(const std::vector<T> &vec, const T needle) {
    for (const T val : vec) {
        if (val == needle)
            return true;
    }
    return false;
}

#endif  // _PASSUTILS_H
