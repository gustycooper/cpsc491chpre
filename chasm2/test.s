#define PROC 0x10
#define X[A] 7
.data 0x100
.label a
5
.label b
X[A]
.label c
25
.text 0x200
ldr r1, c
ldr r0, a
mov lr, 10
add r0, r0, r1
str r0, b
mov r1, X[A]
mov r1, 7
ldr r2, [sp, PROC]
ldr r2, [sp, 0x10]
.label end
PROC X[A]
ldr r2, 45
