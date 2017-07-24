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

#include "InternalizeGlobals.h"
#include "PassUtils.h"

using namespace llvm;

char InternalizeGlobals::ID = 0;
static RegisterPass<InternalizeGlobals> X("internalize-globals",
        "S2E Internalize and zero-initialize global variables", false, false);

bool InternalizeGlobals::runOnModule(Module &m) {
    foreach2(g, m.global_begin(), m.global_end()) {
        if (g->hasInitializer()) {
            g->setLinkage(GlobalValue::InternalLinkage);
            continue;
        }

        Type *ty = g->getType()->getElementType();
        Constant *init;

		if (!(g->getName() == "CF" ||
				g->getName() == "ZF" ||
				g->getName() == "VF" ||
				g->getName() == "NF"
				)) {
			// inline only flags
			continue;
		}

		llvm::outs() << "PPP: make CF internal" << "\n";
        if (ty->isPointerTy())
            init = ConstantPointerNull::get(cast<PointerType>(ty));
        else if (ty->isAggregateType()) 
            init = ConstantAggregateZero::get(ty);
        else if (ty->isIntegerTy()) {
			//init = UndefValue::get(ty);
			init = ConstantInt::get(ty, 0);
		}
        else
            failUnless(false, "unsupported global type");

        g->setInitializer(init);
        g->setLinkage(GlobalValue::InternalLinkage);
    }

    return true;
}
