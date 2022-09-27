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
** ��   ��   ��: loongarhVfp32.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2022 �� 03 �� 23 ��
**
** ��        ��: LoongArch ��ϵ�ܹ� VFP32 ֧��.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHVFP32_H
#define __ARCH_LOONGARCHVFP32_H

#include "../loongarchFpu.h"

PLOONGARCH_FPU_OP  loongarchVfp32PrimaryInit(CPCHAR    pcMachineName, CPCHAR  pcFpuName);
VOID               loongarchVfp32SecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

UINT32             loongarchVfp32GetFCSR(VOID);
VOID               loongarchVfp32SetFCSR(UINT32  uiFCSR);

#endif                                                                  /*  __ARCH_LOONGARCHVFP32_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
