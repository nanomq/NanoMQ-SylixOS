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
** 文   件   名: clockFixedRate.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 26 日
**
** 描        述: 标准固定频率时钟驱动（如 Oscillator 或 Crystal）.
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
static ULONG  __clockFixedRateRecalc(PLW_CLOCK  pclk, ULONG  ulParentRate);
/*********************************************************************************************************
  变量定义
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsFixedRate = {
    .clockRateRecalc = __clockFixedRateRecalc,
};
/*********************************************************************************************************
** 函数名称: __clockFixedRateRecalc
** 功能描述: 获得固定频率时钟的频率值
** 输　入  : pclk             时钟设备
**           ulParentRate     父时钟频率，暂时无用
** 输　出  : 固定频率时钟的频率值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  __clockFixedRateRecalc (PLW_CLOCK  pclk, ULONG  ulParentRate)
{
    return  (__CLK_TO_CLK_FIXED_RATE(pclk)->CLKFR_ulFixedRate);
}
/*********************************************************************************************************
** 函数名称: API_ClockFixedRateRegister
** 功能描述: 注册一个固定频率的时钟
** 输　入  : pcName            时钟名称
**           ulFlags           时钟标识
**           ulFixedRate       固定的频率
** 输　出  : 注册的固定频率时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockFixedRateRegister (CPCHAR  pcName,
                                       ULONG   ulFlags,
                                       ULONG   ulFixedRate)
{
    PLW_CLOCK_FIXED_RATE    pclkfixedrate;
    PLW_CLOCK               pclk;
    INT                     iRet;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkfixedrate = __SHEAP_ZALLOC(sizeof(LW_CLOCK_FIXED_RATE));
    if (!pclkfixedrate) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkfixedrate->CLKFR_clk,
                                pcName,
                                &_G_clkopsFixedRate,
                                ulFlags,
                                LW_NULL,
                                0);
    if (iRet) {
        __SHEAP_FREE(pclkfixedrate);
        return  (LW_NULL);
    }

    pclk = &pclkfixedrate->CLKFR_clk;
    pclkfixedrate->CLKFR_ulFixedRate = ulFixedRate;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkfixedrate);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
