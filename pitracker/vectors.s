.globl _start
_start:
    ;@ enable fpu
    mrc p15, 0, r0, c1, c0, 2
    orr r0,r0,#0x300000 ;@ single precision
    orr r0,r0,#0xC00000 ;@ double precision
    mcr p15, 0, r0, c1, c0, 2
    mov r0,#0x40000000
    fmxr fpexc,r0

    mov sp,#0x8000
    bl notmain
hang: b hang

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

.globl dummy
dummy:
    bx lr

.globl bss_start
bss_start: .word __bss_start__
.globl bss_end
bss_end: .word __bss_end__


;@vcvt.s32.f32 s2,s0
;@ftosis s2,s0
;@ftosizs s2,s0
