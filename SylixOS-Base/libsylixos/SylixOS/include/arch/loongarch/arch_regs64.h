/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: arch_regs64.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2021 �� 10 �� 14 ��
**
** ��        ��: LoongArch64 �Ĵ������.
*********************************************************************************************************/

#ifndef __LOONGARCH_ARCH_REGS64_H
#define __LOONGARCH_ARCH_REGS64_H

/*********************************************************************************************************
  ����
*********************************************************************************************************/

#define ARCH_GREG_NR            32                                      /*  ͨ�üĴ�����Ŀ              */

#define ARCH_REG_CTX_WORD_SIZE  40                                      /*  �Ĵ�������������            */
#define ARCH_STK_MIN_WORD_SIZE  512                                     /*  ��ջ��С����                */

#define ARCH_REG_SIZE           8                                       /*  �Ĵ�����С                  */
#define ARCH_REG_CTX_SIZE       (ARCH_REG_CTX_WORD_SIZE * ARCH_REG_SIZE)/*  �Ĵ��������Ĵ�С            */

#define ARCH_STK_ALIGN_SIZE     16                                      /*  ��ջ����Ҫ��                */

#define ARCH_JMP_BUF_WORD_SIZE  ARCH_REG_CTX_WORD_SIZE                  /*  ��ת��������(������)      */

/*********************************************************************************************************
  �Ĵ����� ARCH_REG_CTX �е�ƫ����
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
  �Ĵ�����
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)

typedef UINT64      ARCH_REG_T;

typedef struct {
    ARCH_REG_T  REG_ulSmallCtx;                                         /*  С������                    */
    ARCH_REG_T  REG_ulReg[ARCH_GREG_NR];                                /*  32 ��ͨ��Ŀ�ļĴ���         */
    ARCH_REG_T  REG_ulPc;                                               /*  ����������Ĵ���            */
    ARCH_REG_T  REG_ulCsrCRMD;                                          /*  ��ǰģʽ��Ϣ�Ĵ���          */
    ARCH_REG_T  REG_ulCsrESTAT;                                         /*  �����жϻ����쳣�鿴�ļĴ���*/
    ARCH_REG_T  REG_ulCsrERA;                                           /*  ���ⷵ�ص�ַ�Ĵ���          */
    ARCH_REG_T  REG_ulCsrBADV;                                          /*  �������ַ�Ĵ���            */
    ARCH_REG_T  REG_ulCsrEUEN;                                          /*  ��չ����ʹ�ܼĴ���          */
    ARCH_REG_T  REG_ulPad;

/*********************************************************************************************************
  LoongArch64 LP64 ABI �ļĴ�������
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
  ���û��ݶ�ջ��
*********************************************************************************************************/

typedef struct {
    ARCH_REG_T          FP_ulFp;
    ARCH_REG_T          FP_ulRa;
} ARCH_FP_CTX;

/*********************************************************************************************************
  ���������л�ȡ��Ϣ
*********************************************************************************************************/

#define ARCH_REG_CTX_GET_PC(ctx)    ((void *)(ctx).REG_ulCsrERA)

#endif                                                                  /*  !defined(__ASSEMBLY__)      */
#endif                                                                  /*  __LOONGARCH_ARCH_REGS64_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
