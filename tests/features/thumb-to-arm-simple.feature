Feature: Check transition from thumb mode to ARM mode (C)

	Background:
		Given the binary "./thumb-to-arm/thumb-to-arm.armle.S.bin" of type "raw-arm-le"
		Given the entry point "0x0"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist
		# ARM
		Then the out file "qemu.log" should contain "e0866006      add	r6, r6, r6"
		# Thumb
		Then the out file "qemu.log" should contain "b500       push	{lr}"
		Then the out file "qemu.log" should contain "bc02       pop	{r1}"
		Then the output should contain "(2 functions)"
