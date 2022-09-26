;/*********************************************************************************************************
;**
;**                                    中国软件开源组织
;**
;**                                   嵌入式实时操作系统
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------文件信息--------------------------------------------------------------------------------
;**
;** 文   件   名: mipsHr2VectorAsm.S
;**
;** 创   建   人: Jiao.JinXing (焦进星)
;**
;** 文件创建日期: 2018 年 02 月 24 日
;**
;** 描        述: 华睿 2 号处理器向量运算单元支持.
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
  向量控制寄存器
*********************************************************************************************************/

#define CP2_VIR                 $0
#define CP2_VFCR                $1
#define CP2_VENR                $2
#define CP2_VFR                 $3
#define CP2_VSCR                $4
#define CP2_VCCR                $8

;/*********************************************************************************************************
;  VCCR 偏移
;*********************************************************************************************************/

#define VECTOR_OFFSET_VCCR      (32 * (256 / 8))                        /*  前面有 32 个 256 位宽寄存器  */

;/*********************************************************************************************************
;  VSCR 寄存器相关定义
;*********************************************************************************************************/

#define VECTOR_VSCR_FS          (1 << 8)                                /*  非规格化结果被置为 0         */

;/*********************************************************************************************************
;  使能向量运算单元(只会破坏 T0 寄存器)
;*********************************************************************************************************/

MACRO_DEF(ENABLE_VECTOR)
    MFC0_EHB(T0, CP0_STATUS)
    OR      T0 , T0 , ST0_CU2
    MTC0_EHB(T0, CP0_STATUS)
    MACRO_END()

;/*********************************************************************************************************
;  初始化向量运算单元
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorInit)
    MFC0_EHB(T1, CP0_STATUS)                                            ;/*  T1 = 原 VECTOR 开关状态     */

    ENABLE_VECTOR                                                       ;/*  使能 VECTOR                 */

    PTR_LA  T0 , mipsHr2VectorRegZero

    VLDDQ   $z0  , 0(T0)                                                ;/*  清零 VECTOR 寄存器          */
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

    CTC2_EHB(ZERO, CP2_VCCR)                                            ;/*  VCC 清零                    */

    LI      T0 , VECTOR_VSCR_FS                                         ;/*  非规格化结果被置为 0        */
    CTC2_EHB(T0, CP2_VSCR)

    MTC0_EHB(T1 , CP0_STATUS)                                           ;/*  恢复原 VECTOR 开关状态      */
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorInit)

;/*********************************************************************************************************
;  使能向量运算单元
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorEnable)
    ENABLE_VECTOR                                                       ;/*  使能 VECTOR                 */
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorEnable)

;/*********************************************************************************************************
;  禁能向量运算单元
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorDisable)
    MFC0_EHB(T0, CP0_STATUS)
    AND     T0 , T0 , ~ST0_CU2
    MTC0_EHB(T0, CP0_STATUS)
    JR      RA
    NOP
    FUNC_END(mipsHr2VectorDisable)

;/*********************************************************************************************************
;  判断向量运算单元是否使能
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
;  保存向量运算单元寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorSave)
    ENABLE_VECTOR                                                       ;/*  使能 VECTOR                 */

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
    SW      T1   , VECTOR_OFFSET_VCCR(A0)                              ;/*  保存 VCCR 寄存器             */

    MIPS_EHB

    JR      RA
    NOP
    FUNC_END(mipsHr2VectorSave)

;/*********************************************************************************************************
;  恢复向量运算单元寄存器
;*********************************************************************************************************/

FUNC_DEF(mipsHr2VectorRestore)
    ENABLE_VECTOR                                                       ;/*  使能 VECTOR                 */

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

    LW      T1   , VECTOR_OFFSET_VCCR(A0)                              ;/*  恢复 VCCR 寄存器             */
    CTC2_EHB(T1,  CP2_VCCR)

    JR      RA
    NOP
    FUNC_END(mipsHr2VectorRestore)

;/*********************************************************************************************************
;  用于向量寄存器清零
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
