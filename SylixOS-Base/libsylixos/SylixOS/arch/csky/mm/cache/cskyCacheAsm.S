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
;** 文   件   名: cskyCacheAsm.S
;**
;** 创   建   人: Hui.Kai (惠凯)
;**
;** 文件创建日期: 2015 年 12 月 01 日
;**
;** 描        述: C-SKY 体系构架 CACHE 驱动.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>
#include <arch/csky/arch_def.h>
#include <config/kernel/cache_cfg.h>
#include <config/cpu/cpu_cfg_csky.h>

#if !defined(__SYLIXOS_CSKY_ARCH_CK803__)

#if LW_CFG_CACHE_EN > 0

    EXPORT_LABEL(cskyICacheInvalidateAll)
    EXPORT_LABEL(cskyICacheEnableHw)
    EXPORT_LABEL(cskyICacheDisableHw)

    EXPORT_LABEL(cskyDCacheInvalidateAll)
    EXPORT_LABEL(cskyDCacheClearAll)
    EXPORT_LABEL(cskyDCacheFlushAll)
    EXPORT_LABEL(cskyDCacheDisableHw)
    EXPORT_LABEL(cskyDCacheEnableHw)

    EXPORT_LABEL(cskyBranchPredictorInvalidate)
    EXPORT_LABEL(cskyBranchPredictionEnable)
    EXPORT_LABEL(cskyBranchPredictionDisable)

    FILE_BEGIN()

;/*********************************************************************************************************
;  无效所有 ICACHE
;*********************************************************************************************************/

FUNC_DEF(cskyICacheInvalidateAll)
    MOVI        A0 , B_CFR_CACHE_I | M_CFR_INV
    MTCR        A0 , CR17
    RTS
    FUNC_END(cskyICacheInvalidateAll)

;/*********************************************************************************************************
;  无效所有 DCACHE
;*********************************************************************************************************/

FUNC_DEF(cskyDCacheInvalidateAll)
    MOVI        A0 , B_CFR_CACHE_D | M_CFR_INV
    MTCR        A0 , CR17
    RTS
    FUNC_END(cskyDCacheInvalidateAll)

;/*********************************************************************************************************
;  CLEAR 所有 DCACHE
;*********************************************************************************************************/

FUNC_DEF(cskyDCacheClearAll)
    MOVI        A0 , B_CFR_CACHE_D | M_CFR_CLR | M_CFR_INV
    MTCR        A0 , CR17
    RTS
    FUNC_END(cskyDCacheClearAll)

;/*********************************************************************************************************
;  FLUSH 所有 DCACHE
;*********************************************************************************************************/

FUNC_DEF(cskyDCacheFlushAll)
    MOVI        A0 , B_CFR_CACHE_D | M_CFR_CLR
    MTCR        A0 , CR17
    RTS
    FUNC_END(cskyDCacheFlushAll)

;/*********************************************************************************************************
;  禁能 ICACHE
;*********************************************************************************************************/

FUNC_DEF(cskyICacheDisableHw)
    MOVI        A0 , B_CFR_CACHE_I | M_CFR_INV
    MTCR        A0 , CR17

    MFCR        A0 , CR18
    BCLRI       A0 , S_CACHE_CFG_IE
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyICacheDisableHw)

;/*********************************************************************************************************
;  禁能 DCACHE
;*********************************************************************************************************/

FUNC_DEF(cskyDCacheDisableHw)
    MOVI        A0 , B_CFR_CACHE_D | M_CFR_CLR | M_CFR_INV
    MTCR        A0 , CR17

    MFCR        A0 , CR18
    BCLRI       A0 , S_CACHE_CFG_DE
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyDCacheDisableHw)

;/*********************************************************************************************************
;  使能 ICACHE
;*********************************************************************************************************/

FUNC_DEF(cskyICacheEnableHw)
    MOVI        A0 , B_CFR_CACHE_I | M_CFR_INV
    MTCR        A0 , CR17

    MFCR        A0 , CR18
    BSETI       A0 , S_CACHE_CFG_IE
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyICacheEnableHw)

;/*********************************************************************************************************
;  使能 DCACHE
;*********************************************************************************************************/

FUNC_DEF(cskyDCacheEnableHw)
    MOVI        A1 , B_CFR_CACHE_D | M_CFR_CLR | M_CFR_INV
    MTCR        A1 , CR17

    MFCR        A1 , CR18
    BSETI       A1 , S_CACHE_CFG_DE                                     ;/*  DE 数据高速缓存设置位       */
    BCLRI       A1 , S_CACHE_CFG_WB                                     ;/*  WB 高速缓存写回设置位       */
    BCLRI       A1 , S_CACHE_CFG_WA                                     ;/*  WA 高速缓存写分配有效设置   */
    OR          A1 , A0

#ifdef __CK860__
    BCLRI       A1 , S_CACHE_CFG_WB                                     ;/*  CK860 2.4 手册上此位为 0    */
#endif

    MTCR        A1 , CR18
    RTS
    FUNC_END(cskyDCacheEnableHw)

;/*********************************************************************************************************
;  无效分支预测缓冲
;*********************************************************************************************************/

FUNC_DEF(cskyBranchPredictorInvalidate)
    MFCR        A0 , CR17
    BSETI       A0 , S_CFR_BHT_INV
    BSETI       A0 , S_CFR_BTB_INV
    MTCR        A0 , CR17
    RTS
    FUNC_END(cskyBranchPredictorInvalidate)

;/*********************************************************************************************************
;  使能分支预测
;*********************************************************************************************************/

FUNC_DEF(cskyBranchPredictionEnable)
    MOVIH       A0 , B_CFR_CACHE_A                                      ;/*  无效分支预测缓冲            */
    MTCR        A0 , CR17

    MFCR        A0 , CR18
    BSETI       A0 , S_CACHE_CFG_BTB                                    ;/*  BTB 分支目标缓冲器使能      */
    BSETI       A0 , S_CACHE_CFG_Z                                      ;/*  Z 允许预测跳转设置位        */
    BSETI       A0 , S_CACHE_CFG_RS                                     ;/*  RS 地址返回栈设置位         */
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyBranchPredictionEnable)

;/*********************************************************************************************************
;  关闭分支预测
;*********************************************************************************************************/

FUNC_DEF(cskyBranchPredictionDisable)
    MOVIH       A0 , B_CFR_CACHE_A                                      ;/*  无效分支预测缓冲            */
    MTCR        A0 , CR17

    MFCR        A0 , CR18
    BCLRI       A0 , S_CACHE_CFG_BTB
#ifndef __CK807__
    BCLRI       A0 , S_CACHE_CFG_Z
#endif
    BCLRI       A0 , S_CACHE_CFG_RS
    MTCR        A0 , CR18
    RTS
    FUNC_END(cskyBranchPredictionDisable)

    FILE_END()

#endif
#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/
