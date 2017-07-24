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

#include "S2EInlineHelpersPass.h"

#include <llvm/Instructions.h>
#include <llvm/Function.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/raw_ostream.h>


using namespace llvm;

char S2EInlineHelpersPass::ID = 0;
static RegisterPass<S2EInlineHelpersPass> X("s2einlinehelpers", "S2E inline arithmetic helper functions", false, false);

static const char * INLINE_FUNCTIONS[] = {
		"helper_add_setq",
		"helper_add_saturate",
		"helper_sub_saturate",
		"helper_double_saturate",
		"helper_add_usaturate",
		"helper_sub_usaturate",
		"helper_ssat",
		"helper_ssat16",
		"helper_usat",
		"helper_usat16",
		"helper_add_cc",
		"helper_adc_cc",
		"helper_sub_cc",
		"helper_sbc_cc",
		"helper_shl",
		"helper_shr",
		"helper_sar",
		"helper_shl_cc",
		"helper_shr_cc",
		"helper_sar_cc",
		"helper_ror_cc",
		"uadds",
		"uadd",
		"uaddl"};

typedef const char ** StringIterator;


S2EInlineHelpersPass::S2EInlineHelpersPass() : FunctionPass(ID)
{
}

bool S2EInlineHelpersPass::shouldInlineCall(CallInst* call)
{
	if (!call ||
	    !call->getCalledFunction() ||
	    !call->getCalledFunction()->hasName())
	{
		return false;
	}

	//TODO: Performance of comparison sucks
	for (StringIterator nameItr = INLINE_FUNCTIONS, nameEnd = INLINE_FUNCTIONS + (sizeof(INLINE_FUNCTIONS) / sizeof(char *));
	     nameItr != nameEnd;
		 nameItr++)
	{
		if (call->getCalledFunction()->getName() == *nameItr)  {
			return true;
		}
	}

	return false;
}

bool S2EInlineHelpersPass::runOnFunction(Function &f) {
	bool inlinedFunctions = false;

	while (true)
	{
		std::list<CallInst*> functionsToInline;
		for (Function::iterator bbItr = f.begin(), bbEnd = f.end(); bbItr != bbEnd; bbItr++)  {
			for (BasicBlock::iterator instItr = bbItr->begin(), instEnd = bbItr->end(); instItr != instEnd; instItr++)  {
				CallInst* call = dyn_cast<CallInst>(instItr);
				if (call && shouldInlineCall(call))  {
					functionsToInline.push_back(call);
				}
			}
		}

		if (functionsToInline.empty())  {
			break;
		}

		InlineFunctionInfo inlineInfo;
		for (std::list<CallInst*>::iterator itr = functionsToInline.begin(), end = functionsToInline.end(); itr != end; itr++)  {
			bool success = InlineFunction(*itr, inlineInfo);
			if (!success)  {
				errs() << "[S2EInlineHelpers] ERROR inlining function " << (*itr)->getCalledFunction()->getName()
						<< " into basic block " << (*itr)->getParent()->getName() << " of function "
						<< (*itr)->getParent()->getParent() << '\n';
				break;
			}
			else  {
				inlinedFunctions = true;
			}
		}
	}

	return inlinedFunctions;
}
