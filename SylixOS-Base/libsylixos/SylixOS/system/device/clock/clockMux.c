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
** 文   件   名: clockGate.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 27 日
**
** 描        述: 多选一时钟驱动（控制父时钟选择）.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "clock.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static UINT8  __clockMuxParentGet(PLW_CLOCK  pclk);
static INT    __clockMuxParentSet(PLW_CLOCK  pclk, UINT8  ucIndex);
/*********************************************************************************************************
  变量定义
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsMux = {
    .clockParentGet = __clockMuxParentGet,
    .clockParentSet = __clockMuxParentSet,
};
/*********************************************************************************************************
** 函数名称: __clockMuxParentGet
** 功能描述: 获得当前选中的父时钟序号
** 输　入  : pclk           时钟设备
** 输　出  : 当前选中的父时钟序号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT8  __clockMuxParentGet (PLW_CLOCK  pclk)
{
    PLW_CLOCK_MUX        pclkmux      = __CLK_TO_CLK_MUX(pclk);
    PLW_CLOCK_MUX_TABLE  pclkmuxtable = pclkmux->CLKM_pclktable;
    UINT                 uiNumParents = pclk->CLK_uiNumParents;
    UINT                 uiVal        = pclkmux->CLKM_pfuncValGet(pclk);
    UINT                 i;

    uiVal &= pclkmux->CLKM_uiMask;
    uiVal  = uiVal >> pclkmux->CLKM_uiShift;

    if (pclk->CLK_ulFlags & LW_CLOCK_IS_MUX) {
        return  (uiVal);
    }

    for (i = 0; i < uiNumParents; i++) {
        if (pclkmuxtable[i].CLKMT_uiVal == uiVal) {
            return  (pclkmuxtable[i].CLKMT_uiSource);
        }
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __clockMuxParentSet
** 功能描述: 设置当前的父时钟序号
** 输　入  : pclk             时钟设备
**           ucIndex          对应的父时钟序号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockMuxParentSet (PLW_CLOCK  pclk, UINT8  ucIndex)
{
    PLW_CLOCK_MUX        pclkmux      = __CLK_TO_CLK_MUX(pclk);
    PLW_CLOCK_MUX_TABLE  pclkmuxtable = pclkmux->CLKM_pclktable;
    UINT                 uiNumParents = pclk->CLK_uiNumParents;
    UINT                 uiVal        = pclkmux->CLKM_pfuncValGet(pclk);
    UINT                 uiRegVal;
    UINT                 i;

    for (i = 0; i < uiNumParents; i++) {
        if (pclkmuxtable[i].CLKMT_uiSource == ucIndex) {
            uiVal = pclkmuxtable[i].CLKMT_uiVal;
        }
    }

    uiVal     = uiVal << pclkmux->CLKM_uiShift;
    uiRegVal  = pclkmux->CLKM_pfuncValGet(pclk);
    uiRegVal &= ~(pclkmux->CLKM_uiMask << pclkmux->CLKM_uiShift);
    uiRegVal |= uiVal;

    return  (pclkmux->CLKM_pfuncValSet(pclk, uiRegVal));
}
/*********************************************************************************************************
** 函数名称: API_ClockDividerRegister
** 功能描述: 注册可变分频的时钟
** 输　入  : pcName            时钟名称
**           pcParentName      父时钟名称
**           ulFlags           初始化标识
**           pclkmuxtable      分频表
**           uiMask            掩码
**           uiShift           移位
**           pfuncValGet       寄存器值获取函数
**           pfuncValSet       寄存器值设置函数
** 输　出  : 可变分频的时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockMuxRegister (CPCHAR               pcName,
                                 PCHAR               *ppcParentName,
                                 ULONG                ulFlags,
                                 UINT                 uiParentNum,
                                 PLW_CLOCK_MUX_TABLE  pclkmuxtable,
                                 UINT                 uiMask,
                                 UINT                 uiShift,
                                 UINTFUNCPTR          pfuncValGet,
                                 FUNCPTR              pfuncValSet)
{
    PLW_CLOCK_MUX       pclkmux;
    PLW_CLOCK           pclk;
    INT                 iRet;

    if (!pcName || !ppcParentName || !pclkmuxtable || !pfuncValGet || !pfuncValSet) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkmux = __SHEAP_ZALLOC(sizeof(LW_CLOCK_MUX));
    if (!pclkmux) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkmux->CLKM_clk,
                                pcName,
                                &_G_clkopsMux,
                                ulFlags,
                                ppcParentName,
                                uiParentNum);
    if (iRet) {
        __SHEAP_FREE(pclkmux);
        return  (LW_NULL);
    }

    pclk = &pclkmux->CLKM_clk;
    pclkmux->CLKM_pclktable   = pclkmuxtable;
    pclkmux->CLKM_uiMask      = uiMask;
    pclkmux->CLKM_uiShift     = uiShift;
    pclkmux->CLKM_pfuncValGet = pfuncValGet;
    pclkmux->CLKM_pfuncValSet = pfuncValSet;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkmux);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
