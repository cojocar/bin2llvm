# vim: set tabstop=4 expandtab shiftwidth=4:

.global _start
.text

.arm
#_start: @0x0
_start:
    add r0, r1, r2
    b after_do_not_explore
do_not_explore:
    add r6, r6, r6
after_do_not_explore:
    add r8, r8, r8
    bx lr
