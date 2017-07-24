Feature: Check a basic translation for ARM little-endian architecture and function recovery (fall through)

	Background:
		Given the binary "./armle-basic/fall-through.armle.S.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check if files were generated
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist
		Then the out file "qemu-0.log" should contain "add	r0, r1, r2"
		Then the out file "qemu-0.log" should contain "add	r8, r8, r8"
		Then the out file "qemu-0.log" should not contain "add	r6, r6, r6"
		Then the output should contain "(1 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Then the out file "final.ll" should not contain "@R6"
		Then the out file "final.ll" should contain "br label"
