Feature: Check if the rename was done

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
		Given llvm file of "final.bc"
		Then the out file "final.ll" should contain "Function_0x00000000"
		Then the out file "final.ll" should contain "Function_0x0000001c"
		Then the out file "final.ll" should contain "Function_0x000000a4"
