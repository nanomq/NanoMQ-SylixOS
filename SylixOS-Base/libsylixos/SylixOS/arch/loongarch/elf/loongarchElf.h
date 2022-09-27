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
** ��   ��   ��: loongarchElf.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2022 �� 03 �� 10 ��
**
** ��        ��: ʵ�� LoongArch ��ϵ�ṹ�� ELF �ļ��ض�λ.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHELF_H
#define __ARCH_LOONGARCHELF_H

#if defined(LW_CFG_CPU_ARCH_LOONGARCH)                                  /*  LoongArch ��ϵ�ṹ          */

#if LW_CFG_CPU_WORD_LENGHT == 32
#define ELF_CLASS              ELFCLASS32
#else
#define ELF_CLASS              ELFCLASS64
#endif
#define ELF_ARCH               EM_LOONGARCH

#define LA_RELA_STACK_DEPTH    16

#endif                                                                  /*  LW_CFG_CPU_ARCH_LOONGARCH   */
#endif                                                                  /*  __ARCH_LOONGARCHELF_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
