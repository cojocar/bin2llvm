include ../../Makefile.arm.common
include ../../Makefile.common

build: multiple-func.armle.c.bin

multiple-func.armle.c.elf: multiple-func.armle.c
	$(CROSS_CC) $(CFLAGS) $(LDFLAGS) -mthumb -march=armv7-r multiple-func.armle.c -mno-thumb-interwork -o multiple-func.armle.c.elf
