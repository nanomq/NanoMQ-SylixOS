/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: assembler.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2021 年 10 月 13 日
**
** 描        述: LoongArch 汇编相关.
*********************************************************************************************************/

#ifndef __ASMLOONGARCH_ASSEMBLER_H
#define __ASMLOONGARCH_ASSEMBLER_H

#include "archprob.h"
#include "arch/loongarch/arch_def.h"

#ifndef __MP_CFG_H
#include "../SylixOS/config/mp/mp_cfg.h"
#endif

/*********************************************************************************************************
  loongarch architecture assembly special code
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)

/*********************************************************************************************************
  assembler define
*********************************************************************************************************/

#define EXPORT_LABEL(label)       .global label

#define IMPORT_LABEL(label)       .extern label

#define FUNC_LABEL(func)          func:
#define LINE_LABEL(line)          line:

#define FUNC_DEF(name)                  \
        .align  3;                      \
        .globl name;                    \
name: ;                                 \

#define FUNC_END(name)

#define MACRO_DEF(mfunc...)             \
        .macro      mfunc

#define MACRO_END()                     \
        .endm

#define FILE_BEGIN()                    \
        .balign     4;

#define FILE_END()

#define SECTION(sec)                    \
        .section    sec

#define WEAK(name)                      \
        .weak       name;               \
        .balign     4;

/*********************************************************************************************************
  Macros to handle different pointer/register sizes for 32/64-bit code
*********************************************************************************************************/

/*********************************************************************************************************
  Size of a register
*********************************************************************************************************/

#ifdef __loongarch64
#define SZREG       8
#else
#define SZREG       4
#endif

/*********************************************************************************************************
  Use the following macros in assemblercode to load/store registers, pointers etc.
*********************************************************************************************************/

#ifdef __loongarch64
#define REG_S       st.d
#define REG_L       ld.d
#define REG_SUB     sub.d
#define REG_ADD     add.d
#define REG_ADDI    addi.d
#else
#define REG_S       st.w
#define REG_L       ld.w
#define REG_SUB     sub.w
#define REG_ADD     add.w
#define REG_ADDI    addi.w
#endif

/*********************************************************************************************************
  LLSC instructions
*********************************************************************************************************/

#ifdef __loongarch64
#define LL          ll.d
#define SC          sc.d
#else
#define LL          ll.w
#define SC          sc.w
#endif

/*********************************************************************************************************
  LI instructions
*********************************************************************************************************/

#ifdef __loongarch64
#define LI          li.d
#else
#define LI          li.w
#endif

/*********************************************************************************************************
  How to add/sub/load/store/shift C int variables.
*********************************************************************************************************/

#if (_LOONGARCH_SZINT == 32)

#define INT_ADD     add.w
#define INT_ADDI    addi.w
#define INT_SUB     sub.w
#define INT_L       ld.w
#define INT_S       st.w
#define INT_SLL     sll.w
#define INT_SLLI    slli.w
#define INT_SRL     srl.w
#define INT_SRLI    srli.w
#define INT_SRA     sra.w
#define INT_SRAI    srai.w

#else

#define INT_ADD     add.d
#define INT_ADDI    addi.d
#define INT_SUB     sub.d
#define INT_L       ld.d
#define INT_S       st.d
#define INT_SLL     sll.d
#define INT_SLLI    slli.d
#define INT_SRL     srl.d
#define INT_SRLI    srli.d
#define INT_SRA     sra.d
#define INT_SRAI    srai.d

#endif

/*********************************************************************************************************
  How to add/sub/load/store/shift C long variables.
*********************************************************************************************************/

#if (_LOONGARCH_SZLONG == 32)

#define LONG_ADD     add.w
#define LONG_ADDI    addi.w
#define LONG_SUB     sub.w
#define LONG_L       ld.w
#define LONG_S       st.w
#define LONG_SLL     sll.w
#define LONG_SLLI    slli.w
#define LONG_SRL     srl.w
#define LONG_SRLI    srli.w
#define LONG_SRA     sra.w
#define LONG_SRAI    srai.w

#define LONG        .word
#define LONGSIZE    4
#define LONGMASK    3
#define LONGLOG     2

#else

#define LONG_ADD     add.d
#define LONG_ADDI    addi.d
#define LONG_SUB     sub.d
#define LONG_L       ld.d
#define LONG_S       st.d
#define LONG_SLL     sll.d
#define LONG_SLLI    slli.d
#define LONG_SRL     srl.d
#define LONG_SRLI    srli.d
#define LONG_SRA     sra.d
#define LONG_SRAI    srai.d

#define LONG        .dword
#define LONGSIZE    8
#define LONGMASK    7
#define LONGLOG     3

#endif

/*********************************************************************************************************
  How to add/sub/load/store/shift pointers.
*********************************************************************************************************/

#if (_LOONGARCH_SZPTR == 32)

#define PTR_ADD     add.w
#define PTR_ADDI    addi.w
#define PTR_SUB     sub.w
#define PTR_L       ld.w
#define PTR_S       st.w
#define PTR_LA      la
#define PTR_LI      li.w
#define PTR_SLL     sll.w
#define PTR_SLLI    slli.w
#define PTR_SRL     srl.w
#define PTR_SRLI    srli.w
#define PTR_SRA     sra.w
#define PTR_SRAI    srai.w

#define PTR_SCALESHIFT  2

#define PTR         .word
#define PTRSIZE     4
#define PTRLOG      2

#else

#define PTR_ADD     add.d
#define PTR_ADDI    addi.d
#define PTR_SUB     sub.d
#define PTR_L       ld.d
#define PTR_S       st.d
#define PTR_LA      la
#define PTR_LI      li.d
#define PTR_SLL     sll.d
#define PTR_SLLI    slli.d
#define PTR_SRL     srl.d
#define PTR_SRLI    srli.d
#define PTR_SRA     sra.d
#define PTR_SRAI    srai.d

#define PTR_SCALESHIFT  3

#define PTR         .dword
#define PTRSIZE     8
#define PTRLOG      3
#endif

/*********************************************************************************************************
  macros define
*********************************************************************************************************/

#define MOV             move

/*********************************************************************************************************
  通用寄存器定义(LP64 ABI)
*********************************************************************************************************/

#define ZERO                    $r0                                     /*  wired zero                  */
#define RA                      $r1                                     /*  return address              */
#define GP                      $r2                                     /*  global pointer              */
#define SP                      $r3                                     /*  stack pointer               */
#define A0                      $r4                                     /*  arg reg 0                   */
#define A1                      $r5                                     /*  arg reg 1                   */
#define V0                      $r4                                     /*  return reg 1                */
#define V1                      $r5                                     /*  return reg 2                */
#define A2                      $r6                                     /*  arg reg 2                   */
#define A3                      $r7                                     /*  arg reg 3                   */
#define A4                      $r8                                     /*  arg reg 4                   */
#define A5                      $r9                                     /*  arg reg 5                   */
#define A6                      $r10                                    /*  arg reg 6                   */
#define A7                      $r11                                    /*  arg reg 7                   */
#define T0                      $r12                                    /*  caller saved 0              */
#define T1                      $r13                                    /*  caller saved 1              */
#define T2                      $r14                                    /*  caller saved 2              */
#define T3                      $r15                                    /*  caller saved 3              */
#define T4                      $r16                                    /*  caller saved 4              */
#define T5                      $r17                                    /*  caller saved 5              */
#define T6                      $r18                                    /*  caller saved 6              */
#define T7                      $r19                                    /*  caller saved 7              */
#define T8                      $r20                                    /*  caller saved 8              */
#define TP                      $r21                                    /*  TLS                         */
#define S9                      $r22                                    /*  callee saved 9              */
#define FP                      S9                                      /*  frame pointer               */
#define S0                      $r23                                    /*  callee saved 0              */
#define S1                      $r24                                    /*  callee saved 1              */
#define S2                      $r25                                    /*  callee saved 2              */
#define S3                      $r26                                    /*  callee saved 3              */
#define S4                      $r27                                    /*  callee saved 4              */
#define S5                      $r28                                    /*  callee saved 5              */
#define S6                      $r29                                    /*  callee saved 6              */
#define S7                      $r30                                    /*  callee saved 7              */
#define S8                      $r31                                    /*  callee saved 8              */

/*********************************************************************************************************
  控制状态寄存器定义
*********************************************************************************************************/

#define CSR_CRMD                0x0
#define CSR_PRMD                0x1
#define CSR_EUEN                0x2
#define CSR_MISC                0x3
#define CSR_ECFG                0x4
#define CSR_ESTAT               0x5
#define CSR_ERA                 0x6
#define CSR_BADV                0x7
#define CSR_BADI                0x8
#define CSR_EENTRY              0xc
#define CSR_CPUID               0x20
#define CSR_TLBRBADV            0x89
#define CSR_TLBRERA             0x8a
#define CSR_TLBRPRMD            0x8f
#define CSR_MERRCTL             0x90
#define CSR_MERRERA             0x94
#define CSR_SAVE                0x30

#else
/*********************************************************************************************************
  LoongArch LLSC 内存屏障
*********************************************************************************************************/

#if LW_CFG_SMP_EN > 0
#define KN_WEAK_LLSC_MB             "   dbar 0  \n"
#else
#define KN_WEAK_LLSC_MB             "           \n"
#endif

#define KN_SMP_LLSC_MB()            __asm__ __volatile__(KN_WEAK_LLSC_MB : : : "memory")

#define KN_SMP_MB_BEFORE_LLSC()     KN_SMP_LLSC_MB()

#define KN_SMP_MB_BEFORE_ATOMIC()   KN_SMP_MB_BEFORE_LLSC()
#define KN_SMP_MB_AFTER_ATOMIC()    KN_SMP_LLSC_MB()

#endif                                                                  /*  __ASSEMBLY__                */
#endif                                                                  /*  __ASMLOONGARCH_ASSEMBLER_H  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
