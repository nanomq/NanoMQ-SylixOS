/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: clock.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 26 ��
**
** ��        ��: ����ʱ�ӿ����߼�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "clock.h"
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineClockRoot;                  /*  ��Чʱ�ӵ���                */
static LW_LIST_LINE_HEADER          _G_plineClockOrphan;                /*  ����ʱ�ӵ���                */
static LW_LIST_LINE_HEADER          _G_plineClockProviders;

static LW_OBJECT_HANDLE             _G_hClkProviderLock = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE             _G_hClkListLock     = LW_OBJECT_HANDLE_INVALID;

#define __CLOCK_PROVIDER_LOCK()     API_SemaphoreMPend(_G_hClkProviderLock, LW_OPTION_WAIT_INFINITE)
#define __CLOCK_PROVIDER_UNLOCK()   API_SemaphoreMPost(_G_hClkProviderLock)
#define __CLOCK_LIST_LOCK()         API_SemaphoreMPend(_G_hClkListLock, LW_OPTION_WAIT_INFINITE)
#define __CLOCK_LIST_UNLOCK()       API_SemaphoreMPost(_G_hClkListLock)
/*********************************************************************************************************
** ��������: __clockSubtreeFind
** ��������: ����һ��ʱ�������е�ʱ��
** �䡡��  : pcName        ʱ�ӵ�����
**           pclk          ��Ҫ��ʼ����ʱ��
** �䡡��  : ���ҵ���ʱ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_CLOCK  __clockSubtreeFind (CPCHAR  pcName, PLW_CLOCK  pclk)
{
    PLW_LIST_LINE   plineTemp;
    PLW_CLOCK       pclkchild;
    PLW_CLOCK       pclkRet;

    if (!lib_strcmp(pclk->CLK_pcName, pcName)) {                        /*  ��ǰʱ��Ϊƥ��ʱ��          */
        return  (pclk);
    }

    for (plineTemp  = pclk->CLK_plineclkchild;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ������ʱ�ӵĺ���ʱ��        */

        pclkchild = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkRet   = __clockSubtreeFind(pcName, pclkchild);
        if (pclkRet) {
            return  (pclkRet);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __clockHwTreeFind
** ��������: ��ʱ�����в���һ��ʱ��
** �䡡��  : pcName        ʱ�ӵ�����
** �䡡��  : ���ҵ���ʱ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_CLOCK  __clockTreeFind (CPCHAR  pcName)
{
    PLW_LIST_LINE   plineTemp;
    PLW_CLOCK       pclk;
    PLW_CLOCK       pclkRet;

    __CLOCK_LIST_LOCK();
    for (plineTemp  = _G_plineClockRoot;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ����Чʱ�����в���          */

        pclk    = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkRet = __clockSubtreeFind(pcName, pclk);
        if (pclkRet) {
            __CLOCK_LIST_UNLOCK();
            return  (pclkRet);
        }
    }

    for (plineTemp  = _G_plineClockOrphan;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �ڹ���ʱ�����в���          */

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
** ��������: __clockParentGetByIndex
** ��������: ͨ����Ż�ȡһ��ʱ�ӵĸ�ʱ��
** �䡡��  : pclk          ʱ��ָ��
**           ucIndex       ��ʱ�����
** �䡡��  : ��õĸ�ʱ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_CLOCK  __clockParentGetByIndex (PLW_CLOCK  pclk, UINT8  ucIndex)
{
    if (ucIndex >= pclk->CLK_uiNumParents) {                            /*  ��ʱ�������Ч              */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (!pclk->CLK_clkparents[ucIndex]) {                               /*  ��Ӧ�ĸ�ʱ��λ�õ�ǰΪ��    */
        pclk->CLK_clkparents[ucIndex] = __clockTreeFind(pclk->CLK_ppcParentNames[ucIndex]);
    }

    return  (pclk->CLK_clkparents[ucIndex]);
}
/*********************************************************************************************************
** ��������: __clockParentGet
** ��������: ���һ��ʱ�ӵĸ�ʱ��
** �䡡��  : pclk           ʱ��ָ��
** �䡡��  : ��õĸ�ʱ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_CLOCK  __clockParentGet (PLW_CLOCK  pclk)
{
    UINT8  ucIndex = 0;

    if ((pclk->CLK_uiNumParents > 1) &&                                 /*  �����ڶ����ʱ��ʱ          */
        pclk->CLK_clkops->clockParentGet) {
        ucIndex = pclk->CLK_clkops->clockParentGet(pclk);               /*  ��ø�ʱ�Ӷ�Ӧ�����        */
    }

    return  (__clockParentGetByIndex(pclk, ucIndex));
}
/*********************************************************************************************************
** ��������: __clockParentIndexFetch
** ��������: ���һ��ʱ�Ӹ�ʱ�ӵ����
** �䡡��  : pclk           ʱ��ָ��
**           pclkParent     ��ʱ��ָ��
** �䡡��  : ��ʱ�ӵ����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __clockParentUpdate
** ��������: ����һ��ʱ�ӵĸ�ʱ��
** �䡡��  : pclk           ��Ҫ���µ�ʱ��
**           pclknewparent  �µĸ�ʱ�ӣ�Ϊ��ʱ����ʱ�ӱ�����Ϊ����ʱ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __clockParentUpdate (PLW_CLOCK  pclk, PLW_CLOCK  pclknewparent)
{
    if (pclk->CLK_clkparent) {
        _List_Line_Del(&pclk->CLK_lineManage,
                       &pclk->CLK_clkparent->CLK_plineclkchild);        /*  ��ԭ��ʱ��������ɾ��        */
    } else {
        _List_Line_Del(&pclk->CLK_lineManage,
                       &_G_plineClockOrphan);
    }

    if (pclknewparent) {                                                /*  �����������µĸ�ʱ��      */
        _List_Line_Add_Ahead(&pclk->CLK_lineManage,
                             &pclknewparent->CLK_plineclkchild);
    } else {
        _List_Line_Add_Ahead(&pclk->CLK_lineManage,
                             &_G_plineClockOrphan);
    }

    pclk->CLK_clkparent = pclknewparent;
}
/*********************************************************************************************************
** ��������: __clockParentSet
** ��������: ����һ��ʱ�ӵĸ�ʱ��
** �䡡��  : pclk           ʱ��ָ��
**           pclkParent     ��ʱ��ָ��
**           ucParentIndex  ��ʱ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __clockRateRecalc
** ��������: ����һ��ʱ�ӵ�Ƶ��
** �䡡��  : pclk          ��Ҫ��ʼ����ʱ��
**           bRecalcChild  �Ƿ�������ʱ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __clockRateRecalc (PLW_CLOCK  pclk, BOOL  bRecalcChild)
{
    PLW_CLOCK_OPS    pclkfuncs;
    PLW_LIST_LINE    plineTemp;
    PLW_CLOCK        pclkchild;
    ULONG            ulParentRate;

    pclkfuncs = pclk->CLK_clkops;

    if (pclk->CLK_clkparent) {                                          /*  ��¼��ʱ��Ƶ��              */
        ulParentRate = pclk->CLK_clkparent->CLK_ulRate;
    } else {
        ulParentRate = 0;
    }

    if (pclkfuncs->clockRateRecalc) {                                   /*  ���ݸ�ʱ��Ƶ�ʼ���Ƶ��      */
        pclk->CLK_ulRate = pclkfuncs->clockRateRecalc(pclk, ulParentRate);
    } else {                                                            /*  ����Ƶ��Ϊ��ʱ��Ƶ��        */
        pclk->CLK_ulRate = ulParentRate;
    }

    if (bRecalcChild) {                                                 /*  ��������ʱ�����¼���Ƶ��    */
        for (plineTemp  = pclk->CLK_plineclkchild;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            pclkchild = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
            __clockRateRecalc(pclkchild, LW_TRUE);
        }
    }
}
/*********************************************************************************************************
** ��������: API_ClockParentSet
** ��������: ����һ��ʱ�ӵĸ�ʱ��
** �䡡��  : pclk           ʱ��ָ��
**           pclkParent     ��ʱ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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

    if (pclk->CLK_clkparent == pclkParent) {                            /*  ��ǰ���ڵ��Ѿ�ΪԤ��ֵ      */
        return  (ERROR_NONE);
    }

    if ((pclk->CLK_uiNumParents > 1) &&
        !pclk->CLK_clkops->clockParentSet) {                            /*  δ���常ʱ�����õĽӿ�      */
        _ErrorHandle(EPERM);
        return  (PX_ERROR);
    }

    iParentIndex = __clockParentIndexFetch(pclk, pclkParent);           /*  ���Ҹ�ʱ�ӵ����            */
    if (iParentIndex < 0) {
        return  (iParentIndex);
    }

    iRet = __clockParentSet(pclk, pclkParent, iParentIndex);            /*  ���ø�ʱ��                  */

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_ClockRateSet
** ��������: ����һ��ʱ�ӵ�Ƶ��
** �䡡��  : pclk              ʱ��
**           ulReqRate         Ԥ��ʱ��Ƶ��ֵ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_ClockRateSet (PLW_CLOCK  pclk, ULONG  ulReqRate)
{
    LONG    lRate;

    if (!pclk) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclk->CLK_clkops->clockRateRound) {                             /*  �����ӽ���ʱ��            */
        lRate = pclk->CLK_clkops->clockRateRound(pclk,
                                                 ulReqRate,
                                                 &pclk->CLK_clkparent->CLK_ulRate);
    } else {
        lRate = ulReqRate;
    }

    if (lRate < 0) {
        return  (PX_ERROR);
    }

    if (lRate == API_ClockRateGet(pclk)) {                             /*  ����͵�ǰʱ����ȣ�����    */
        return  (ERROR_NONE);
    }

    pclk->CLK_clkops->clockRateSet(pclk, lRate, pclk->CLK_clkparent->CLK_ulRate);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ClockRateGet
** ��������: ���һ��ʱ�ӵĵ�ǰƵ��
** �䡡��  : pclk              ʱ��
** �䡡��  : ʱ��Ƶ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
ULONG  API_ClockRateGet (PLW_CLOCK  pclk)
{
    ULONG  ulRate;

    if (!pclk) {                                                        /*  ������Ч                    */
        _ErrorHandle(EINVAL);
        return  (0);
    }

    __clockRateRecalc(pclk, LW_FALSE);                                  /*  ���¼���һ�µ�ǰʱ��Ƶ��    */

    ulRate = pclk->CLK_ulRate;                                          /*  ��õ�ǰʱ��Ƶ��            */

    if (!pclk->CLK_uiNumParents) {                                      /*  ���Ϊʱ��Դ                */
        return  (ulRate);

    } else if (!pclk->CLK_clkparent) {                                  /*  �и�ʱ�ӣ�����ʱ�ӻ�δ��Ч  */
        _ErrorHandle(EAGAIN);
        return  (0);
    }

    return  (ulRate);
}
/*********************************************************************************************************
** ��������: API_ClockRateRound
** ��������: ���һ��ʱ�ӵ���ӽ�ʱ��
** �䡡��  : pclk           ��Ҫ����ӽ�ʱ�ӵ�ʱ��
**           ulRate         ��Ҫ����ӽ�ʱ�ӵ�Ƶ��
** �䡡��  : ��ӽ���ʱ�ӣ����û������ӽ�ʱ�ӽӿڣ��򷵻ظ�ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_ClockSimpleGet
** ��������: ��ȡʱ��
** �䡡��  : pdtpaClkSpec        �豸������
**           pvData              ˽�в���
** �䡡��  : ��ȡʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockSimpleGet (PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec, PVOID  pvData)
{
    return  (pvData);
}
/*********************************************************************************************************
** ��������: API_ClockProviderAdd
** ��������: ���һ��ʱ���ṩ��
** �䡡��  : pdtnDev             �豸���ڵ�
**           pfuncClkGet         ���ʱ�ӵĺ���
**           pvData              ˽�в���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_ClockGetFromProvider
** ��������: ͨ���豸��������ȡһ��ʱ��
** �䡡��  : pdtpaClkSpec         �豸������
** �䡡��  : ��õ�ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ������ʱ�����е�ʱ��        */

        pclkprovider = _LIST_ENTRY(plineTemp, LW_CLOCK_PROVIDER, CLKP_lineManage);
        if (pclkprovider->CLKP_pdtndev == pdtpaClkSpec->DTPH_pdtnDev) { /*  �ҵ���Ӧ��ʱ���ṩ��        */
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
** ��������: API_ClockFind
** ��������: ����ʱ��
** �䡡��  : pcName   ���ҵ�ʱ������
** �䡡��  : ���ҵ���ʱ��ָ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_ClockEnable
** ��������: ʱ��ʹ��
** �䡡��  : pclk  ʱ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_ClockEnable (PLW_CLOCK  pclk)
{
    INT  iRet;

    if (!pclk) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pclk->CLK_ulFlags & LW_CLOCK_IS_ROOT) {                         /*  ��ʱ��������Ϊʹ��          */
        return  (ERROR_NONE);
    }

    if (pclk->CLK_uiEnableCount == 0) {                                 /*  ���δʹ��                  */
        iRet = API_ClockEnable(pclk->CLK_clkparent);                    /*  ʹ���丸ʱ��                */
        if (iRet) {
            return  (PX_ERROR);
        }

        if (pclk->CLK_clkops->clockEnable) {                            /*  ���øýӿڵ�ʹ�ܲ���        */
            iRet = pclk->CLK_clkops->clockEnable(pclk);
            if (iRet) {
                API_ClockDisable(pclk->CLK_clkparent);                  /*  ʹ��ʧ����Ҫ���ܸ�ʱ��      */
                return  (PX_ERROR);
            }
        }
    }

    pclk->CLK_uiEnableCount++;                                          /*  ����ʹ�ܼ���                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ClockDisable
** ��������: ʱ�ӽ���
** �䡡��  : pclk  ʱ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_ClockDisable (PLW_CLOCK  pclk)
{
    if (!pclk) {
        _ErrorHandle(EINVAL);
        return;
    }

    if (pclk->CLK_ulFlags & LW_CLOCK_IS_ROOT) {                         /*  ��ʱ�����Ͳ��ɹر�          */
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
** ��������: API_ClockInitDataSet
** ��������: ���ʱ�ӳ�ʼ������
** �䡡��  : pclk              ʱ��ָ��
**           pcName            ʱ������
**           pclkops           ʱ�Ӳ�������
**           ulFlags           ��ʼ����ʶ
**           ppcParentName     ��ʱ������
**           uiParentNum       ��ʱ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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

    for (i = 0; i < uiParentNum; i++) {                                 /*  ��¼ÿһ����ʱ�ӵ�����      */
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
** ��������: API_ClockRegister
** ��������: ע��һ��ʱ�ӹ���ṹ
** �䡡��  : pclk            ʱ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_ClockRegister (PLW_CLOCK  pclk)
{
    PLW_LIST_LINE   plineTemp;
    PLW_CLOCK       pclkorphan;
    PLW_CLOCK       pclkparent;

    if (!pclk) {                                                        /*  �����ж�                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__clockTreeFind(pclk->CLK_pcName)) {                            /*  ʱ�����в�Ӧ���и�ʱ��      */
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }

    pclk->CLK_clkparent = __clockParentGet(pclk);                       /*  ��ȡ��ǰָ��ĸ�ʱ��        */

    __CLOCK_LIST_LOCK();
    if (pclk->CLK_clkparent) {                                          /*  ����и�ʱ��                */
       _List_Line_Add_Ahead(&pclk->CLK_lineManage,                      /*  ����ʱ�Ӽ��븸ʱ�ӵ�������  */
                            &pclk->CLK_clkparent->CLK_plineclkchild);
    } else if (!pclk->CLK_uiNumParents) {                               /*  ���û�и�ʱ��              */
       _List_Line_Add_Ahead(&pclk->CLK_lineManage,                      /*  ����ʱ�Ӽ�����Чʱ����      */
                            &_G_plineClockRoot);
    } else {                                                            /*  δ�ҵ���ʱ�ӣ���Ӧ�и�ʱ��  */
       _List_Line_Add_Ahead(&pclk->CLK_lineManage,                      /*  ����ʱ�Ӽ������ʱ����      */
                            &_G_plineClockOrphan);
    }
    __CLOCK_LIST_UNLOCK();

    if (pclk->CLK_clkops->clockInit) {                                  /*  ִ��ʱ�ӵĳ�ʼ������        */
        pclk->CLK_clkops->clockInit(pclk);
    }

    __CLOCK_LIST_LOCK();
    for (plineTemp  = _G_plineClockOrphan;
         plineTemp != LW_NULL;                                          /*  �����������Ƿ��п��Ƴ���  */
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ����ʱ�������еĽṹ        */

        pclkorphan = _LIST_ENTRY(plineTemp, LW_CLOCK, CLK_lineManage);
        pclkparent = __clockParentGet(pclkorphan);
        if (pclkparent) {                                               /*  �����ʱ���ҵ���ʱ����      */
            __clockParentUpdate(pclkorphan, pclkparent);                /*  ��ʱ�Ӹ���                  */
        }
    }
    __CLOCK_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ClockCoreInit
** ��������: ϵͳʱ�Ӻ��ĳ�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
