ADD_DEFINITIONS(-std=c++11 -g)

add_llvm_loadable_module(translator
	S2EARMMergePcThumbPass.cpp
	TransformBBToVoid.cpp
	RemoveExtraStoreToPC.cpp
	S2EDeleteInstructionCount.cpp
	FixOverlappedBBs.cpp
	ARMMarkCall.cpp
	ARMMarkReturn.cpp
	MarkFuncEntry.cpp
	BuildFunctions.cpp
	ARMMarkJumps.cpp
	RemoveBranchTrace.cpp
	ReplaceConstantLoads.cpp
	SolveIndirectSingle.cpp
	ARMDumpThumbBit.cpp
	FunctionRename.cpp
	JumpTableInfo.cpp
	TagInstPc.cpp
	PassUtils.cpp
	MetaUtils.cpp
	PcUtils.cpp
	InternalizeGlobals.cpp
	LoadViaGlobalAliasReplace.cpp
	)


