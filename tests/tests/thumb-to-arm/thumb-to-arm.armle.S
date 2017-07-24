# vim: set tabstop=4 expandtab shiftwidth=4:

.global _start
.text

.arm
#_start: @0x0
_start: 
    add r1, r2, r3
    adr r0, _main_thumb + 1
    bx r0
    add r5, r5, r5
after:
    add r6, r6, r6


.thumb
_main_thumb:
    push {lr}
    add r3, r4, r5
    ldr r0, _arm_again_ptr
    blx r0
    pop {r1}
    bx r1

_arm_again_ptr:
.word _arm_again

.arm
_arm_again:
    add r6, r6, r6
    bx lr
