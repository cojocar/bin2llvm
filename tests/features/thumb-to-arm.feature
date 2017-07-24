Feature: Check transition from thumb mode to ARM mode (C)

	Background:
		Given the binary "./thumb-to-arm/arm-thumb-arm.c.bin" of type "raw-arm-le"
		#Given the binary "./thumb-to-arm/thumb-to-arm.armle.S.bin" of type "raw-arm-le"
		#Given the binary "./thumb-to-arm/thumb-to-arm.armle.S.elf" of type "elf"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist
		# ARM
		Then the out file "qemu.log" should contain "0x00000018:  e52db004      push	{fp}"
		Then the out file "qemu.log" should contain "pop	{fp, pc}"
		# Thumb
		Then the out file "qemu.log" should contain "b580       push	{r7, lr}"
		Then the out file "qemu.log" should contain "f7ff eff2  blx	0x18"
		Then the out file "qemu.log" should contain "bd80       pop	{r7, pc}"
		Then the output should contain "(3 functions)"
