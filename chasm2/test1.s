#define X 7
#define REG pc
#define REG13 r13
#define PROC 0x11
.text 0x200
mov REG, 5
mov r0, X
ldr r5, [REG13, PROC]
.label end
