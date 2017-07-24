Feature: Check if __ld{l,b} and __st{l,b} are replaced

	Background:
		Given the binary "./armle-func/one-func.armle.c.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist
		Then an out file named "qemu.log" should exist
		Then the output should contain "(2 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Given llvm file of "final.bcpost_passes.bc"
		Then the out file "final.bcpost_passes.ll" should contain "call i32 @__ldl_mmu"
		Then the out file "final.bcpost_passes.ll" should contain "call void @__stl_mmu"
		Then the out file "final.ll" should contain "@MEMORY"
		Then the out file "final.ll" should not contain "call void @__stl_mmu"
		Then the out file "final.ll" should not contain "call i32 @__ldl_mmu"
