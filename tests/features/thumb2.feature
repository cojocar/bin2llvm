Feature: Check that thumb2 is working

	Background:
		Given the binary "./thumb2/multiple-func.armle.c.elf" of type "elf"
		When translator runs with random output directory

	Scenario: Check correctness of qemu dissas
		Given unified "qemu-*.log" in "qemu.log"
		Then the out file "qemu.log" should contain "0x00000000:  b580       push	{r7, lr}"
		Then the out file "qemu.log" should contain "0x00000006:  f000 f803  bl	0x10"
		Then the out file "qemu.log" should contain "0x0000004c:  f000 f806  bl	0x5c"
		Then the out file "qemu.log" should contain "0x0000006a:  d903       bls.n	0x74"
		Then the output should contain "(3 functions)"

	Scenario: Check if final.ll is correct
		Given llvm file of "final.bc"
		Then the out file "final.ll" should contain "@R7"
		Then the out file "final.ll" should contain "Function_0x00000010"
		Then the out file "final.ll" should contain "Function_0x0000005c"
		Then the out file "final.ll" should contain "Function_0x00000000"
		Then the out file "final.ll" should contain "@R3"
