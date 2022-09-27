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
** ��   ��   ��: loongarchFpu.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2022 �� 03 �� 23 ��
**
** ��        ��: LoongArch ��ϵ�ܹ�Ӳ������������ (VFP).
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHFPU_H
#define __ARCH_LOONGARCHFPU_H

/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

/*********************************************************************************************************
  LoongArch fpu ��������
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
  LoongArch fpu ��������
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
