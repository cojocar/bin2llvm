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

#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

MDNode *getBlockMeta(BasicBlock *bb, StringRef kind) {
    assert(!bb->empty() && "BB is empty");
    return bb->getTerminator()->getMetadata(kind);
}

MDNode *getBlockMeta(Function *f, StringRef kind) {
    assert(!f->empty() && "function is empty");
    return getBlockMeta(&f->getEntryBlock(), kind);
}

void setBlockMeta(BasicBlock *bb, StringRef kind, MDNode *md) {
    assert(!bb->empty() && "BB is empty");
    bb->getTerminator()->setMetadata(kind, md);
}

void setBlockMeta(Function *f, StringRef kind, MDNode *md) {
    assert(!f->empty() && "function is empty");
    setBlockMeta(&f->getEntryBlock(), kind, md);
}

bool hasOperand(MDNode *md, Value *operand) {
    fori(i, 0, md->getNumOperands()) {
        if (md->getOperand(i) == operand)
            return true;
    }
    return false;
}

MDNode *removeOperand(MDNode *md, unsigned index) {
    std::vector<Value*> operands;
    fori(i, 0, md->getNumOperands()) {
        if (i != index)
            operands.push_back(md->getOperand(i));
    }
    return MDNode::get(md->getContext(), operands);
}

MDNode *removeNullOperands(MDNode *md) {
    std::vector<Value*> operands;
    fori(i, 0, md->getNumOperands()) {
        if (md->getOperand(i) != NULL)
            operands.push_back(md->getOperand(i));
    }
    return operands.size() == 0 ? NULL : MDNode::get(md->getContext(), operands);
}

bool getBlockSuccs(BasicBlock *bb, std::vector<BasicBlock*> &succs) {
    MDNode *md = getBlockMeta(bb, "succs");
    if (!md)
        return false;

    succs.clear();
    fori(i, 0, md->getNumOperands()) {
        BlockAddress *op = cast<BlockAddress>(md->getOperand(i));
        succs.push_back(op->getBasicBlock());
    }

    return true;
}

void setBlockSuccs(BasicBlock *bb, const std::vector<BasicBlock*> &succs) {
    std::vector<Value*> operands;
    for (BasicBlock *succ : succs) {
        operands.push_back(BlockAddress::get(succ->getParent(), succ));
    }
    setBlockMeta(bb, "succs", MDNode::get(bb->getContext(), operands));
}

bool getBlockSuccs(Function *f, std::vector<Function*> &succs) {
    MDNode *md = getBlockMeta(f, "succs");
    if (!md)
        return false;

    succs.clear();
    fori(i, 0, md->getNumOperands())
        succs.push_back(cast<Function>(md->getOperand(i)));

    return true;
}

void setBlockSuccs(Function *f, const std::vector<Function*> &succs) {
    setBlockMeta(f, "succs", MDNode::get(f->getContext(), (std::vector<Value*>&)succs));
}
