/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: loongarchVfpNone.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2022 �� 03 �� 23 ��
**
** ��        ��: LoongArch ��ϵ�ܹ��� VFP ֧��.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHVFPNONE_H
#define __ARCH_LOONGARCHVFPNONE_H

PLOONGARCH_FPU_OP  loongarchVfpNonePrimaryInit(CPCHAR    pcMachineName, CPCHAR  pcFpuName);
VOID               loongarchVfpNoneSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

#endif                                                                  /*  __ARCH_LOONGARCHVFPNONE_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
