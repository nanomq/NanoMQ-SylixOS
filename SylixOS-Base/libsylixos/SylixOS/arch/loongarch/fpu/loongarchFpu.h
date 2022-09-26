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
** 文   件   名: loongarchFpu.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 03 月 23 日
**
** 描        述: LoongArch 体系架构硬件浮点运算器 (VFP).
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHFPU_H
#define __ARCH_LOONGARCHFPU_H

/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  LoongArch fpu 操作函数
*********************************************************************************************************/

typedef struct {
    VOIDFUNCPTR     LFPU_pfuncEnable;
    VOIDFUNCPTR     LFPU_pfuncDisable;
    BOOLFUNCPTR     LFPU_pfuncIsEnable;
    VOIDFUNCPTR     LFPU_pfuncSave;
    VOIDFUNCPTR     LFPU_pfuncRestore;
    VOIDFUNCPTR     LFPU_pfuncCtxShow;
} LOONGARCH_FPU_OP;
typedef LOONGARCH_FPU_OP *PLOONGARCH_FPU_OP;

/*********************************************************************************************************
  LoongArch fpu 基本操作
*********************************************************************************************************/

#define LOONGARCH_VFP_ENABLE(op)              op->LFPU_pfuncEnable()
#define LOONGARCH_VFP_DISABLE(op)             op->LFPU_pfuncDisable()
#define LOONGARCH_VFP_ISENABLE(op)            op->LFPU_pfuncIsEnable()
#define LOONGARCH_VFP_SAVE(op, ctx)           op->LFPU_pfuncSave((ctx))
#define LOONGARCH_VFP_RESTORE(op, ctx)        op->LFPU_pfuncRestore((ctx))
#define LOONGARCH_VFP_CTXSHOW(op, fd, ctx)    op->LFPU_pfuncCtxShow((fd), (ctx))

#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */
#endif                                                                  /*  __ARCH_LOONGARCHFPU_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
