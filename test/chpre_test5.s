#define gus(A) ldr A, [r13,  4]! 
#define opal(A, B) ldr A, [B,  4]! 
gus(r0)
opal(r0, r13)
