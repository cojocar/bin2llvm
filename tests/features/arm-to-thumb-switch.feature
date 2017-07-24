Feature: Check transition switch from ARM to thumb mode

	Background:
		Given the binary "./arm-to-thumb/arm-to-thumb.armle.S.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas (ARM)
		Then the out file "qemu-0.log" should contain "add	r1, r2, r3"
		Then the out file "qemu-0.log" should contain "add	r0, pc, #1"
		Then the out file "qemu-0.log" should contain "bx	r0"
		# thumb mode
		Then the out file "qemu-0.log" should contain "adds	r3, r4, r5"
		Then the out file "qemu-0.log" should contain "b.n	0xe"
		Then the output should contain "(1 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Then the out file "final.ll" should contain "@R1"
		Then the out file "final.ll" should contain "@R2"
		Then the out file "final.ll" should contain "@R3"
		Then the out file "final.ll" should contain "@R4"
		Then the out file "final.ll" should contain "@R5"
		Then the out file "final.ll" should contain "br label"
