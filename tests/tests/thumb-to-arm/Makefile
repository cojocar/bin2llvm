include ../../Makefile.arm.common
include ../../Makefile.common

build: c_src asm_src 
	
c_src: arm.c thumb.c
	$(CROSS_CC) $(CFLAGS) -marm -march=armv7-r arm.c -mthumb-interwork -c -o arm.o
	$(CROSS_CC) $(CFLAGS) -mthumb -march=armv7-r thumb.c -mthumb-interwork -c -o thumb.o
	$(CROSS_CC) $(LDFLAGS) arm.o thumb.o -o arm-thumb-arm.c.elf
	$(OBJCOPY) -S -j.text -O binary arm-thumb-arm.c.elf arm-thumb-arm.c.bin

asm_src: thumb-to-arm.armle.S.bin
