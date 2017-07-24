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

#include <llvm/Support/IRReader.h>

#include "PassUtils.h"

using namespace llvm;

namespace logging {
    static raw_null_ostream dummy;

    static const char* const levelStrings[] = {"ERROR", "WARNING", "INFO", "DEBUG"};

    raw_ostream &getStream(logging::Level level) {
        if (level > logLevel)
            return dummy;

        errs() << "[" << levelStrings[level] << "] ";
        return errs();
    }
}

cl::opt<unsigned> debugVerbosity("debug-verbosity",
        cl::desc("Verbosity of generated code (0: default, 1: basic log, 2: register log"),
        cl::value_desc("level"));

cl::opt<std::string> s2eOutDir("s2e-out-dir",
        cl::desc("S2E output directory"),
        cl::value_desc("path"));

cl::opt<logging::Level> logLevel("loglevel",
        cl::desc("Logging level"),
        cl::values(
            clEnumValN(logging::ERROR, "error", "Only show errors"),
            clEnumValN(logging::WARNING, "warning", "Show warnings"),
            clEnumValN(logging::INFO, "info",
                "(default) Show some informative messages about what passes are doing"),
            clEnumValN(logging::DEBUG, "debug",
                "Show extensive debug messages"),
            clEnumValEnd
        ),
        cl::initializer<logging::Level>(logging::INFO));

cl::opt<fallback::Mode> fallbackMode("fallback-mode",
        cl::desc("How to handle edges that are not recorded in successor lists"),
        cl::values(
            clEnumValN(fallback::NONE, "none",
                "Make error block unreachable, allows most optimization"
                " (assumes a single code path)"),
            clEnumValN(fallback::ERROR, "error",
                "Print error message and exit on unknown edge"),
            clEnumValN(fallback::BASIC, "basic",
                "Fallback for any unknown edge"),
            clEnumValN(fallback::EXTENDED, "extended",
                "(default) Basic fallback + jump table for unknown edges with"
                " target blocks that are in the recovered set"),
            clEnumValEnd
        ),
        cl::initializer<fallback::Mode>(fallback::EXTENDED));

cl::list<unsigned> breakAt("break-at",
        cl::desc("Basic block addresses to insert debugging breakpoints at"),
        cl::value_desc("address"));

bool checkif(bool condition, const std::string &message) {
    if (!condition)
        errs() << "Check failed: " << message << '\n';

    return condition;
}

void failUnless(bool condition, const std::string &message) {
    if (!condition) {
        errs() << "Check failed: " << message << '\n';
        exit(1);
    }
}

std::string s2eRoot() {
    const char *envval = getenv("S2EDIR");
    if (!envval) {
        errs() << "missing S2EDIR environment variable\n";
        exit(1);
    }
    return std::string(envval);
}

std::string runlibDir() {
    return s2eRoot() + "/runlib";
}

std::string s2eOutFile(const std::string &basename) {
    if (s2eOutDir.getNumOccurrences() == 0) {
        errs() << "Error: no -s2e-out-dir specified\n";
        exit(1);
    }
    return s2eOutDir + "/" + basename;
}

bool fileOpen(std::ifstream &f, const std::string &path,
        bool failIfMissing) {
    f.open(path);

    if (failIfMissing)
        failUnless(f.is_open(), "could not open " + path);

    return f.is_open();
}

bool s2eOpen(std::ifstream &f, const std::string &basename,
        bool failIfMissing) {
    return fileOpen(f, s2eOutFile(basename), failIfMissing);
}

void doInlineAsm(IRBuilder<> &b, StringRef _asm, StringRef constraints,
        bool hasSideEffects, ArrayRef<Value*> args) {
    Type *argTypes[args.size()];
    ArrayRef<Type*> argTypesRef((Type**)argTypes, args.size());

    fori(i, 0, args.size())
        argTypes[i] = args[i]->getType();

    b.CreateCall(
        InlineAsm::get(
            FunctionType::get(b.getVoidTy(), argTypesRef, false),
            _asm, constraints, hasSideEffects
        ),
        args
    );
}

Module *loadBitcodeFile(StringRef path) {
    SMDiagnostic err;
    Module *from = ParseIRFile(path, err, getGlobalContext());

    if (!from) {
        err.print("could not load bitcode file", errs());
        exit(1);
    }

    return from;
}
