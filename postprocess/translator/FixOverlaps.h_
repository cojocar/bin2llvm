#ifndef _FIXOVERLAPS_H
#define _FIXOVERLAPS_H

#include "llvm/Pass.h"

using namespace llvm;

struct FixOverlapsPass : public ModulePass {
    static char ID;
    FixOverlapsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _FIXOVERLAPS_H
