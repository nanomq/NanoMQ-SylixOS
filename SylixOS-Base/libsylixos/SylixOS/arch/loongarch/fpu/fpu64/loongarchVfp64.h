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
** 文   件   名: loongarhVfp64.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 03 月 24 日
**
** 描        述: LoongArch 体系架构 VFP64 支持.
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
