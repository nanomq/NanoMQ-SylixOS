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
** 文   件   名: arch_regs64.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2021 年 10 月 14 日
**
** 描        述: LoongArch64 寄存器相关.
*********************************************************************************************************/

#ifndef __LOONGARCH_ARCH_REGS64_H
#define __LOONGARCH_ARCH_REGS64_H

/*********************************************************************************************************
  定义
*********************************************************************************************************/

#define ARCH_GREG_NR            32                                      /*  通用寄存器数目              */

#define ARCH_REG_CTX_WORD_SIZE  40                                      /*  寄存器上下文字数            */
#define ARCH_STK_MIN_WORD_SIZE  512                                     /*  堆栈最小字数                */

#define ARCH_REG_SIZE           8                                       /*  寄存器大小                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  寄存器上下文大小            */

#define ARCH_STK_ALIGN_SIZE     16                                      /*  堆栈对齐要求                */

#define ARCH_JMP_BUF_WORD_SIZE  ARCH_REG_CTX_WORD_SIZE                  /*  跳转缓冲字数(向后兼容)      */

/*********************************************************************************************************
  寄存器在 ARCH_REG_CTX 中的偏移量
*********************************************************************************************************/

#define CTX_TYPE_OFFSET         (0 * ARCH_REG_SIZE)
#define XGREG(n)                ((n + 1) * ARCH_REG_SIZE)
#define XPC                     ((ARCH_GREG_NR + 1) * ARCH_REG_SIZE)
#define XCRMD                   ((ARCH_GREG_NR + 2) * ARCH_REG_SIZE)
#define XESTAT                  ((ARCH_GREG_NR + 3) * ARCH_REG_SIZE)
#define XERA                    ((ARCH_GREG_NR + 4) * ARCH_REG_SIZE)
#define XBADVADDR               ((ARCH_GREG_NR + 5) * ARCH_REG_SIZE)
#define XEUEN                   ((ARCH_GREG_NR + 6) * ARCH_REG_SIZE)

/*********************************************************************************************************
  寄存器表
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT64      ARCH_REG_T;

typedef struct {
    ARCH_REG_T  REG_ulSmallCtx;                                         /*  小上下文                    */
    ARCH_REG_T  REG_ulReg[ARCH_GREG_NR];                                /*  32 个通用目的寄存器         */
    ARCH_REG_T  REG_ulPc;                                               /*  程序计数器寄存器            */
    ARCH_REG_T  REG_ulCsrCRMD;                                          /*  当前模式信息寄存器          */
    ARCH_REG_T  REG_ulCsrESTAT;                                         /*  产生中断或者异常查看的寄存器*/
    ARCH_REG_T  REG_ulCsrERA;                                           /*  例外返回地址寄存器          */
    ARCH_REG_T  REG_ulCsrBADV;                                          /*  出错虚地址寄存器            */
    ARCH_REG_T  REG_ulCsrEUEN;                                          /*  扩展部件使能寄存器          */
    ARCH_REG_T  REG_ulPad;

/*********************************************************************************************************
  LoongArch64 LP64 ABI 的寄存器索引
*********************************************************************************************************/
#define REG_ZERO                0                                       /*  wired zero                  */
#define REG_RA                  1                                       /*  return address              */
#define REG_GP                  2                                       /*  global pointer              */
#define REG_SP                  3                                       /*  stack pointer               */
#define REG_A0                  4                                       /*  arg reg 0                   */
#define REG_A1                  5                                       /*  arg reg 1                   */
#define REG_V0                  4                                       /*  return reg 0                */
#define REG_V1                  5                                       /*  return reg 1                */
#define REG_A2                  6                                       /*  arg reg 2                   */
#define REG_A3                  7                                       /*  arg reg 3                   */
#define REG_A4                  8                                       /*  arg reg 4                   */
#define REG_A5                  9                                       /*  arg reg 5                   */
#define REG_A6                  10                                      /*  arg reg 6                   */
#define REG_A7                  11                                      /*  arg reg 7                   */
#define REG_T0                  12                                      /*  caller saved 0              */
#define REG_T1                  13                                      /*  caller saved 1              */
#define REG_T2                  14                                      /*  caller saved 2              */
#define REG_T3                  15                                      /*  caller saved 3              */
#define REG_T4                  16                                      /*  caller saved 4              */
#define REG_T5                  17                                      /*  caller saved 5              */
#define REG_T6                  18                                      /*  caller saved 6              */
#define REG_T7                  19                                      /*  caller saved 7              */
#define REG_T8                  20                                      /*  caller saved 8              */
#define REG_TP                  21                                      /*  TLS                         */
#define REG_S9                  22                                      /*  callee saved 9              */
#define REG_FP                  REG_S9                                  /*  frame pointer               */
#define REG_S0                  23                                      /*  callee saved 0              */
#define REG_S1                  24                                      /*  callee saved 1              */
#define REG_S2                  25                                      /*  callee saved 2              */
#define REG_S3                  26                                      /*  callee saved 3              */
#define REG_S4                  27                                      /*  callee saved 4              */
#define REG_S5                  28                                      /*  callee saved 5              */
#define REG_S6                  29                                      /*  callee saved 6              */
#define REG_S7                  30                                      /*  callee saved 7              */
#define REG_S8                  31                                      /*  callee saved 8              */
} ARCH_REG_CTX;

/*********************************************************************************************************
  调用回溯堆栈表
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T          FP_ulFp;
    ARCH_REG_T          FP_ulRa;
} ARCH_FP_CTX;

/*********************************************************************************************************
  从上下文中获取信息
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_ulCsrERA)

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __LOONGARCH_ARCH_REGS64_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
