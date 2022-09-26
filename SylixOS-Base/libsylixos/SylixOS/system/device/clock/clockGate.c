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
** 描        述: 门控时钟驱动（只能控制时钟使能和禁能）.
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
static INT  __clockGateEnable(PLW_CLOCK  pclk);
static VOID __clockGateDisable(PLW_CLOCK  pclk);
static BOOL __clockGateIsEnabled(PLW_CLOCK  pclk);
/*********************************************************************************************************
  变量定义
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsGate = {
    .clockEnable    = __clockGateEnable,
    .clockDisable   = __clockGateDisable,
    .clockIsEnabled = __clockGateIsEnabled,
};
/*********************************************************************************************************
** 函数名称: __clockGateEnable
** 功能描述: 使能时钟
** 输　入  : pclk           时钟设备
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockGateEnable (PLW_CLOCK  pclk)
{
    PLW_CLOCK_GATE  pclkgate  = __CLK_TO_CLK_GATE(pclk);

    if (pclkgate->CLKG_pfuncEnable) {
        return  (pclkgate->CLKG_pfuncEnable(pclk));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __clockGateDisable
** 功能描述: 禁能时钟
** 输　入  : pclk           时钟设备
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __clockGateDisable (PLW_CLOCK  pclk)
{
    PLW_CLOCK_GATE  pclkgate  = __CLK_TO_CLK_GATE(pclk);

    if (pclkgate->CLKG_pfuncDisable) {
        pclkgate->CLKG_pfuncDisable(pclk);
    }
}
/*********************************************************************************************************
** 函数名称: __clockGateIsEnabled
** 功能描述: 判断时钟是否使能
** 输　入  : pclk           时钟设备
** 输　出  : LW_TRUE 时钟使能，LW_FALSE 时钟禁能
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL __clockGateIsEnabled (PLW_CLOCK  pclk)
{
    PLW_CLOCK_GATE  pclkgate  = __CLK_TO_CLK_GATE(pclk);

    if (pclkgate->CLKG_pfuncIsEnabled) {
        return  (pclkgate->CLKG_pfuncIsEnabled(pclk));
    }

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: API_ClockGateRegister
** 功能描述: 注册一个控制门时钟
** 输　入  : pcName            时钟名称
**           pcParentName      父时钟名称
**           ulFlags           初始化标识
**           pfuncEnable       使能操作函数
**           pfuncDisable      禁能操作函数
**           pfuncIsEnabled    是否使能判断函数
** 输　出  : 注册的控制门时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockGateRegister (CPCHAR       pcName,
                                  PCHAR       *ppcParentName,
                                  ULONG        ulFlags,
                                  FUNCPTR      pfuncEnable,
                                  FUNCPTR      pfuncDisable,
                                  BOOLFUNCPTR  pfuncIsEnabled)
{
    PLW_CLOCK_GATE       pclkgate;
    PLW_CLOCK            pclk;
    INT                  iRet;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkgate = __SHEAP_ZALLOC(sizeof(LW_CLOCK_GATE));
    if (!pclkgate) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkgate->CLKG_clk,
                                pcName,
                                &_G_clkopsGate,
                                ulFlags,
                                ppcParentName,
                                ppcParentName ? 1 : 0);
    if (iRet) {
        __SHEAP_FREE(pclkgate);
        return  (LW_NULL);
    }

    pclk = &pclkgate->CLKG_clk;
    pclkgate->CLKG_pfuncEnable    = pfuncEnable;
    pclkgate->CLKG_pfuncDisable   = pfuncDisable;
    pclkgate->CLKG_pfuncIsEnabled = pfuncIsEnabled;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkgate);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
