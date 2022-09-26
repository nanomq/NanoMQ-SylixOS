;/*********************************************************************************************************
;**
;**                                    �й�������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: riscvContextAsm.S
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2018 �� 03 �� 20 ��
;**
;** ��        ��: RISC-V ��ϵ�����������л�.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <config/kernel/kernel_cfg.h>
#include <config/mp/mp_cfg.h>
#include "riscvContextAsm.h"

    FILE_BEGIN()

#if LW_CFG_SMP_EN > 0
    IMPORT_LABEL(_SchedSafeStack)
#endif
    IMPORT_LABEL(_SchedSwp)
#if LW_CFG_COROUTINE_EN > 0
    IMPORT_LABEL(_SchedCrSwp)
#endif

    EXPORT_LABEL(archTaskCtxStart)
    EXPORT_LABEL(archTaskCtxSwitch)
#if LW_CFG_COROUTINE_EN > 0
    EXPORT_LABEL(archCrtCtxSwitch)
#endif
    EXPORT_LABEL(archIntCtxLoad)
    EXPORT_LABEL(archSigCtxLoad)
    EXPORT_LABEL(archTaskCtxCopy)

;/*********************************************************************************************************
;  ��ǰ CPU ��ʼ���ж�����ִ��
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxStart)
    REG_L   T0 , 0(A0)                                                  ;/*  ��ȡ��ǰ TCB �� REG_CTX ��ַ*/

    RESTORE_SMALL_REG_CTX                                               ;/*  �ָ�С�Ĵ���������          */
    FUNC_END(archTaskCtxStart)

;/*********************************************************************************************************
;  �߳��л�
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxSwitch)
    REG_L   T0 , 0(A0)                                                  ;/*  ��ȡ��ǰ TCB �� REG_CTX ��ַ*/

    SAVE_SMALL_REG_CTX                                                  ;/*  ����С�Ĵ���������          */

    MOVE    S0 , A0                                                     ;/*  S0 �ݴ� A0                  */

#if LW_CFG_SMP_EN > 0
    CALL    _SchedSafeStack                                             ;/*  SP = _SchedSafeStack();     */
    MOVE    SP , RV0
    MOVE    A0 , S0                                                     ;/*  �ָ� A0                     */
#endif

    CALL    _SchedSwp                                                   ;/*  _SchedSwp();                */

    REG_L   T0 , 0(S0)                                                  ;/*  ��ȡ��ǰ TCB �� REG_CTX ��ַ*/
    REG_L   T1 , CTXTYPE_OFFSET(T0)                                     ;/*  �������������              */
    BNEZ    T1 , _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  �ָ���Ĵ���������          */

LINE_LABEL(_RestoreSmallCtx)
    RESTORE_SMALL_REG_CTX                                               ;/*  �ָ�С�Ĵ���������          */
    FUNC_END(archTaskCtxSwitch)

;/*********************************************************************************************************
;  Э���л�
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

#if LW_CFG_COROUTINE_EN > 0
FUNC_DEF(archCrtCtxSwitch)
    REG_L   T0 , (2 * ARCH_REG_SIZE)(A0)                                ;/*  ��ȡ��ǰ CCB �� REG_CTX ��ַ*/

    SAVE_SMALL_REG_CTX                                                  ;/*  ����С�Ĵ���������          */

    MOVE    S0 , A0                                                     ;/*  S0 �ݴ� A0                  */

    CALL    _SchedCrSwp                                                 ;/*  _SchedCrSwp();              */

    REG_L   T0 , (2 * ARCH_REG_SIZE)(S0)                                ;/*  ��ȡ��ǰ CCB �� REG_CTX ��ַ*/
    RESTORE_SMALL_REG_CTX                                               ;/*  �ָ�С�Ĵ���������          */
    FUNC_END(archCrtCtxSwitch)
#endif

;/*********************************************************************************************************
;  �жϷ���ʱ, �߳�װ��
;  ����Ϊ��ǰ CPU ���ƿ�, �� A0 Ϊ��ǰ CPU ���ƿ�ָ��
;*********************************************************************************************************/

FUNC_DEF(archIntCtxLoad)
    REG_L   T0 , 0(A0)                                                  ;/*  ��ȡ��ǰ CCB �� REG_CTX ��ַ*/
    REG_L   T1 , CTXTYPE_OFFSET(T0)                                     ;/*  �������������              */
    BNEZ    T1 , _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  �ָ���Ĵ���������          */
    FUNC_END(archIntCtxLoad)

;/*********************************************************************************************************
;  �ź������ķ���
;  ����Ϊ ARCH_REG_CTX ָ��, �� A0 Ϊ ARCH_REG_CTX ָ��
;*********************************************************************************************************/

FUNC_DEF(archSigCtxLoad)
    MOVE    T0 , A0
    REG_L   T1 , CTXTYPE_OFFSET(T0)                                     ;/*  �������������              */
    BNEZ    T1 , _RestoreSmallCtx

    RESTORE_BIG_REG_CTX                                                 ;/*  �ָ���Ĵ���������          */
    FUNC_END(archSigCtxLoad)

;/*********************************************************************************************************
;  ��������������
;  ���� A0 ΪĿ�� ARCH_REG_CTX ָ��, A1 ΪԴ ARCH_REG_CTX ָ��
;*********************************************************************************************************/

FUNC_DEF(archTaskCtxCopy)
    REG_L   T0 , ( 0 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 0 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 1 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 1 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 2 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 2 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 3 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 3 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 4 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 4 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 5 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 5 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 6 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 6 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 7 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 7 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 8 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 8 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , ( 9 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , ( 9 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (10 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (10 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (11 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (11 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (12 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (12 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (13 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (13 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (14 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (14 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (15 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (15 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (16 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (16 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (17 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (17 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (18 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (18 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (19 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (19 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (20 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (20 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (21 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (21 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (22 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (22 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (23 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (23 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (24 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (24 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (25 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (25 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (26 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (26 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (27 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (27 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (28 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (28 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (29 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (29 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (30 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (30 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (31 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (31 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (32 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (32 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (33 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (33 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (34 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (34 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (35 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (35 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (36 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (36 * ARCH_REG_SIZE)(A0)

    REG_L   T0 , (37 * ARCH_REG_SIZE)(A1)
    REG_S   T0 , (37 * ARCH_REG_SIZE)(A0)

    RET
    FUNC_END(archTaskCtxCopy)

    FILE_END()

;/*********************************************************************************************************
;  END
;*********************************************************************************************************/