Feature: Check helper_sub_cc inlining

	Background:
		Given the binary "./helpers/helper_sub_cc.armle.S.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist
		Then an out file named "qemu.log" should exist
		Then the output should contain "(1 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Then the out file "final.ll" should contain "@R0"
		Then the out file "final.ll" should contain "@R1"
		Then the out file "final.ll" should contain "@R2"
		Then the out file "final.ll" should not contain "@helper_sub_cc"
