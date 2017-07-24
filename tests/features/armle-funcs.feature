Feature: Check linking for functions (multiple functions)

	Background:
		Given the binary "./armle-func/multiple-func.armle.c.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		Then an out file named "final.bc" should exist
		Then an out file named "final-linked.bc" should exist
		Then an out file named "qemu-0.log" should exist
		#Then the output should contain "[linky] we got 3 direct calls"
		Then the output should contain "(3 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final-linked.bc"
		Then the out file "final-linked.ll" should contain "call void @linked-final-func-final-func-void-tcg-llvm-tb-"
		Then the out file "final-linked.ll" should contain "define void @linked-final-func-final-func-void-tcg-llvm-tb-"
