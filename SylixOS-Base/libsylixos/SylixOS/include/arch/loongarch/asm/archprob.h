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
** ��   ��   ��: archprob.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2021 �� 10 �� 13 ��
**
** ��        ��: LoongArch ƽ̨����̽��.
*********************************************************************************************************/

#ifndef __ARCHPROB_H
#define __ARCHPROB_H

/*********************************************************************************************************
  LoongArch architecture detect
*********************************************************************************************************/

#define __SYLIXOS_LOONGARCH_ARCH_LOONGARCH32      32
#define __SYLIXOS_LOONGARCH_ARCH_LOONGARCH64      64

#ifdef __GNUC__
#  if defined(_LOONGARCH_ARCH_LOONGARCH32)
#    define __SYLIXOS_LOONGARCH_ARCH__   __SYLIXOS_LOONGARCH_ARCH_LOONGARCH32

#  elif defined(_LOONGARCH_ARCH_LOONGARCH64)
#    define __SYLIXOS_LOONGARCH_ARCH__   __SYLIXOS_LOONGARCH_ARCH_LOONGARCH64

#  endif                                                                /*  user define only            */

#else
#  define __SYLIXOS_LOONGARCH_ARCH__     __SYLIXOS_LOONGARCH_ARCH_LOONGARCH32
                                                                        /*  default LOONGARCH32         */
#endif

#endif                                                                  /*  __ARCHPROB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
