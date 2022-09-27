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
** 文   件   名: clockDivider.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 26 日
**
** 描        述: 可变分频值的时钟
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
  宏定义
*********************************************************************************************************/
#define DIV_ROUND_DOWN_ULL(ll, d)     \
    ({                                \
         lib_lldiv_t     ullDiv;      \
         ULONG           ullTmp;      \
         ullDiv  = lib_lldiv(ll, d);  \
         ullTmp  = ullDiv.quot;       \
         ullTmp;                      \
    })
#define DIV_ROUND_UP_ULL(ll, d)     DIV_ROUND_DOWN_ULL((ll) + (d) - 1, (d))
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
static ULONG  __clockDividerRateRecalc(PLW_CLOCK  pclk, ULONG  ulParentRate);
static LONG   __clockDividerRateRound(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulRate);
static INT    __clockDividerRateSet(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate);
/*********************************************************************************************************
  变量定义
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsDivider = {
    .clockRateRecalc = __clockDividerRateRecalc,
    .clockRateRound  = __clockDividerRateRound,
    .clockRateSet    = __clockDividerRateSet,
};
/*********************************************************************************************************
** 函数名称: __clockIsBestDiv
** 功能描述: 判断是否是最合适的分频值
** 输　入  : ulRate           预期分频数值
**           ulNow            当前分频数值
**           ulBest           当前最佳分频数值
**           ulFlags          时钟标志
** 输　出  : LW_TRUE 为最佳分频值，LW_FALSE 不为最佳分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL __clockIsBestDiv (ULONG  ulRate, ULONG  ulNow, ULONG  ulBest, ULONG  ulFlags)
{
    if (ulFlags & LW_CLOCK_DIVIDER_ROUND_CLOSEST) {                     /*  如果采用最接近的分频值      */
        return  (lib_labs(ulRate - ulNow) < lib_labs(ulRate - ulBest));
    }

    return  (ulNow <= ulRate) && (ulNow > ulBest);
}
/*********************************************************************************************************
** 函数名称: __clockIsValidDiv
** 功能描述: 判断是否是有效的分频值
** 输　入  : pclkdivtable     分频表
**           uiDiv            分频值
** 输　出  : LW_TRUE 为有效分频值，LW_FALSE 不为有效分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL __clockIsValidDiv (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: __clockTableMinDivGet
** 功能描述: 获取表中最小分频值
** 输　入  : pclkdivtable        分频表
** 输　出  : 最大分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableMinDivGet (PLW_CLOCK_DIV_TABLE  pclkdivtable)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiMinDiv = UINT_MAX;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv < uiMinDiv) {
            uiMinDiv = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiMinDiv);
}
/*********************************************************************************************************
** 函数名称: __clockTableDivRoundUpGet
** 功能描述: 获得表中最接近预期分频值且大于预期分频值的分频值
** 输　入  : pclkdivtable     分频表
**           uiDiv            预期分频值
** 输　出  : 最接近预期分频值且大于预期分频值的分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableDivRoundUpGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiUp = UINT_MAX;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {                        /*  找到最匹配的分频值          */
            return  (pclkdivitem->CLKDT_uiDiv);

        } else if (pclkdivitem->CLKDT_uiDiv < uiDiv) {                  /*  表项分频值比预期分频值小    */
            continue;
        }

        if ((pclkdivitem->CLKDT_uiDiv - uiDiv) < (uiUp - uiDiv)) {      /*  表项值与预期值差距更小      */
            uiUp = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiUp);
}
/*********************************************************************************************************
** 函数名称: __clockTableDivRoundDownGet
** 功能描述: 获得表中最接近预期分频值且小于预期分频值的分频值
** 输　入  : pclkdivtable     分频表
**           uiDiv            预期分频值
** 输　出  : 最接近预期分频值且小于预期分频值的分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableDivRoundDownGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiDown = __clockTableMinDivGet(pclkdivtable);

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {                        /*  找到最匹配的分频值          */
            return  (pclkdivitem->CLKDT_uiDiv);

        } else if (pclkdivitem->CLKDT_uiDiv > uiDiv) {                  /*  表项分频值比预期分频值大    */
            continue;
        }

        if ((uiDiv - pclkdivitem->CLKDT_uiDiv) < (uiDiv - uiDown)) {    /*  表项值与预期值差距更小      */
            uiDown = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiDown);
}
/*********************************************************************************************************
** 函数名称: __clockDivRoundClosest
** 功能描述: 获得表中最接近预期分频值的分频值
** 输　入  : pclkdivtable     分频表
**           ulParentRate     父时钟频率
**           ulRate           预期分频值
** 输　出  : 最接近预期分频值的分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockDivRoundClosest (PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                     ULONG                ulParentRate,
                                     ULONG                ulRate)
{
    ULONG  ulUpRate;
    ULONG  ulDownRate;
    UINT   uiDivUp;
    UINT   uiDivDown;

    uiDivUp    = DIV_ROUND_UP_ULL((UINT64)ulParentRate, ulRate);        /*  分频值的上界                */
    uiDivDown  = ulParentRate / ulRate;                                 /*  分频值的下界                */

    uiDivUp    = __clockTableDivRoundUpGet(pclkdivtable, uiDivUp);
    uiDivDown  = __clockTableDivRoundDownGet(pclkdivtable, uiDivDown);

    ulUpRate   = DIV_ROUND_UP_ULL((UINT64)ulParentRate, uiDivUp);
    ulDownRate = DIV_ROUND_UP_ULL((UINT64)ulParentRate, uiDivDown);

    return  ((ulRate - ulUpRate) <= (ulDownRate - ulRate) ? uiDivUp : uiDivDown);
}
/*********************************************************************************************************
** 函数名称: __clockDivRound
** 功能描述: 获得最接近的分频值
** 输　入  : pclkdivtable        分频表
**           ulParentRate     父时钟频率
**           ulRate           本时钟频率
**           ulFlags          时钟标志
** 输　出  : 最接近的分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockDivRound (PLW_CLOCK_DIV_TABLE  pclkdivtable,
                              ULONG                ulParentRate,
                              ULONG                ulRate,
                              ULONG                ulFlags)
{
    UINT  uiDiv;

    if (ulFlags & LW_CLOCK_DIVIDER_ROUND_CLOSEST) {                     /*  如果采用最接近的分频值      */
        return  (__clockDivRoundClosest(pclkdivtable, ulParentRate, ulRate));
    }

    uiDiv = DIV_ROUND_UP_ULL((UINT64)ulParentRate, ulRate);

    return  (__clockTableDivRoundUpGet(pclkdivtable, uiDiv));           /*  采用略大于的分频值          */
}
/*********************************************************************************************************
** 函数名称: __clockTableValGet
** 功能描述: 通过分频值获取寄存器数值
** 输　入  : pclkdivtable     分频表
**           uiDiv            分频值
** 输　出  : 寄存器数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableValGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {
            return  (pclkdivitem->CLKDT_uiVal);
        }
    }

    return  (0);
}
/*********************************************************************************************************
** 函数名称: __clockTableDivGet
** 功能描述: 通过寄存器数值获取分频值
** 输　入  : pclkdivtable     分频表
**           uiVal            寄存器数值
** 输　出  : 分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableDivGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiVal)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiVal == uiVal) {
            return  (pclkdivitem->CLKDT_uiDiv);
        }
    }

    return  (1);
}
/*********************************************************************************************************
** 函数名称: __clockTableMaxDivGet
** 功能描述: 获取表中最大分频值
** 输　入  : pclkdivtable        分频表
** 输　出  : 最大分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableMaxDivGet (PLW_CLOCK_DIV_TABLE  pclkdivtable)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiMaxDiv = 0;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv > uiMaxDiv) {
            uiMaxDiv = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiMaxDiv);
}
/*********************************************************************************************************
** 函数名称: __clockTableBestDivGet
** 功能描述: 获得最合适的分频值
** 输　入  : pclk               时钟设备
**           pclkParent         父时钟设备
**           ulRate             时钟频率
**           pulBestParentRate  最适合的父时钟频率
**           pclkdivtable       分频表
** 输　出  : 最合适的分频值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __clockTableBestDivGet (PLW_CLOCK            pclk,
                                     PLW_CLOCK            pclkParent,
                                     ULONG                ulRate,
                                     ULONG               *pulBestParentRate,
                                     PLW_CLOCK_DIV_TABLE  pclkdivtable)
{
    ULONG  ulParentRate;
    ULONG  ulParentRateSaved = *pulBestParentRate;
    ULONG  ulBest            = 0;
    ULONG  ulNow;
    UINT   uiMaxDiv;
    UINT   uiBestDiv         = 0;
    INT    i;

    if (!ulRate) {
        ulRate = 1;
    }

    uiMaxDiv = __clockTableMaxDivGet(pclkdivtable);

    if (!(__HW_FLAGS_GET(pclk) & LW_CLOCK_SET_RATE_PARENT)) {
        ulParentRate = *pulBestParentRate;
        uiBestDiv    = __clockDivRound(pclkdivtable,
                                       ulParentRate,
                                       ulRate,
                                       __HW_FLAGS_GET(pclk));
        uiBestDiv    = uiBestDiv > uiMaxDiv ? uiMaxDiv : uiBestDiv;
        return  (uiBestDiv);
    }

    uiMaxDiv = __MIN(ULONG_MAX / ulRate, uiMaxDiv);

    for (i  = __clockTableDivRoundUpGet(pclkdivtable, 1);
         i <= uiMaxDiv;
         i  = __clockTableDivRoundUpGet(pclkdivtable, i)) {

         if (ulRate * i == ulParentRateSaved) {
             *pulBestParentRate = ulParentRateSaved;
             return  (i);
         }

         ulParentRate = API_ClockRateRound(pclk, ulRate * i);
         ulNow        = DIV_ROUND_UP_ULL((UINT64)ulParentRate, i);
         if (__clockIsBestDiv(ulRate, ulNow, ulBest, __HW_FLAGS_GET(pclk))) {
             uiBestDiv         = i;
             ulBest            = ulNow;
            *pulBestParentRate = ulParentRate;
         }
    }

    if (!uiBestDiv) {
        uiBestDiv         = __clockTableMaxDivGet(pclkdivtable);
       *pulBestParentRate = API_ClockRateRound(pclkParent, ulRate);
    }

    return  (uiBestDiv);
}
/*********************************************************************************************************
** 函数名称: __clockDividerValGet
** 功能描述: 获得对应频率的寄存器数值
** 输　入  : pclkdivtable     分频表
**           ulParentRate     父时钟频率
**           ulRate           本时钟频率
** 输　出  : 寄存器数值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockDividerValGet (PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                  ULONG                ulParentRate,
                                  ULONG                ulRate)
{
    UINT  uiDiv;
    UINT  uiValue;

    uiDiv = DIV_ROUND_UP_ULL((UINT64)ulParentRate, ulRate);

    if (!__clockIsValidDiv(pclkdivtable, uiDiv)) {                      /*  判断是否是有效分频值        */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    uiValue = __clockTableValGet(pclkdivtable, uiDiv);

    return  (uiValue);
}
/*********************************************************************************************************
** 函数名称: __clockDividerRateRecalc
** 功能描述: 获得分频或倍频时钟的频率值
** 输　入  : pclk             时钟设备
**           ulParentRate     父时钟频率
** 输　出  : 时钟的频率值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ULONG  __clockDividerRateRecalc (PLW_CLOCK  pclk, ULONG  ulParentRate)
{
    PLW_CLOCK_DIVIDER  pclkdivider = __CLK_TO_CLK_DIVIDER(pclk);
    UINT32             uiRegVal;
    UINT32             uiDiv;

    uiRegVal  = pclkdivider->CLKD_pfuncValGet(pclk);
    uiRegVal  = uiRegVal >> pclkdivider->CLKD_uiShift;
    uiRegVal &= pclkdivider->CLKD_uiMask;
    uiDiv     = __clockTableDivGet(pclkdivider->CLKD_pclkdivtable, uiRegVal);

    return  (DIV_ROUND_UP_ULL((UINT64)ulParentRate, uiDiv));
}
/*********************************************************************************************************
** 函数名称: __clockDividerRateSet
** 功能描述: 设置频率
** 输　入  : pclk             时钟设备
**           ulRate           预设置的频率值
**           ulParentRate     父时钟的频率值
** 输　出  : ERROR_NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockDividerRateSet (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate)
{
    PLW_CLOCK_DIVIDER  pclkdivider  = __CLK_TO_CLK_DIVIDER(pclk);
    UINT               uiRegVal;
    INT                iVal;

    iVal = __clockDividerValGet(pclkdivider->CLKD_pclkdivtable, ulParentRate, ulRate);
    if (iVal < 0) {
        return  (iVal);
    }

    uiRegVal  = pclkdivider->CLKD_pfuncValGet(pclk);
    uiRegVal &= ~((pclkdivider->CLKD_uiMask) << (pclkdivider->CLKD_uiShift));
    uiRegVal |= iVal << pclkdivider->CLKD_uiShift;

    return  (pclkdivider->CLKD_pfuncValSet(pclk, uiRegVal));
}
/*********************************************************************************************************
** 函数名称: __clockDividerRateRound
** 功能描述: 获得最接近预设频率的频率值
** 输　入  : pclk             时钟设备
**           ulRate           预设置的频率值
**           pulParentRate    父时钟的频率值
** 输　出  : 最接近预设频率的频率值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __clockDividerRateRound (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulParentRate)
{
    PLW_CLOCK_DIVIDER  pclkdivider = __CLK_TO_CLK_DIVIDER(pclk);
    PLW_CLOCK          pclkParent  = pclk->CLK_clkparent;
    UINT32             uiDiv;

    uiDiv = __clockTableBestDivGet(pclk,
                                   pclkParent,
                                   ulRate,
                                   pulParentRate,
                                   pclkdivider->CLKD_pclkdivtable);

    return  (DIV_ROUND_UP_ULL((UINT64)*pulParentRate, uiDiv));
}
/*********************************************************************************************************
** 函数名称: API_ClockDividerRegister
** 功能描述: 注册可变分频的时钟
** 输　入  : pcName            时钟名称
**           ppcParentName     父时钟名称
**           ulFlags           初始化标识
**           pclkdivtable      分频表
**           uiMask            寄存器掩码
**           uiShift           寄存器移位
**           pfuncValGet       寄存器值获取函数
**           pfuncValSet       寄存器值设置函数
** 输　出  : 可变分频的时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockDividerRegister (CPCHAR               pcName,
                                     CHAR               **ppcParentName,
                                     ULONG                ulFlags,
                                     PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                     UINT                 uiMask,
                                     UINT                 uiShift,
                                     UINTFUNCPTR          pfuncValGet,
                                     FUNCPTR              pfuncValSet)
{
    PLW_CLOCK_DIVIDER   pclkdivider;
    PLW_CLOCK           pclk;
    INT                 iRet;

    if (!pcName || !ppcParentName || !pclkdivtable || !pfuncValGet || !pfuncValSet) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkdivider = __SHEAP_ZALLOC(sizeof(LW_CLOCK_DIVIDER));
    if (!pclkdivider) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkdivider->CLKD_clk,
                                pcName,
                                &_G_clkopsDivider,
                                ulFlags,
                                ppcParentName,
                                1);
    if (iRet) {
        __SHEAP_FREE(pclkdivider);
        return  (LW_NULL);
    }

    pclk = &pclkdivider->CLKD_clk;
    pclkdivider->CLKD_pclkdivtable = pclkdivtable;
    pclkdivider->CLKD_uiMask       = uiMask;
    pclkdivider->CLKD_uiShift      = uiShift;
    pclkdivider->CLKD_pfuncValGet  = pfuncValGet;
    pclkdivider->CLKD_pfuncValSet  = pfuncValSet;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkdivider);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
