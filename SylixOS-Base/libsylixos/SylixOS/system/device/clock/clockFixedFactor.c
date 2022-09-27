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
** 文   件   名: clockFixedFactor.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 26 日
**
** 描        述: 有固定分频或倍频值的时钟
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
static ULONG  __clockFixedFactorRateRecalc(PLW_CLOCK  pclk, ULONG  ulParentRate);
static LONG   __clockFixedFactorRateRound(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulRate);
static INT    __clockFixedFactorRateSet(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate);
/*********************************************************************************************************
  变量定义
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsFixedFactor = {
    .clockRateRecalc = __clockFixedFactorRateRecalc,
    .clockRateRound  = __clockFixedFactorRateRound,
    .clockRateSet    = __clockFixedFactorRateSet,
};
/*********************************************************************************************************
** 函数名称: __clockFixedFactorRateRecalc
** 功能描述: 获得分频或倍频时钟的频率值
** 输　入  : pclk             时钟设备
**           ulParentRate     父时钟频率
** 输　出  : 固定频率时钟的频率值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  __clockFixedFactorRateRecalc (PLW_CLOCK  pclk, ULONG  ulParentRate)
{
    PLW_CLOCK_FIXED_FACTOR  pclkfixedfactor = __CLK_TO_CLK_FIXED_FACTOR(pclk);
    UINT64                  ullRate;
    lib_lldiv_t             ullDiv;

    ullRate = (UINT64)ulParentRate * pclkfixedfactor->CLKFF_uiFixedMult;/*  父时钟倍频                  */
    ullDiv  = lib_lldiv(ullRate, pclkfixedfactor->CLKFF_uiFixedDiv);    /*  父时钟分频                  */
    ullRate = ullDiv.quot;

    return  ((ULONG)ullRate);
}
/*********************************************************************************************************
** 函数名称: __clockFixedFactorRateRound
** 功能描述: 获得最接近预设频率的频率值
** 输　入  : pclk             时钟设备
**           ulRate           预设置的频率值
**           pulParentRate    父时钟的频率值
** 输　出  : 最接近预设频率的频率值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __clockFixedFactorRateRound (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulParentRate)
{
    PLW_CLOCK_FIXED_FACTOR  pclkfixedfactor = __CLK_TO_CLK_FIXED_FACTOR(pclk);
    ULONG                   ulBestParent;

    if (__HW_FLAGS_GET(pclk) & LW_CLOCK_SET_RATE_PARENT) {              /*  同时设置父时钟的频率        */
        ulBestParent   = (ulRate / pclkfixedfactor->CLKFF_uiFixedMult) *
                         pclkfixedfactor->CLKFF_uiFixedDiv;
        *pulParentRate = API_ClockRateRound(__HW_PARENT_GET(pclk), ulBestParent);
    }

    return  ((*pulParentRate / pclkfixedfactor->CLKFF_uiFixedDiv) *
             pclkfixedfactor->CLKFF_uiFixedMult);
}
/*********************************************************************************************************
** 函数名称: __clockFixedFactorRateRound
** 功能描述: 设置频率
** 输　入  : pclk             时钟设备
**           ulRate           预设置的频率值
**           ulParentRate     父时钟的频率值
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockFixedFactorRateSet (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ClockFixedFactorRegister
** 功能描述: 注册固定分频或倍频的时钟
** 输　入  : pcName            时钟名称
**           pcParentName      父时钟名称
**           ulFlags           初始化标识
**           uiFixedMult       固定的倍频值
**           uiFixedDiv        固定的分频值
** 输　出  : 固定分频或倍频的时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockFixedFactorRegister (CPCHAR  pcName,
                                         PCHAR  *ppcParentName,
                                         ULONG   ulFlags,
                                         UINT    uiFixedMult,
                                         UINT    uiFixedDiv)
{
    PLW_CLOCK_FIXED_FACTOR   pclkfixedfactor;
    PLW_CLOCK                pclk;
    INT                      iRet;

    if (!pcName || !ppcParentName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkfixedfactor = __SHEAP_ZALLOC(sizeof(LW_CLOCK_FIXED_FACTOR));
    if (!pclkfixedfactor) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkfixedfactor->CLKFF_clk,
                                pcName,
                                &_G_clkopsFixedFactor,
                                ulFlags,
                                ppcParentName,
                                1);
    if (iRet) {
        __SHEAP_FREE(pclkfixedfactor);
        return  (LW_NULL);
    }

    pclk = &pclkfixedfactor->CLKFF_clk;
    pclkfixedfactor->CLKFF_uiFixedMult = uiFixedMult;
    pclkfixedfactor->CLKFF_uiFixedDiv  = uiFixedDiv;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkfixedfactor);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
