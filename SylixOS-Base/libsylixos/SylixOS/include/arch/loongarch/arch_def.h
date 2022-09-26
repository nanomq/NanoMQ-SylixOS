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
** ��   ��   ��: arch_def.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2021 �� 10 �� 13 ��
**
** ��        ��: LoongArch ��ض���.
*********************************************************************************************************/

#ifndef __LOONGARCH_ARCH_DEF_H
#define __LOONGARCH_ARCH_DEF_H

/*********************************************************************************************************
  __CONST64
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define __CONST64(x)                x
#else
#ifdef __SYLIXOS_KERNEL
#if LW_CFG_CPU_WORD_LENGHT == 32
#define __CONST64(x)                x##ull
#else
#define __CONST64(x)                x##ul
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT      */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ASSEMBLY__                */

/*********************************************************************************************************
  ��������ͷ�ļ�
*********************************************************************************************************/

#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#include "./inc/loongarchregs.h"

/*********************************************************************************************************
  LoongArch ָ��
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef UINT32                      LOONGARCH_INSTRUCTION;
#define LOONGARCH_EXEC_INST(inst)   __asm__ __volatile__ (inst)
#endif                                                                  /*  !defined(__ASSEMBLY__)      */

/*********************************************************************************************************
  Current Mode (CRMD) Register

   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       R                   |W| D | D |P|D|I| P |
  |                       s                   |E| A | A |G|A|E| L | CRMD
  |                       v                   | | T | T | | | | V |
  |                                           | | M | F | | | |   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_CRMD_WE       9                                               /*  Watch point enable (R/W)    */
#define M_CRMD_WE       (0x1 << S_CRMD_WE)
#define S_CRMD_DATM     7                                               /*  Is mem access type in direct*/
                                                                        /*  address translation (R/W)   */
#define M_CRMD_DATM     (0x3 << S_CRMD_DATM)
#define S_CRMD_DATF     5                                               /*  Instruction mem access type */
                                                                        /*  in DAT (R/W)                */
#define M_CRMD_DATF     (0x3 << S_CRMD_DATF)
#define S_CRMD_PG       4                                               /*  Mapping address enable (R/W)*/
#define M_CRMD_PG       (0x1 << S_CRMD_PG)
#define S_CRMD_DA       3                                               /*  Direct address enable (R/W) */
#define M_CRMD_DA       (0x1 << S_CRMD_DA)
#define S_CRMD_IE       2                                               /*  Global int enable (R/W)     */
#define M_CRMD_IE       (0x1 << S_CRMD_IE)
#define S_CRMD_PLV      0                                               /*  PLV level (R/W)             */
#define M_CRMD_PLV      (0x3 << S_CRMD_PLV)

/*********************************************************************************************************
  Prior Mode (PRMD) Register

   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           R                           |P|P| P | PRMD
  |                           s                           |W|I| P |
  |                           v                           |E|E| L |
  |                                                       | | | V |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*********************************************************************************************************/

#define S_PRMD_PWE      3                                               /*  Watch point enable (R/W)    */
#define M_PRMD_PWE      (0x1 << S_PRMD_PWE)
#define S_PRMD_PIE      2                                               /*  Global int enable (R/W)     */
#define M_PRMD_PIE      (0x1 << S_PRMD_PIE)
#define S_PRMD_PPLV     0                                               /*  PLV level (R/W)             */
#define M_PRMD_PPLV     (0x3 << S_PRMD_PPLV)

#endif                                                                  /*  defined(__SYLIXOS_KERNEL)   */
#endif                                                                  /*  __LOONGARCH_ARCH_DEF_H      */
/*********************************************************************************************************
  END
*********************************************************************************************************/
