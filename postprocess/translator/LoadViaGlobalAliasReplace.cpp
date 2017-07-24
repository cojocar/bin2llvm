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

#include "LoadViaGlobalAliasReplace.h"
#include "PassUtils.h"

using namespace llvm;

char LoadAliasReplacePass::ID = 0;
static RegisterPass<LoadAliasReplacePass> X("alias-global-bb",
        "replace load from a global with the use of store", false, false);

bool LoadAliasReplacePass::runOnBasicBlock(BasicBlock &BB)
{
	std::list<Instruction *> eraseLoads;
	std::list< std::pair<LoadInst*, StoreInst *> > aliases;

	bool verbose_ = false;
	if (BB.getParent()->getName() == "Function_0x0001beec" &&
			BB.getName() == "entry")
		verbose_ = true;
#define DEBUG_a(a) if (verbose_) do { a ; } while (0)
	//DEBUG_a(llvm::outs() << "got bb\n");

	auto rit = BB.end();
	do {
		--rit;
		if (rit == BB.begin())
			break;
		LoadInst *loadInst = dyn_cast<LoadInst>(rit);
		if (!loadInst)
			continue;

		GlobalVariable *gvLoad = dyn_cast<GlobalVariable>(loadInst->getOperand(0));
		if (!gvLoad)
			continue;

		//if (gvLoad->getName() == "ZF")
		//	llvm::outs() << "LOAD: " << *loadInst << "\n";

		auto rit_prev = rit;
		do {
			--rit_prev;
			StoreInst *storeInst = dyn_cast<StoreInst>(rit_prev);
			if (!storeInst)
				continue;
			GlobalVariable *gvStore = dyn_cast<GlobalVariable>(storeInst->getOperand(1));
			if (!gvStore)
				continue;
			if (gvLoad == gvStore) {
				DEBUG_a(llvm::outs() << "found alias: " << *storeInst << " " << *loadInst <<"\n");
				//DEBUG_a(llvm::outs() << "QQQ:  " << *storeInst->getOperand(0) <<"\n");
				aliases.push_back(std::make_pair(loadInst, storeInst));
			}
		} while (rit_prev != BB.begin());
		assert(rit_prev != rit);
	} while (rit != BB.begin());

	for (auto a : aliases) {
		a.first->replaceAllUsesWith(a.second->getOperand(0));
	}

	///for (auto a : aliases) {
	///	//a.first->removeFromParent();
	///}

	if (aliases.size() > 0)
		return true;
	return false;
}

