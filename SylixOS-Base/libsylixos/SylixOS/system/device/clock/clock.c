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
** 文   件   名: clock.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 26 日
**
** 描        述: 物理时钟控制逻辑
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
  全局变量定义
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineClockRoot;                  /*  有效时钟的树                */
static LW_LIST_LINE_HEADER          _G_plineClockOrphan;                /*  孤立时钟的树                */
static LW_LIST_LINE_HEADER          _G_plineClockProviders;

static LW_OBJECT_HANDLE             _G_hClkProviderLock = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE             _G_hClkListLock     = LW_OBJECT_HANDLE_INVALID;

#define __CLOCK_PROVIDER_LOCK()     API_SemaphoreMPend(_G_hClkProviderLock, LW_OPTION_WAIT_INFINITE)
#define __CLOCK_PROVIDER_UNLOCK()   API_SemaphoreMPost(_G_hClkProviderLock)
#define __CLOCK_LIST_LOCK()         API_SemaphoreMPend(_G_hClkListLock, LW_OPTION_WAIT_INFINITE)
#define __CLOCK_LIST_UNLOCK()       API_SemaphoreMPost(_G_hClkListLock)
/*********************************************************************************************************
** 函数名称: __clockSubtreeFind
** 功能描述: 查找一个时钟子树中的时钟
** 输　入  : pcName        时钟的名称
**           pclk          需要初始化的时钟
** 输　出  : 查找到的时钟
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_CLOCK  __clockSubtreeFind (CPCHAR  pcName, PLW_CLOCK  pclk)
{
    PLW_LIST_LINE   plineTemp;
    PLW_CLOCK       pclkchild;
    PLW_CLOCK       pclkRet;

    if (!lib_strcmp(pclk->CLK_pcName, pcName)) {                        /*  当前时钟为匹配时钟          */
        return  (pclk);
    }

    for (plineTemp  = pclk->CLK_plineclkchild;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历该时钟的孩子时钟        */

        pclkchild = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkRet   = __clockSubtreeFind(pcName, pclkchild);
        if (pclkRet) {
            return  (pclkRet);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __clockHwTreeFind
** 功能描述: 在时钟树中查找一个时钟
** 输　入  : pcName        时钟的名称
** 输　出  : 查找到的时钟
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_CLOCK  __clockTreeFind (CPCHAR  pcName)
{
    PLW_LIST_LINE   plineTemp;
    PLW_CLOCK       pclk;
    PLW_CLOCK       pclkRet;

    __CLOCK_LIST_LOCK();
    for (plineTemp  = _G_plineClockRoot;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  在有效时钟树中查找          */

        pclk    = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkRet = __clockSubtreeFind(pcName, pclk);
        if (pclkRet) {
            __CLOCK_LIST_UNLOCK();
            return  (pclkRet);
        }
    }

    for (plineTemp  = _G_plineClockOrphan;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  在孤立时钟树中查找          */

        pclk    = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkRet = __clockSubtreeFind(pcName, pclk);
        if (pclkRet) {
            __CLOCK_LIST_UNLOCK();
            return  (pclkRet);
        }
    }
    __CLOCK_LIST_UNLOCK();

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __clockParentGetByIndex
** 功能描述: 通过序号获取一个时钟的父时钟
** 输　入  : pclk          时钟指针
**           ucIndex       父时钟序号
** 输　出  : 获得的父时钟
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_CLOCK  __clockParentGetByIndex (PLW_CLOCK  pclk, UINT8  ucIndex)
{
    if (ucIndex >= pclk->CLK_uiNumParents) {                            /*  父时钟序号无效              */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (!pclk->CLK_clkparents[ucIndex]) {                               /*  对应的父时钟位置当前为空    */
        pclk->CLK_clkparents[ucIndex] = __clockTreeFind(pclk->CLK_ppcParentNames[ucIndex]);
    }

    return  (pclk->CLK_clkparents[ucIndex]);
}
/*********************************************************************************************************
** 函数名称: __clockParentGet
** 功能描述: 获得一个时钟的父时钟
** 输　入  : pclk           时钟指针
** 输　出  : 获得的父时钟
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_CLOCK  __clockParentGet (PLW_CLOCK  pclk)
{
    UINT8  ucIndex = 0;

    if ((pclk->CLK_uiNumParents > 1) &&                                 /*  当存在多个父时钟时          */
        pclk->CLK_clkops->clockParentGet) {
        ucIndex = pclk->CLK_clkops->clockParentGet(pclk);               /*  获得父时钟对应的序号        */
    }

    return  (__clockParentGetByIndex(pclk, ucIndex));
}
/*********************************************************************************************************
** 函数名称: __clockParentIndexFetch
** 功能描述: 获得一个时钟父时钟的序号
** 输　入  : pclk           时钟指针
**           pclkParent     父时钟指针
** 输　出  : 父时钟的序号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockParentIndexFetch (PLW_CLOCK  pclk, PLW_CLOCK  pclkParent)
{
    INT  i;

    if (!pclkParent) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    for (i = 0; i < pclk->CLK_uiNumParents; i++) {
        if (__clockParentGetByIndex(pclk, i) == pclkParent) {
            return  (i);
        }
    }

    _ErrorHandle(ENOENT);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __clockParentUpdate
** 功能描述: 更新一个时钟的父时钟
** 输　入  : pclk           需要更新的时钟
**           pclknewparent  新的父时钟；为空时，该时钟被设置为孤立时钟
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __clockParentUpdate (PLW_CLOCK  pclk, PLW_CLOCK  pclknewparent)
{
    if (pclk->CLK_clkparent) {
        _List_Line_Del(&pclk->CLK_lineManage,
                       &pclk->CLK_clkparent->CLK_plineclkchild);        /*  从原父时钟链表中删除        */
    } else {
        _List_Line_Del(&pclk->CLK_lineManage,
                       &_G_plineClockOrphan);
    }

    if (pclknewparent) {                                                /*  如果存在需更新的父时钟      */
        _List_Line_Add_Ahead(&pclk->CLK_lineManage,
                             &pclknewparent->CLK_plineclkchild);
    } else {
        _List_Line_Add_Ahead(&pclk->CLK_lineManage,
                             &_G_plineClockOrphan);
    }

    pclk->CLK_clkparent = pclknewparent;
}
/*********************************************************************************************************
** 函数名称: __clockParentSet
** 功能描述: 设置一个时钟的父时钟
** 输　入  : pclk           时钟指针
**           pclkParent     父时钟指针
**           ucParentIndex  父时钟序号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __clockParentSet (PLW_CLOCK  pclk, PLW_CLOCK  pclkParent, UINT8  ucParentIndex)
{
    __clockParentUpdate(pclk, pclkParent);

    if (pclkParent && pclk->CLK_clkops->clockParentSet) {
        pclk->CLK_clkops->clockParentSet(pclk, ucParentIndex);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __clockRateRecalc
** 功能描述: 设置一个时钟的频率
** 输　入  : pclk          需要初始化的时钟
**           bRecalcChild  是否设置子时钟
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __clockRateRecalc (PLW_CLOCK  pclk, BOOL  bRecalcChild)
{
    PLW_CLOCK_OPS    pclkfuncs;
    PLW_LIST_LINE    plineTemp;
    PLW_CLOCK        pclkchild;
    ULONG            ulParentRate;

    pclkfuncs = pclk->CLK_clkops;

    if (pclk->CLK_clkparent) {                                          /*  记录父时钟频率              */
        ulParentRate = pclk->CLK_clkparent->CLK_ulRate;
    } else {
        ulParentRate = 0;
    }

    if (pclkfuncs->clockRateRecalc) {                                   /*  根据父时钟频率计算频率      */
        pclk->CLK_ulRate = pclkfuncs->clockRateRecalc(pclk, ulParentRate);
    } else {                                                            /*  否则频率为父时钟频率        */
        pclk->CLK_ulRate = ulParentRate;
    }

    if (bRecalcChild) {                                                 /*  对所有子时钟重新计算频率    */
        for (plineTemp  = pclk->CLK_plineclkchild;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            pclkchild = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
            __clockRateRecalc(pclkchild, LW_TRUE);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_ClockParentSet
** 功能描述: 设置一个时钟的父时钟
** 输　入  : pclk           时钟指针
**           pclkParent     父时钟指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockParentSet (PLW_CLOCK  pclk, PLW_CLOCK  pclkParent)
{
    INT  iParentIndex;
    INT  iRet;

    if (!pclk || !pclkParent) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclk->CLK_clkparent == pclkParent) {                            /*  当前父节点已经为预设值      */
        return  (ERROR_NONE);
    }

    if ((pclk->CLK_uiNumParents > 1) &&
        !pclk->CLK_clkops->clockParentSet) {                            /*  未定义父时钟设置的接口      */
        _ErrorHandle(EPERM);
        return  (PX_ERROR);
    }

    iParentIndex = __clockParentIndexFetch(pclk, pclkParent);           /*  查找父时钟的序号            */
    if (iParentIndex < 0) {
        return  (iParentIndex);
    }

    iRet = __clockParentSet(pclk, pclkParent, iParentIndex);            /*  设置父时钟                  */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_ClockRateSet
** 功能描述: 设置一个时钟的频率
** 输　入  : pclk              时钟
**           ulReqRate         预设时钟频率值
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockRateSet (PLW_CLOCK  pclk, ULONG  ulReqRate)
{
    LONG    lRate;

    if (!pclk) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclk->CLK_clkops->clockRateRound) {                             /*  获得最接近的时钟            */
        lRate = pclk->CLK_clkops->clockRateRound(pclk,
                                                 ulReqRate,
                                                 &pclk->CLK_clkparent->CLK_ulRate);
    } else {
        lRate = ulReqRate;
    }

    if (lRate < 0) {
        return  (PX_ERROR);
    }

    if (lRate == API_ClockRateGet(pclk)) {                             /*  如果和当前时钟相等，返回    */
        return  (ERROR_NONE);
    }

    pclk->CLK_clkops->clockRateSet(pclk, lRate, pclk->CLK_clkparent->CLK_ulRate);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ClockRateGet
** 功能描述: 获得一个时钟的当前频率
** 输　入  : pclk              时钟
** 输　出  : 时钟频率
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
ULONG  API_ClockRateGet (PLW_CLOCK  pclk)
{
    ULONG  ulRate;

    if (!pclk) {                                                        /*  参数无效                    */
        _ErrorHandle(EINVAL);
        return  (0);
    }

    __clockRateRecalc(pclk, LW_FALSE);                                  /*  重新计算一下当前时钟频率    */

    ulRate = pclk->CLK_ulRate;                                          /*  获得当前时钟频率            */

    if (!pclk->CLK_uiNumParents) {                                      /*  如果为时钟源                */
        return  (ulRate);

    } else if (!pclk->CLK_clkparent) {                                  /*  有父时钟，但父时钟还未有效  */
        _ErrorHandle(EAGAIN);
        return  (0);
    }

    return  (ulRate);
}
/*********************************************************************************************************
** 函数名称: API_ClockRateRound
** 功能描述: 获得一个时钟的最接近时钟
** 输　入  : pclk           需要求最接近时钟的时钟
**           ulRate         需要求最接近时钟的频率
** 输　出  : 最接近的时钟，如果没有求最接近时钟接口，则返回父时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
LONG  API_ClockRateRound (PLW_CLOCK  pclk, ULONG  ulRate)
{
    ULONG  ulParentRate;
    LONG   lRet;

    if (!pclk) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    if (pclk->CLK_clkparent) {
        ulParentRate = pclk->CLK_clkparent->CLK_ulRate;
    } else {
        ulParentRate = 0;
    }

    if (pclk->CLK_clkops->clockRateRound) {
        lRet = pclk->CLK_clkops->clockRateRound(pclk, ulRate, &ulParentRate);
        if (lRet < 0) {
            return  (lRet);
        }

    } else {
        return  (ulParentRate);
    }

    return  (lRet);
}
/*********************************************************************************************************
** 函数名称: API_ClockSimpleGet
** 功能描述: 获取时钟
** 输　入  : pdtpaClkSpec        设备树参数
**           pvData              私有参数
** 输　出  : 获取时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockSimpleGet (PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec, PVOID  pvData)
{
    return  (pvData);
}
/*********************************************************************************************************
** 函数名称: API_ClockProviderAdd
** 功能描述: 添加一个时钟提供者
** 输　入  : pdtnDev             设备树节点
**           pfuncClkGet         获得时钟的函数
**           pvData              私有参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockProviderAdd (PLW_DEVTREE_NODE  pdtnDev,
                           PLW_CLOCK       (*pfuncClkGet)(PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec,
                                                          PVOID                     pvData),
                           PVOID             pvData)
{
    PLW_CLOCK_PROVIDER  pclkprovider;

    if (!pdtnDev || !pfuncClkGet) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pclkprovider = __SHEAP_ALLOC(sizeof(LW_CLOCK_PROVIDER));
    if (!pclkprovider) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pclkprovider->CLKP_pdtndev = pdtnDev;
    pclkprovider->CLKP_clkGet  = pfuncClkGet;
    pclkprovider->CLKP_pvData  = pvData;

    __CLOCK_PROVIDER_LOCK();
    _List_Line_Add_Ahead(&pclkprovider->CLKP_lineManage,
                         &_G_plineClockProviders);
    __CLOCK_PROVIDER_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ClockGetFromProvider
** 功能描述: 通过设备树参数获取一个时钟
** 输　入  : pdtpaClkSpec         设备树参数
** 输　出  : 获得的时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockGetFromProvider (PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec)
{
    PLW_CLOCK_PROVIDER  pclkprovider;
    PLW_CLOCK           pclk = LW_NULL;
    PLW_LIST_LINE       plineTemp;

    if (!pdtpaClkSpec) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (_G_hClkProviderLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hClkProviderLock = API_SemaphoreMCreate("provider_lock", LW_PRIO_DEF_CEILING,
                                                   LW_OPTION_WAIT_PRIORITY |
                                                   LW_OPTION_INHERIT_PRIORITY |
                                                   LW_OPTION_DELETE_SAFE |
                                                   LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    __CLOCK_PROVIDER_LOCK();
    for (plineTemp  = _G_plineClockProviders;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  查找在时钟树中的时钟        */

        pclkprovider = _LIST_ENTRY(plineTemp, LW_CLOCK_PROVIDER, CLKP_lineManage);
        if (pclkprovider->CLKP_pdtndev == pdtpaClkSpec->DTPH_pdtnDev) { /*  找到对应的时钟提供者        */
            pclk = pclkprovider->CLKP_clkGet(pdtpaClkSpec,
                                             pclkprovider->CLKP_pvData);
            if (!pclk) {
                _ErrorHandle(ENOENT);
                break;
            }
        }
    }
    __CLOCK_PROVIDER_UNLOCK();

    return  (pclk);
}
/*********************************************************************************************************
** 函数名称: API_ClockFind
** 功能描述: 查找时钟
** 输　入  : pcName   查找的时钟名称
** 输　出  : 查找到的时钟指针
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockFind (CPCHAR  pcName)
{
    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (__clockTreeFind(pcName));
}
/*********************************************************************************************************
** 函数名称: API_ClockEnable
** 功能描述: 时钟使能
** 输　入  : pclk  时钟指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockEnable (PLW_CLOCK  pclk)
{
    INT  iRet;

    if (!pclk) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclk->CLK_ulFlags & LW_CLOCK_IS_ROOT) {                         /*  根时钟类型认为使能          */
        return  (ERROR_NONE);
    }

    if (pclk->CLK_uiEnableCount == 0) {                                 /*  如果未使能                  */
        iRet = API_ClockEnable(pclk->CLK_clkparent);                    /*  使能其父时钟                */
        if (iRet) {
            return  (PX_ERROR);
        }

        if (pclk->CLK_clkops->clockEnable) {                            /*  调用该接口的使能操作        */
            iRet = pclk->CLK_clkops->clockEnable(pclk);
            if (iRet) {
                API_ClockDisable(pclk->CLK_clkparent);                  /*  使能失败需要禁能父时钟      */
                return  (PX_ERROR);
            }
        }
    }

    pclk->CLK_uiEnableCount++;                                          /*  增加使能计数                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ClockDisable
** 功能描述: 时钟禁能
** 输　入  : pclk  时钟指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_ClockDisable (PLW_CLOCK  pclk)
{
    if (!pclk) {
        _ErrorHandle(EINVAL);
        return;
    }

    if (pclk->CLK_ulFlags & LW_CLOCK_IS_ROOT) {                         /*  根时钟类型不可关闭          */
        return;
    }

    if (pclk->CLK_uiEnableCount == 0) {
        return;
    }

    if (pclk->CLK_uiEnableCount > 0) {
        pclk->CLK_uiEnableCount--;

        if ((pclk->CLK_uiEnableCount == 0) && pclk->CLK_clkops->clockDisable) {
            pclk->CLK_clkops->clockDisable(pclk);
            API_ClockDisable(pclk->CLK_clkparent);
        }
    }
}
/*********************************************************************************************************
** 函数名称: API_ClockInitDataSet
** 功能描述: 填充时钟初始化参数
** 输　入  : pclk              时钟指针
**           pcName            时钟名称
**           pclkops           时钟操作函数
**           ulFlags           初始化标识
**           ppcParentName     父时钟名称
**           uiParentNum       父时钟数量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockInitDataSet (PLW_CLOCK      pclk,
                           CPCHAR         pcName,
                           PLW_CLOCK_OPS  pclkops,
                           ULONG          ulFlags,
                           CHAR         **ppcParentName,
                           UINT           uiParentNum)
{
    INT  i;

    if (!pclk || !pclkops || !pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((uiParentNum > 0) && !ppcParentName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclkops->clockRateSet &&
        !(pclkops->clockRateRound && pclkops->clockRateRecalc)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "must implement .clockRateRound() "
                     "in addition to .clockRateRecalc()\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclkops->clockParentSet && !pclkops->clockParentGet) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "must implement .clockParentGet() & "
                     ".clockParentSet()\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if ((uiParentNum > 1) && !pclkops->clockParentGet) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "must implement .clockParentGet() as "
                     "it has multi parents\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pclk->CLK_clkops       = pclkops;
    pclk->CLK_ulFlags      = ulFlags;
    pclk->CLK_uiNumParents = uiParentNum;
    pclk->CLK_pcName       = lib_strdup(pcName);
    if (!pclk->CLK_pcName) {
        _ErrorHandle(ENOMEM);
        goto  __error_handle0;
    }

    pclk->CLK_ppcParentNames = __SHEAP_ZALLOC(sizeof(PCHAR) * uiParentNum);
    if (!pclk->CLK_ppcParentNames) {
        _ErrorHandle(ENOMEM);
        goto  __error_handle1;
    }

    for (i = 0; i < uiParentNum; i++) {                                 /*  记录每一个父时钟的名称      */
        if (!ppcParentName[i]) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "invalid NULL in parent_names\r\n");
            _ErrorHandle(EINVAL);
            goto  __error_handle2;
        }

        pclk->CLK_ppcParentNames[i] = lib_strdup(ppcParentName[i]);
        if (!pclk->CLK_ppcParentNames[i]) {
            _ErrorHandle(ENOMEM);
            goto  __error_handle2;
        }
    }

    pclk->CLK_clkparents = __SHEAP_ZALLOC(sizeof(PLW_CLOCK) * uiParentNum);
    if (!pclk->CLK_clkparents) {
        _ErrorHandle(ENOMEM);
        goto  __error_handle3;
    }

    return  (ERROR_NONE);

__error_handle3:
    while (--i >= 0) {
        lib_free(pclk->CLK_ppcParentNames[i]);
    }

__error_handle2:
    __SHEAP_FREE(pclk->CLK_ppcParentNames);

__error_handle1:
    lib_free(pclk->CLK_pcName);

__error_handle0:
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_ClockRegister
** 功能描述: 注册一个时钟管理结构
** 输　入  : pclk            时钟指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockRegister (PLW_CLOCK  pclk)
{
    PLW_LIST_LINE   plineTemp;
    PLW_CLOCK       pclkorphan;
    PLW_CLOCK       pclkparent;

    if (!pclk) {                                                        /*  参数判断                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__clockTreeFind(pclk->CLK_pcName)) {                            /*  时钟树中不应已有该时钟      */
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }

    pclk->CLK_clkparent = __clockParentGet(pclk);                       /*  获取当前指向的父时钟        */

    __CLOCK_LIST_LOCK();
    if (pclk->CLK_clkparent) {                                          /*  如果有父时钟                */
       _List_Line_Add_Ahead(&pclk->CLK_lineManage,                      /*  将该时钟加入父时钟的子链表  */
                            &pclk->CLK_clkparent->CLK_plineclkchild);
    } else if (!pclk->CLK_uiNumParents) {                               /*  如果没有父时钟              */
       _List_Line_Add_Ahead(&pclk->CLK_lineManage,                      /*  将该时钟加入有效时钟树      */
                            &_G_plineClockRoot);
    } else {                                                            /*  未找到父时钟，但应有父时钟  */
       _List_Line_Add_Ahead(&pclk->CLK_lineManage,                      /*  将该时钟加入孤立时钟树      */
                            &_G_plineClockOrphan);
    }
    __CLOCK_LIST_UNLOCK();

    if (pclk->CLK_clkops->clockInit) {                                  /*  执行时钟的初始化操作        */
        pclk->CLK_clkops->clockInit(pclk);
    }

    __CLOCK_LIST_LOCK();
    for (plineTemp  = _G_plineClockOrphan;
         plineTemp != LW_NULL;                                          /*  检查孤立树中是否有可移除的  */
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  更新时钟拓扑中的结构        */

        pclkorphan = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkparent = __clockParentGet(pclkorphan);
        if (pclkparent) {                                               /*  如果此时可找到父时钟了      */
            __clockParentUpdate(pclkorphan, pclkparent);                /*  父时钟更新                  */
        }
    }
    __CLOCK_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ClockCoreInit
** 功能描述: 系统时钟核心初始化
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_ClockCoreInit (VOID)
{
    if (_G_hClkListLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hClkListLock = API_SemaphoreMCreate("clk_list_lock", LW_PRIO_DEF_CEILING,
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    if (_G_hClkProviderLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hClkProviderLock = API_SemaphoreMCreate("clk_provider_lock", LW_PRIO_DEF_CEILING,
                                                   LW_OPTION_WAIT_PRIORITY    |
                                                   LW_OPTION_INHERIT_PRIORITY |
                                                   LW_OPTION_DELETE_SAFE |
                                                   LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
