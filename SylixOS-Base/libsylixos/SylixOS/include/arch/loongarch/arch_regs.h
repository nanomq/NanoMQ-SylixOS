/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: arch_regs.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 04 月 07 日
**
** 描        述: LoongArch 寄存器相关.
*********************************************************************************************************/

#ifndef __LOONGARCH_ARCH_REGS_H
#define __LOONGARCH_ARCH_REGS_H

#if LW_CFG_CPU_WORD_LENGHT == 32
#include "arch_regs32.h"
#else
#include "arch_regs64.h"
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

#endif                                                                  /*  __LOONGARCH_ARCH_REGS_H     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
