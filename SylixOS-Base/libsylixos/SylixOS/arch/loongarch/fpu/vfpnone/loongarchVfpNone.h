/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: loongarchVfpNone.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 03 月 23 日
**
** 描        述: LoongArch 体系架构无 VFP 支持.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHVFPNONE_H
#define __ARCH_LOONGARCHVFPNONE_H

PLOONGARCH_FPU_OP  loongarchVfpNonePrimaryInit(CPCHAR    pcMachineName, CPCHAR  pcFpuName);
VOID               loongarchVfpNoneSecondaryInit(CPCHAR  pcMachineName, CPCHAR  pcFpuName);

#endif                                                                  /*  __ARCH_LOONGARCHVFPNONE_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
