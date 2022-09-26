;/*********************************************************************************************************
;**
;**                                    �й������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: loongarchContextAsm.h
;**
;** ��   ��   ��: Wang.Ziyang (������)
;**
;** �ļ���������: 2021 �� 11 �� 04 ��
;**
;** ��        ��: LoongArch ��ϵ�ܹ������Ĵ���.
;*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHCONTEXTASM_H
#define __ARCH_LOONGARCHCONTEXTASM_H

#include "arch/loongarch/arch_regs.h"

;/*********************************************************************************************************
;  ���� CSR �Ĵ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_CSR)
    CSRRD       T1 , CSR_CRMD                                           ;/*  ���� CRMD �Ĵ���            */
    REG_S       T1 , TP , XCRMD

    CSRRD       T1 , CSR_ESTAT                                          ;/*  ���� ESTAT �Ĵ���           */
    REG_S       T1 , TP , XESTAT

    CSRRD       T1 , CSR_BADV                                           ;/*  ���� BADV �Ĵ���            */
    REG_S       T1 , TP , XBADVADDR

    CSRRD       T1 , CSR_EUEN                                           ;/*  ���� EUEN �Ĵ���            */
    REG_S       T1 , TP , XEUEN
    MACRO_END()

;/*********************************************************************************************************
;  �ָ� CSR �Ĵ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_CSR)
    REG_L       T1 , TP , XCRMD
    INT_SRLI    T2 , T1 , 6
    ANDI        T2 , T2 , 0x8
    ANDI        T1 , T1 , 0x7
    OR          T1 , T1 , T2
    CSRWR       T1 , CSR_PRMD                                           ;/*  �ָ� CRMD �� PLV IE WE      */
    MACRO_END()

;/*********************************************************************************************************
;  ����С�����ļĴ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_SMALL_REG_CTX)
    ORI         T0 , ZERO , 1
    REG_S       T0 , TP , CTX_TYPE_OFFSET                               ;/*  ���ΪС������              */

    ;/*
    ; * ���� S0-S8($23-$31)
    ; */
    REG_S       $r23 , TP , XGREG(23)
    REG_S       $r24 , TP , XGREG(24)
    REG_S       $r25 , TP , XGREG(25)
    REG_S       $r26 , TP , XGREG(26)
    REG_S       $r27 , TP , XGREG(27)
    REG_S       $r28 , TP , XGREG(28)
    REG_S       $r29 , TP , XGREG(29)
    REG_S       $r30 , TP , XGREG(30)
    REG_S       $r31 , TP , XGREG(31)
    ;/*
    ; * ���� RA GP SP FP ($1-$3 $22)
    ; */
    REG_S       $r1  , TP , XGREG(1)
    REG_S       $r2  , TP , XGREG(2)
    REG_S       $r3  , TP , XGREG(3)
    REG_S       $r22 , TP , XGREG(22)

    REG_S       RA , TP , XPC                                           ;/*  RA ���� PC ����             */

    SAVE_CSR                                                            ;/*  ���� CSR �Ĵ���             */
    MACRO_END()

;/*********************************************************************************************************
;  �ָ�С�����ļĴ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_SMALL_REG_CTX)
    REG_L       T1 , TP , XPC
    CSRWR       T1 , CSR_ERA                                            ;/*  PC �ָ��� ERA �Ĵ���        */

    RESTORE_CSR                                                         ;/*  �ָ� CSR �Ĵ���             */

    ;/*
    ; * �ָ� S0-S8($23-$31)
    ; */
    REG_L       $r23 , TP , XGREG(23)
    REG_L       $r24 , TP , XGREG(24)
    REG_L       $r25 , TP , XGREG(25)
    REG_L       $r26 , TP , XGREG(26)
    REG_L       $r27 , TP , XGREG(27)
    REG_L       $r28 , TP , XGREG(28)
    REG_L       $r29 , TP , XGREG(29)
    REG_L       $r30 , TP , XGREG(30)
    REG_L       $r31 , TP , XGREG(31)
    ;/*
    ; * �ָ� RA GP SP A0 FP ($1-$4 $22)
    ; */
    REG_L       $r1  , TP , XGREG(1)
    REG_L       $r2  , TP , XGREG(2)
    REG_L       $r3  , TP , XGREG(3)
    REG_L       $r4  , TP , XGREG(4)
    REG_L       $r22 , TP , XGREG(22)

#if LW_CFG_CPU_FAST_TLS > 0
    REG_L       $r21 , TP , XGREG(21)
#endif

    ERTN                                                                ;/*  ����                        */
    MACRO_END()

;/*********************************************************************************************************
;  ���� $0 - $31 �Ĵ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_GREGS)
    ;/*
    ; * $0 �̶�Ϊ 0
    ; */
    REG_S       $r1  , TP , XGREG(1)
    REG_S       $r2  , TP , XGREG(2)
    REG_S       $r3  , TP , XGREG(3)
    REG_S       $r4  , TP , XGREG(4)
    REG_S       $r5  , TP , XGREG(5)
    REG_S       $r6  , TP , XGREG(6)
    REG_S       $r7  , TP , XGREG(7)
    REG_S       $r8  , TP , XGREG(8)
    REG_S       $r9  , TP , XGREG(9)
    REG_S       $r10 , TP , XGREG(10)
    REG_S       $r11 , TP , XGREG(11)
    REG_S       $r12 , TP , XGREG(12)
    REG_S       $r13 , TP , XGREG(13)
    REG_S       $r14 , TP , XGREG(14)
    REG_S       $r15 , TP , XGREG(15)
    REG_S       $r16 , TP , XGREG(16)
    REG_S       $r17 , TP , XGREG(17)
    REG_S       $r18 , TP , XGREG(18)
    REG_S       $r19 , TP , XGREG(19)
    REG_S       $r20 , TP , XGREG(20)
    REG_S       $r21 , TP , XGREG(21)
    REG_S       $r22 , TP , XGREG(22)
    REG_S       $r23 , TP , XGREG(23)
    REG_S       $r24 , TP , XGREG(24)
    REG_S       $r25 , TP , XGREG(25)
    REG_S       $r26 , TP , XGREG(26)
    REG_S       $r27 , TP , XGREG(27)
    REG_S       $r28 , TP , XGREG(28)
    REG_S       $r29 , TP , XGREG(29)
    REG_S       $r30 , TP , XGREG(30)
    REG_S       $r31 , TP , XGREG(31)
    MACRO_END()

;/*********************************************************************************************************
;  �ָ� $0 - $31 �Ĵ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_GREGS)
    ;/*
    ; * $0 �̶�Ϊ 0
    ; */
    REG_L       $r1  , TP , XGREG(1)
    REG_L       $r2  , TP , XGREG(2)
    REG_L       $r3  , TP , XGREG(3)
    REG_L       $r4  , TP , XGREG(4)
    REG_L       $r5  , TP , XGREG(5)
    REG_L       $r6  , TP , XGREG(6)
    REG_L       $r7  , TP , XGREG(7)
    REG_L       $r8  , TP , XGREG(8)
    REG_L       $r9  , TP , XGREG(9)
    REG_L       $r10 , TP , XGREG(10)
    REG_L       $r11 , TP , XGREG(11)
    REG_L       $r12 , TP , XGREG(12)
    REG_L       $r13 , TP , XGREG(13)
    REG_L       $r14 , TP , XGREG(14)
    REG_L       $r15 , TP , XGREG(15)
    REG_L       $r16 , TP , XGREG(16)
    REG_L       $r17 , TP , XGREG(17)
    REG_L       $r18 , TP , XGREG(18)
    REG_L       $r19 , TP , XGREG(19)
    REG_L       $r20 , TP , XGREG(20)
    ;/*
    ; * $21 �� TP(�����ָ�)
    ; */
    REG_L       $r22 , TP , XGREG(22)
    REG_L       $r23 , TP , XGREG(23)
    REG_L       $r24 , TP , XGREG(24)
    REG_L       $r25 , TP , XGREG(25)
    REG_L       $r26 , TP , XGREG(26)
    REG_L       $r27 , TP , XGREG(27)
    REG_L       $r28 , TP , XGREG(28)
    REG_L       $r29 , TP , XGREG(29)
    REG_L       $r30 , TP , XGREG(30)
    REG_L       $r31 , TP , XGREG(31)
    MACRO_END()

;/*********************************************************************************************************
;  ����������ļĴ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(SAVE_BIG_REG_CTX)
    SAVE_GREGS                                                          ;/*  ���� $0 - $31 �Ĵ���        */

    REG_S       ZERO , TP , CTX_TYPE_OFFSET                             ;/*  ���Ϊ��������              */

    REG_S       RA , TP , XPC                                           ;/*  RA ���� PC ����             */

    SAVE_CSR                                                            ;/*  ���� CSR �Ĵ���             */
    MACRO_END()

;/*********************************************************************************************************
;  �ָ��������ļĴ���(���� TP: ARCH_REG_CTX ��ַ)
;*********************************************************************************************************/

MACRO_DEF(RESTORE_BIG_REG_CTX)
    REG_L       T1 , TP , XPC
    CSRWR       T1 , CSR_ERA                                            ;/*  PC �ָ��� ERA �Ĵ���        */

    RESTORE_CSR                                                         ;/*  �ָ� CSR �Ĵ���             */

    RESTORE_GREGS                                                       ;/*  �ָ� $0 - $31 �Ĵ���        */

    REG_L       $r21 , TP , XGREG(21)                                   ;/*  �ָ� TP �Ĵ���              */

    ERTN                                                                ;/*  ����                        */
    MACRO_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
