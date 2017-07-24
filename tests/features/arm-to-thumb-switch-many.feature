Feature: Check transition from thumb mode to ARM mode (ASM)

	Background:
		Given the binary "./arm-to-thumb/arm-to-thumb-many.armle.S.bin" of type "raw-arm-le"
		#Given the binary "./arm-to-thumb/arm-to-thumb-many.armle.S.elf" of type "elf"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		# arm
		Then the out file "qemu.log" should contain "e12fff10      bx	r0"
		Then the out file "qemu.log" should contain "add	r1, r2, r3"
		Then the out file "qemu.log" should contain "bx	r0"
		Then the out file "qemu.log" should contain "add	r3, r4, r5"
		Then the out file "qemu.log" should contain "add	r2, r3, r3"
		# Thumb
		Then the out file "qemu.log" should contain "4700       bx	r0"
		Then the out file "qemu.log" should contain "adds	r6, r6, r6"
		Then the output should contain "(2 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Then the out file "final.ll" should not contain "@R9"
		Then the out file "final.ll" should contain "@R0"
		Then the out file "final.ll" should contain "@R1"
		Then the out file "final.ll" should contain "@R2"
		Then the out file "final.ll" should contain "@R3"
		Then the out file "final.ll" should contain "@R6"
		# add in ARM
		Then the out file "final.ll" should contain "%0 = load i32* @R5,"
		Then the out file "final.ll" should contain "%1 = load i32* @R4,"
		Then the out file "final.ll" should contain "add i32 %1, %0"

