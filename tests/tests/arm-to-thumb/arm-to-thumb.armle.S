# vim: set tabstop=4 expandtab shiftwidth=4:

.global _start
.text

.arm
#_start: @0x0
_start:
    add r1, r2, r3
    adr r0, _main_thumb + 1
    bx r0
.thumb
_main_thumb:
    add r3, r4, r5
    b .
