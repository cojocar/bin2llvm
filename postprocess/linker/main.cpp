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

#include <llvm-c/Core.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

#include <map>
#include <iostream>

#include "../translator/FixOverlappedBBs.h"
#include "../translator/BuildFunctions.h"


using namespace std;
using namespace llvm;

static void
usage(char *argv0)
{
    cerr << "Usage:\t" << argv0 <<
        "file0.bc [file1.bc [file2.bc ...]] out-file.bc" << endl;
    cerr << "\tLink multiple bc files into one" << endl;
}

static void
gatherFuncsFromModule(Module *m,
        map<uint64_t, Function *> &allFuncsMap,
        list<Function *> &allFuncsList)
{
    for (auto funci = m->begin(), funce = m->end();
            funci != funce;
            ++funci) {

        if (!std::strstr(funci->getName().data(), "void-tcg-llvm-tb"))
            continue;

        auto &bb = funci->getEntryBlock();
        auto &ins = bb.front();
        if (!ins.getMetadata("BB_pcStart")) {
            cout << "[linky] skip function (no BB_pcStart) " << funci->getName().data() << "\n";
            continue;
        }

        uint64_t startPC = FixOverlappedBBs::getPCStartOfFunc(funci);

        //cout << "[linky] doing func " << funci->getName().data() <<
        //endl;
        if (allFuncsMap.find(startPC) == allFuncsMap.end()) {
            allFuncsMap[startPC] = funci;
            allFuncsList.push_back(funci);
            //cout << "[linky] new func at " << std::hex << startPC << "\n";
            //cout.unsetf(ios::hex);
        } else {
            //cout << "[linky] already discovered " <<
            //funci->getName().data() << endl;
        }
    }
}

static void
gatherFuncs(int fileCount, char *files[],
        map<uint64_t, Function *> &allFuncsMap,
        list<Function *> &allFuncsList)
{
    for (int i = 0; i < fileCount; ++i) {
        SMDiagnostic error;
        Module *m = ParseIRFile(std::string(files[i]), error,
                getGlobalContext());
        if (m) {
            gatherFuncsFromModule(m, allFuncsMap, allFuncsList);
        } else {
            cout << "[linky] unable to load file " << files[i] << " " <<
                endl;
        }
    }
}

static void
cloneFuncs(Module *newModule,
        list<Function *> &allFuncsList)
{
    ValueToValueMapTy valueMap;
    /* clone all in the new module */
    for (auto funci = allFuncsList.begin(), funce = allFuncsList.end();
            funci != funce;
            ++funci) {
        Constant *c = newModule->
            getOrInsertFunction("linked-final-func-"+std::string((*funci)->getName()),
                    (*funci)->getFunctionType(),
                    (*funci)->getAttributes());
        llvm::Function *newFunc = cast<llvm::Function>(c);
        assert(newFunc);

        SmallVector<llvm::ReturnInst *, 20> tempList;
        //cout << "[linky] clone function " << (*funci)->getName().data() << endl;
        BuildFunctions::copyGlobalReferences(newModule, *funci, valueMap);
        CloneFunctionInto(newFunc, *funci, valueMap, false, tempList);
    }
    cout << "[linky] done cloning\n";
}

static void
linkFuncs(Module *newModule,
        map<uint64_t, Function *> &allFuncsMap,
        list<Function *> &allFuncsList)
{
    list<std::pair<StoreInst *, Function *> > directCalls;

    /* gather direct calls */
    for (auto funci = allFuncsList.begin(), funce = allFuncsList.end();
            funci != funce;
            ++funci) {
        if ((*funci)->begin() == (*funci)->end()) {
            errs() << "[linky]\t[!!] empty func in " << (*funci)->getName().data() << "\n";
        }
        for (auto bbi = (*funci)->begin(), bbe = (*funci)->end();
                bbi != bbe;
                ++bbi) {
            if ((bbi)->begin() == (bbi)->end()) {
                errs() << "[linky\t[!!] empty bb in " <<
                    (*funci)->getName().data() << "\n";
            }
            for (auto insi = bbi->begin(), inse = bbi->end();
                    insi != inse;
                    ++insi) {
                if (isa<StoreInst>(insi)) {
                    StoreInst *storeInst = dyn_cast<StoreInst>(insi);
                    if (storeInst->getMetadata("INS_directCall")) {
                        uint64_t targetPC =
                            FixOverlappedBBs::getHexMetadataFromIns(insi,
                                    "INS_directCall");
                        if (allFuncsMap.find(targetPC) == allFuncsMap.end()) {
                            cout << "[linky] unknown call to " <<
                                FixOverlappedBBs::hex(targetPC) << endl;
                            continue;
                        }
                        assert(allFuncsMap.find(targetPC) != allFuncsMap.end());
                        Function *targetFunc = allFuncsMap[targetPC];
                        directCalls.push_back(std::make_pair(storeInst, targetFunc));
                    }
                }
            }
        }
    }

    cout.unsetf(ios::hex);
    cout << "[linky] we got " << directCalls.size() << " direct calls" << endl;

    /* perform the actual linking */
    for (auto calli = directCalls.begin(), calle = directCalls.end();
            calli != calle;
            ++calli) {
        StoreInst *storeInst = dyn_cast<StoreInst>(calli->first);
        Function *targetFunc = dyn_cast<Function>(calli->second);

        assert(storeInst);
        assert(targetFunc);

        CallInst *c = CallInst::Create(targetFunc);

        ReplaceInstWithInst(storeInst, c);
    }
    cout << "[linky] done linking\n";
}

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        usage(argv[0]);
        exit(-1);
    }

    map<uint64_t, Function *> allFuncsMap;
    list<Function *> allFuncsList;

    cout << "[linky] linking: ";
    for (unsigned i = 1; i < argc-1; ++i) {
        cout << argv[i] << "\n";
    }
    cout << "into " << argv[argc-1] << "\n";
    gatherFuncs(argc-2, argv+1, allFuncsMap, allFuncsList);
    cout.unsetf(ios::hex);
    cout << "[linky] discovered " << allFuncsList.size() << " unique functions" << endl;

    Module *newModule = new llvm::Module("LinkedFunctions",
            llvm::getGlobalContext());
    assert(newModule);
    cloneFuncs(newModule, allFuncsList);

    allFuncsMap.clear();
    allFuncsList.clear();
    gatherFuncsFromModule(newModule, allFuncsMap, allFuncsList);
    linkFuncs(newModule, allFuncsMap, allFuncsList);

    cout << "[linky] saving " << allFuncsList.size() << " functions to " <<
        argv[argc-1] << "\n";

    std::string error;
    llvm::raw_fd_ostream bitcodeOstream(
            argv[argc-1],
            error, 0);
    llvm::WriteBitcodeToFile(newModule,
            bitcodeOstream);
    bitcodeOstream.close();

    llvm::raw_fd_ostream functionCnt(
            (std::string(argv[argc-1])+std::string(".fcnt")).c_str(),
        error, 0);
    functionCnt << allFuncsList.size();
    functionCnt.close();
    return 0;
}
