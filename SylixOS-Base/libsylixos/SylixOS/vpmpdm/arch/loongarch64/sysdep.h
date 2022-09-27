#ifndef _SYS_ASM_H
#define _SYS_ASM_H

/* LP64 abi  */
# define zero   $r0
# define ra $r1
# define tp $r2
# define sp $r3
# define a0 $r4
# define a1 $r5
# define a2 $r6
# define a3 $r7
# define a4 $r8
# define a5 $r9
# define a6 $r10
# define a7 $r11
# define v0 $r4
# define v1 $r5
# define t0 $r12
# define t1 $r13
# define t2 $r14
# define t3 $r15
# define t4 $r16
# define t5 $r17
# define t6 $r18
# define t7 $r19
# define t8 $r20
# define x  $r21
# define fp $r22
# define s0 $r23
# define s1 $r24
# define s2 $r25
# define s3 $r26
# define s4 $r27
# define s5 $r28
# define s6 $r29
# define s7 $r30
# define s8 $r31

# define fa0    $f0
# define fa1    $f1
# define fa2    $f2
# define fa3    $f3
# define fa4    $f4
# define fa5    $f5
# define fa6    $f6
# define fa7    $f7
# define fv0    $f0
# define fv1    $f1
# define ft0    $f8
# define ft1    $f9
# define ft2    $f10
# define ft3    $f11
# define ft4    $f12
# define ft5    $f13
# define ft6    $f14
# define ft7    $f15
# define ft8    $f16
# define ft9    $f17
# define ft10   $f18
# define ft11   $f19
# define ft12   $f20
# define ft13   $f21
# define ft14   $f22
# define ft15   $f23
# define fs0    $f24
# define fs1    $f25
# define fs2    $f26
# define fs3    $f27
# define fs4    $f28
# define fs5    $f29
# define fs6    $f30
# define fs7    $f31

/* Makros to generate eh_frame unwind information.  */
# define cfi_startproc          .cfi_startproc
# define cfi_endproc            .cfi_endproc
# define cfi_def_cfa(reg, off)      .cfi_def_cfa reg, off
# define cfi_def_cfa_register(reg)  .cfi_def_cfa_register reg
# define cfi_def_cfa_offset(off)    .cfi_def_cfa_offset off
# define cfi_adjust_cfa_offset(off) .cfi_adjust_cfa_offset off
# define cfi_offset(reg, off)       .cfi_offset reg, off
# define cfi_rel_offset(reg, off)   .cfi_rel_offset reg, off
# define cfi_register(r1, r2)       .cfi_register r1, r2
# define cfi_return_column(reg) .cfi_return_column reg
# define cfi_restore(reg)       .cfi_restore reg
# define cfi_same_value(reg)        .cfi_same_value reg
# define cfi_undefined(reg)     .cfi_undefined reg
# define cfi_remember_state     .cfi_remember_state
# define cfi_restore_state      .cfi_restore_state
# define cfi_window_save        .cfi_window_save
# define cfi_personality(enc, exp)  .cfi_personality enc, exp
# define cfi_lsda(enc, exp)     .cfi_lsda enc, exp

/* Macros to handle different pointer/register sizes for 32/64-bit code.  */
#ifdef __loongarch64
# define PTRLOG 3
# define SZREG  8
# define SZFREG 8
# define REG_L ld.d
# define REG_S st.d
# define FREG_L fld.d
# define FREG_S fst.d
#elif defined __loongarch32
# define PTRLOG 2
# define SZREG  4
# define SZFREG 4
# define REG_L ld.w
# define REG_S st.w
# define FREG_L fld.w
# define FREG_S fst.w
#else
# error __loongarch_xlen must equal 32 or 64
#endif


/* Declare leaf routine.  */
#define LEAF(symbol)            \
    .text;              \
    .globl  symbol;         \
    .align  3;          \
    cfi_startproc ;         \
    .type   symbol, @function;  \
symbol:

# define ENTRY(symbol) LEAF(symbol)

/* Mark end of function.  */
#undef END
#define END(function)           \
    cfi_endproc ;           \
    .size   function,.-function;

/* Stack alignment.  */
#define ALMASK  ~15

# undef libc_hidden_builtin_def
# define libc_hidden_builtin_def(name)

#endif /* sys/asm.h */
