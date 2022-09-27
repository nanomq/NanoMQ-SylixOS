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
** ��   ��   ��: loongarchCacheLs3x.h
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2022 �� 03 �� 15 ��
**
** ��        ��: LoongArch ls3a5000 ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHCACHELS3X_H
#define __ARCH_LOONGARCHCACHELS3X_H

VOID  loongarchCacheLs3xInit(LW_CACHE_OP *pcacheop,
                             CACHE_MODE   uiInstruction,
                             CACHE_MODE   uiData,
                             CPCHAR       pcMachineName);
VOID  loongarchCacheLs3xReset(CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_LOONGARCHCACHELS3X_H */
/*********************************************************************************************************
  END
*********************************************************************************************************/
