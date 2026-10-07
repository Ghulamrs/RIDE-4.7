# M10 probe: hand-written, in the shape cxx1's x86_64-windows GNU spelling emits
# (push rbp; mov rbp,rsp; sub; locals at -k(%rbp)), with a CodeView line table
# (.cv_* directives) and hand-written .debug$S symbol records. No .debug$T: the
# only types used are CodeView's built-in T_INT4 (0x74) and T_NOTYPE.
  .text
  .def add; .scl 2; .type 32; .endef
  .globl add
add:
.Lb_add:
  .cv_func_id 0
  .cv_file 1 "C:\\cxx1\\m10\\m10probe.c"
  .cv_loc 0 1 2 0
  .seh_proc add
  pushq %rbp
  .seh_pushreg %rbp
  movq %rsp, %rbp
  .seh_setframe %rbp, 0
  subq $16, %rsp
  .seh_stackalloc 16
  .seh_endprologue
  movl %ecx, -4(%rbp)
  movl %edx, -8(%rbp)
  .cv_loc 0 1 3 0
  movl -4(%rbp), %eax
  addl -8(%rbp), %eax
  movl %eax, -12(%rbp)
  .cv_loc 0 1 4 0
  movl -12(%rbp), %eax
  addl %eax, %eax
  addq $16, %rsp
  popq %rbp
  retq
.Le_add:
  .seh_endproc

  .def main; .scl 2; .type 32; .endef
  .globl main
main:
.Lb_main:
  .cv_func_id 1
  .cv_loc 1 1 7 0
  .seh_proc main
  pushq %rbp
  .seh_pushreg %rbp
  movq %rsp, %rbp
  .seh_setframe %rbp, 0
  subq $48, %rsp
  .seh_stackalloc 48
  .seh_endprologue
  .cv_loc 1 1 8 0
  movl $20, -4(%rbp)
  .cv_loc 1 1 9 0
  movl -4(%rbp), %ecx
  movl $1, %edx
  callq add
  movl %eax, -8(%rbp)
  .cv_loc 1 1 10 0
  movl -8(%rbp), %edx
  addl g(%rip), %edx
  leaq .Lfmt(%rip), %rcx
  callq printf
  .cv_loc 1 1 11 0
  xorl %eax, %eax
  addq $48, %rsp
  popq %rbp
  retq
.Le_main:
  .seh_endproc

  .data
  .globl g
  .p2align 2
g:
  .long 7
  .section .rdata,"dr"
.Lfmt:
  .asciz "%d\n"

  .section .debug$S,"dr"
  .p2align 2
  .long 4                       # CV_SIGNATURE_C13
# --- symbols for add
  .long 241                     # DEBUG_S_SYMBOLS
  .long .Ls1e-.Ls1b
.Ls1b:
  .short .Lr1e-.Lr1b
.Lr1b:
  .short 0x1110                 # S_GPROC32
  .long 0, 0, 0                 # parent, end, next
  .long .Le_add-.Lb_add         # code size
  .long 0, 0                    # debug start, debug end
  .long 0                       # type: T_NOTYPE
  .secrel32 add
  .secidx add
  .byte 0                       # flags
  .asciz "add"
  .p2align 2
.Lr1e:
  .short .Lr2e-.Lr2b
.Lr2b:
  .short 0x1111                 # S_REGREL32
  .long -4                      # offset from rbp
  .long 0x74                    # T_INT4
  .short 334                    # CV_AMD64_RBP
  .asciz "a"
  .p2align 2
.Lr2e:
  .short .Lr3e-.Lr3b
.Lr3b:
  .short 0x1111
  .long -8
  .long 0x74
  .short 334
  .asciz "b"
  .p2align 2
.Lr3e:
  .short .Lr4e-.Lr4b
.Lr4b:
  .short 0x1111
  .long -12
  .long 0x74
  .short 334
  .asciz "s"
  .p2align 2
.Lr4e:
  .short 2
  .short 6                      # S_END
.Ls1e:
  .p2align 2
  .cv_linetable 0, add, .Le_add
# --- symbols for main
  .long 241
  .long .Ls2e-.Ls2b
.Ls2b:
  .short .Lm1e-.Lm1b
.Lm1b:
  .short 0x1110
  .long 0, 0, 0
  .long .Le_main-.Lb_main
  .long 0, 0
  .long 0
  .secrel32 main
  .secidx main
  .byte 0
  .asciz "main"
  .p2align 2
.Lm1e:
  .short .Lm2e-.Lm2b
.Lm2b:
  .short 0x1111
  .long -4
  .long 0x74
  .short 334
  .asciz "x"
  .p2align 2
.Lm2e:
  .short .Lm3e-.Lm3b
.Lm3b:
  .short 0x1111
  .long -8
  .long 0x74
  .short 334
  .asciz "y"
  .p2align 2
.Lm3e:
  .short 2
  .short 6
# --- the global
  .short .Lg1e-.Lg1b
.Lg1b:
  .short 0x110d                 # S_GDATA32
  .long 0x74
  .secrel32 g
  .secidx g
  .asciz "g"
  .p2align 2
.Lg1e:
.Ls2e:
  .p2align 2
  .cv_linetable 1, main, .Le_main
  .cv_filechecksums
  .cv_stringtable
