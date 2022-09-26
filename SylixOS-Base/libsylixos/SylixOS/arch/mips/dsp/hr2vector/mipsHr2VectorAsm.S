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
;** ��   ��   ��: mipsHr2VectorAsm.S
;**
;** ��   ��   ��: Jiao.JinXing (������)
;**
;** �ļ���������: 2018 �� 02 �� 24 ��
;**
;** ��        ��: ��� 2 �Ŵ������������㵥Ԫ֧��.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if (defined(_MIPS_ARCH_HR2) || defined(_MIPS_ARCH_HCW)) && LW_CFG_CPU_DSP_EN > 0

#include "arch/mips/arch_dsp.h"

    FILE_BEGIN()

    EXPORT_LABEL(mipsHr2VectorInit)
    EXPORT_LABEL(mipsHr2VectorEnable)
    EXPORT_LABEL(mipsHr2VectorDisable)
    EXPORT_LABEL(mipsHr2VectorIsEnable)
    EXPORT_LABEL(mipsHr2VectorSave)
    EXPORT_LABEL(mipsHr2VectorRestore)

/*********************************************************************************************************
  �������ƼĴ���
*********************************************************************************************************/

#define CP2_VIR                 $0
#define CP2_VFCR                $1
#define CP2_VENR                $2
#define CP2_VFR                 $3
#define CP2_VSCR                $4
#define CP2_VCCR                $8

;/*********************************************************************************************************
;  VCCR ƫ��
;*********************************************************************************************************/

#define VECTOR_OFFSET_VCCR      (32 * (256 / 8))                        /*  ǰ���� 32 �� 256 λ���Ĵ���  */

;/*********************************************************************************************************
;  VSCR �Ĵ�����ض���
;*********************************************************************************************************/

#define VECTOR_VSCR_FS          (1 << 8)                                /*  �ǹ�񻯽������Ϊ 0         */

;/*********************************************************************************************************
;  ʹ���������㵥Ԫ(ֻ���ƻ� T0 �Ĵ���)
;*********************************************************************************************************/

MACRO_DEF(ENABLE_VECTOR)
    MFC0_EHB(T0, CP0_STATUS)
    OR      T0 , T0 , ST0_CU2
    MTC0_EHB(T0, CP0_STATUS)
    MACRO_END()

;/*********************************************************************************************************
;  ��ʼ���������㵥Ԫ
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorInit)
    MFC0_EHB(T1, CP0_STATUS)                                            ;/*  T1 = ԭ VECTOR ����״̬     */

    ENABLE_VECTOR                                                       ;/*  ʹ�� VECTOR                 */

    PTR_LA  T0 , mipsHr2VectorRegZero

    VLDDQ   $z0  , 0(T0)                                                ;/*  ���� VECTOR �Ĵ���          */
    VLDDQ   $z1  , 0(T0)
    VLDDQ   $z2  , 0(T0)
    VLDDQ   $z3  , 0(T0)
    VLDDQ   $z4  , 0(T0)
    VLDDQ   $z5  , 0(T0)
    VLDDQ   $z6  , 0(T0)
    VLDDQ   $z7  , 0(T0)
    VLDDQ   $z8  , 0(T0)
    VLDDQ   $z9  , 0(T0)
    VLDDQ   $z10 , 0(T0)
    VLDDQ   $z11 , 0(T0)
    VLDDQ   $z12 , 0(T0)
    VLDDQ   $z13 , 0(T0)
    VLDDQ   $z14 , 0(T0)
    VLDDQ   $z15 , 0(T0)
    VLDDQ   $z16 , 0(T0)
    VLDDQ   $z17 , 0(T0)
    VLDDQ   $z18 , 0(T0)
    VLDDQ   $z19 , 0(T0)
    VLDDQ   $z20 , 0(T0)
    VLDDQ   $z21 , 0(T0)
    VLDDQ   $z22 , 0(T0)
    VLDDQ   $z23 , 0(T0)
    VLDDQ   $z24 , 0(T0)
    VLDDQ   $z25 , 0(T0)
    VLDDQ   $z26 , 0(T0)
    VLDDQ   $z27 , 0(T0)
    VLDDQ   $z28 , 0(T0)
    VLDDQ   $z29 , 0(T0)
    VLDDQ   $z30 , 0(T0)
    VLDDQ   $z31 , 0(T0)

    CTC2_EHB(ZERO, CP2_VCCR)                                            ;/*  VCC ����                    */

    LI      T0 , VECTOR_VSCR_FS                                         ;/*  �ǹ�񻯽������Ϊ 0        */
    CTC2_EHB(T0, CP2_VSCR)

    MTC0_EHB(T1 , CP0_STATUS)                                           ;/*  �ָ�ԭ VECTOR ����״̬      */
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorInit)

;/*********************************************************************************************************
;  ʹ���������㵥Ԫ
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorEnable)
    ENABLE_VECTOR                                                       ;/*  ʹ�� VECTOR                 */
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorEnable)

;/*********************************************************************************************************
;  �����������㵥Ԫ
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorDisable)
    MFC0_EHB(T0, CP0_STATUS)
    AND     T0 , T0 , ~ST0_CU2
    MTC0_EHB(T0, CP0_STATUS)
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorDisable)

;/*********************************************************************************************************
;  �ж��������㵥Ԫ�Ƿ�ʹ��
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorIsEnable)
    MOV     V0 , ZERO                                                   ;/*  FALSE return value          */
    MFC0_EHB(T0, CP0_STATUS)
    AND     T0 , T0 , ST0_CU2                                           ;/*  VECTOR enabled              */
    BEQ     T0 , ZERO, 1f
    NOP
    LI      V0 , 1                                                      ;/*  TRUE return value           */
1:
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorIsEnable)

;/*********************************************************************************************************
;  �����������㵥Ԫ�Ĵ���
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorSave)
    ENABLE_VECTOR                                                       ;/*  ʹ�� VECTOR                 */

    VSTDQ   $z0  , 0  (A0)
    VSTDQ   $z1  , 32 (A0)
    VSTDQ   $z2  , 64 (A0)
    VSTDQ   $z3  , 96 (A0)
    VSTDQ   $z4  , 128(A0)
    VSTDQ   $z5  , 160(A0)
    VSTDQ   $z6  , 192(A0)
    VSTDQ   $z7  , 224(A0)
    VSTDQ   $z8  , 256(A0)
    VSTDQ   $z9  , 288(A0)
    VSTDQ   $z10 , 320(A0)
    VSTDQ   $z11 , 352(A0)
    VSTDQ   $z12 , 384(A0)
    VSTDQ   $z13 , 416(A0)
    VSTDQ   $z14 , 448(A0)
    VSTDQ   $z15 , 480(A0)
    VSTDQ   $z16 , 512(A0)
    VSTDQ   $z17 , 544(A0)
    VSTDQ   $z18 , 576(A0)
    VSTDQ   $z19 , 608(A0)
    VSTDQ   $z20 , 640(A0)
    VSTDQ   $z21 , 672(A0)
    VSTDQ   $z22 , 704(A0)
    VSTDQ   $z23 , 736(A0)
    VSTDQ   $z24 , 768(A0)
    VSTDQ   $z25 , 800(A0)
    VSTDQ   $z26 , 832(A0)
    VSTDQ   $z27 , 864(A0)
    VSTDQ   $z28 , 896(A0)
    VSTDQ   $z29 , 928(A0)
    VSTDQ   $z30 , 960(A0)
    VSTDQ   $z31 , 992(A0)

    CFC2_EHB(T1,  CP2_VCCR)
    SW      T1   , VECTOR_OFFSET_VCCR(A0)                              ;/*  ���� VCCR �Ĵ���             */

    MIPS_EHB

    JR      RA
    NOP
    FUNC_END(mipsHr2VectorSave)

;/*********************************************************************************************************
;  �ָ��������㵥Ԫ�Ĵ���
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorRestore)
    ENABLE_VECTOR                                                       ;/*  ʹ�� VECTOR                 */

    VLDDQ   $z0  , 0  (A0)
    VLDDQ   $z1  , 32 (A0)
    VLDDQ   $z2  , 64 (A0)
    VLDDQ   $z3  , 96 (A0)
    VLDDQ   $z4  , 128(A0)
    VLDDQ   $z5  , 160(A0)
    VLDDQ   $z6  , 192(A0)
    VLDDQ   $z7  , 224(A0)
    VLDDQ   $z8  , 256(A0)
    VLDDQ   $z9  , 288(A0)
    VLDDQ   $z10 , 320(A0)
    VLDDQ   $z11 , 352(A0)
    VLDDQ   $z12 , 384(A0)
    VLDDQ   $z13 , 416(A0)
    VLDDQ   $z14 , 448(A0)
    VLDDQ   $z15 , 480(A0)
    VLDDQ   $z16 , 512(A0)
    VLDDQ   $z17 , 544(A0)
    VLDDQ   $z18 , 576(A0)
    VLDDQ   $z19 , 608(A0)
    VLDDQ   $z20 , 640(A0)
    VLDDQ   $z21 , 672(A0)
    VLDDQ   $z22 , 704(A0)
    VLDDQ   $z23 , 736(A0)
    VLDDQ   $z24 , 768(A0)
    VLDDQ   $z25 , 800(A0)
    VLDDQ   $z26 , 832(A0)
    VLDDQ   $z27 , 864(A0)
    VLDDQ   $z28 , 896(A0)
    VLDDQ   $z29 , 928(A0)
    VLDDQ   $z30 , 960(A0)
    VLDDQ   $z31 , 992(A0)

    LW      T1   , VECTOR_OFFSET_VCCR(A0)                              ;/*  �ָ� VCCR �Ĵ���             */
    CTC2_EHB(T1,  CP2_VCCR)

    JR      RA
    NOP
    FUNC_END(mipsHr2VectorRestore)

;/*********************************************************************************************************
;  ���������Ĵ�������
;*********************************************************************************************************/

    SECTION(.bss)
    .balign     32
LINE_LABEL(mipsHr2VectorRegZero)
    .space      32

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/