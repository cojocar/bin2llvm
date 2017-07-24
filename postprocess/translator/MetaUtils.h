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

#ifndef _METAUTILS_H
#define _METAUTILS_H

using namespace llvm;

#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>

MDNode *getBlockMeta(BasicBlock *bb, StringRef kind);
MDNode *getBlockMeta(Function *f, StringRef kind);
void setBlockMeta(BasicBlock *bb, StringRef kind, MDNode *md);
void setBlockMeta(Function *f, StringRef kind, MDNode *md);

template<typename block_t>
static void setBlockMeta(block_t *bb, StringRef kind, uint32_t i) {
    LLVMContext &ctx = bb->getContext();
    setBlockMeta(bb, kind, MDNode::get(ctx,
                ConstantInt::get(Type::getInt32Ty(ctx), i)));
}

bool hasOperand(MDNode *md, Value *operand);
MDNode *removeOperand(MDNode *md, unsigned index);
MDNode *removeNullOperands(MDNode *md);

bool getBlockSuccs(BasicBlock *bb, std::vector<BasicBlock*> &succs);
void setBlockSuccs(BasicBlock *bb, const std::vector<BasicBlock*> &succs);
bool getBlockSuccs(Function *f, std::vector<Function*> &succs);
void setBlockSuccs(Function *f, const std::vector<Function*> &succs);

#endif  // _METAUTILS_H
