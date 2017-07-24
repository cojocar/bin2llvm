# vim: set tabstop=4 expandtab shiftwidth=4:

.global _start
.text

.arm
_start:
    cmp r0, r1
again:
    cmp r1, r2
    bcc again
    b .
    bx lr
