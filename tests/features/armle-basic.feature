Feature: Check a basic translation for ARM little-endian architecture

	Background:
		Given the binary "./armle-basic/add-ins.armle.S.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check if files were generated
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist
		Then the out file "qemu-0.log" should contain:
		"""
		0x00000000:  e0821003      add	r1, r2, r3
		"""

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Then the out file "final.ll" should contain "%0 = load i32* @R3"
		Then the out file "final.ll" should contain "%1 = load i32* @R2"
		Then the out file "final.ll" should contain "%tmp10_v = add i32 %1, %0"
		Then the out file "final.ll" should contain "store i32 %tmp10_v, i32* @R1"
