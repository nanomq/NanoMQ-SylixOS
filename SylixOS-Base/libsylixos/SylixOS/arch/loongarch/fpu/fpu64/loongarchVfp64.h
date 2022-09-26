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
** ��   ��   ��: loongarhVfp64.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2022 �� 03 �� 24 ��
**
** ��        ��: LoongArch ��ϵ�ܹ� VFP64 ֧��.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHVFP64_H
#define __ARCH_LOONGARCHVFP64_H

#include "../loongarchFpu.h"

PLOONGARCH_FPU_OP  loongarchVfp64PrimaryInit(CPCHAR    pcMachineName, CPCHAR  pcFpuName);
VOID               loongarchVfp64SecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

UINT32             loongarchVfp64GetFCSR(VOID);
VOID               loongarchVfp64SetFCSR(UINT32  uiFCSR);

#endif                                                                  /*  __ARCH_LOONGARCHVFP64_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
