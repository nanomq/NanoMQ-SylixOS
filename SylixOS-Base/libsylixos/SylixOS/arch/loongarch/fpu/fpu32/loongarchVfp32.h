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
** 文   件   名: loongarhVfp32.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 03 月 23 日
**
** 描        述: LoongArch 体系架构 VFP32 支持.
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
