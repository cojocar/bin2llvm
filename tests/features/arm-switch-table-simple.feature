Feature: Check if a switch statemetn is detected and translated

	Background:
		Given the binary "./switch-table/switch-statement.armle.c.elf" of type "elf"
		When translator runs with random output directory

	Scenario: Check if files were generated
		Then an out file named "final.bc" should exist
		Then an out file named "qemu-0.log" should exist

		Given llvm file of "final.bc"
	Scenario: Check if final.ll is correct (has switch statemetn)
		Given llvm file of "final.bc"
		Then the out file "final.ll" should contain "switch"
