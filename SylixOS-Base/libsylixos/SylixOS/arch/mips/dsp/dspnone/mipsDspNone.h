/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mipsDspNone.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 01 �� 10 ��
**
** ��        ��: MIPS ��ϵ�ܹ��� DSP ֧��.
*********************************************************************************************************/

#ifndef __ARCH_MIPSDSPNONE_H
#define __ARCH_MIPSDSPNONE_H

PMIPS_DSP_OP  mipsDspNonePrimaryInit(CPCHAR    pcMachineName, CPCHAR  pcDspName);
VOID          mipsDspNoneSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcDspName);

#endif                                                                  /*  __ARCH_MIPSDSPNONE_H        */
/*********************************************************************************************************
  END
*********************************************************************************************************/