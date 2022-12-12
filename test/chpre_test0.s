#define psh str r0, [r13, 4]!
#define pop ldr r0, [r13], 4
// .file 1 "figisa_4.c"
.label main
  sbi r13, r13, #52   // carve stack
  psh(r5)
  str r5, [r13, 4]!
  str r0, [r13, 4]!
  str r12, [r13, #32] // save fp on stack
  mov r12, r13        // Establish fp
  str r6, [r13, #28]  // save r6 on stack
  str r7, [r13, #24]  // save r7 on stack
  str r10, [r13, #20] // save r10 on stack
  pop(r5)
  psh gusty pop
  psh psh psh
