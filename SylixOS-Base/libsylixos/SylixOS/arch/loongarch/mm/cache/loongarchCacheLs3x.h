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
** 文   件   名: loongarchCacheLs3x.h
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2022 年 03 月 15 日
**
** 描        述: LoongArch ls3a5000 体系构架 CACHE 驱动.
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
