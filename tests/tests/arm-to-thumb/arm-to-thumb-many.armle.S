# vim: set tabstop=4 expandtab shiftwidth=4:

.global _start
.text

.arm
#_start: @0x0
_start: 
    add r1, r2, r3
    adr r0, _thumb_first + 1
    bx r0
dead:
    add r9, r9, r9

.thumb
_thumb_first:
    add r6, r6, r6
    ldr r0, =_back_to_arm
    bx r0

.arm
_exit_arm:
    add r2, r3, r3
    b .

_back_to_arm:
    add r3, r4, r5
    adr r0, _exit_arm
    bx r0

