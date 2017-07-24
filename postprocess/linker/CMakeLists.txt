ADD_DEFINITIONS(-std=c++11 -g)

#${LLVM_LIBRARY_DIRS}

add_llvm_executable(linky
	../translator/FixOverlappedBBs.cpp
	../translator/PcUtils.cpp
	../translator/BuildFunctions.cpp
	main.cpp
	)

# wierd linkage
target_link_libraries(linky
	gtest
	gtest_main
	LLVMAnalysis
	LLVMArchive
	LLVMAsmParser
	LLVMAsmPrinter
	LLVMBitReader
	LLVMBitWriter
	LLVMCodeGen
	LLVMCore
	LLVMDebugInfo
	LLVMExecutionEngine
	LLVMInstCombine
	LLVMInstrumentation
	LLVMInterpreter
	LLVMipa
	LLVMipo
	LLVMJIT
	LLVMLinker
	LLVMMC
	LLVMMCDisassembler
	LLVMMCJIT
	LLVMMCParser
	LLVMObject
	LLVMRuntimeDyld
	LLVMScalarOpts
	LLVMSelectionDAG
	LLVMSupport
	LLVMTableGen
	LLVMTarget
	LLVMTransformUtils
	LLVMVectorize
	LLVMX86AsmParser
	LLVMX86AsmPrinter
	LLVMX86CodeGen
	LLVMX86Desc
	LLVMX86Disassembler
	LLVMX86Info
	LLVMX86Utils
	LTO
	profile_rt
	)
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#	    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

