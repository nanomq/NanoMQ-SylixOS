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
** 文   件   名: pinCtrl.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 02 日
**
** 描        述: 引脚控制实现
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
#include "pinCtrl.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER      _G_plinePinCtrls;                       /*  GPIO 控制器链表             */
static LW_LIST_LINE_HEADER      _G_plinePinCtrlMaps;                    /*  全局有效的映射列表          */
/*********************************************************************************************************
  同步锁
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_hPctlLock     = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE         _G_hPctlMapsLock = LW_OBJECT_HANDLE_INVALID;

#define __PCTL_LOCK()           API_SemaphoreMPend(_G_hPctlLock, LW_OPTION_WAIT_INFINITE)
#define __PCTL_UNLOCK()         API_SemaphoreMPost(_G_hPctlLock)
#define __PCTLMAPS_LOCK()       API_SemaphoreMPend(_G_hPctlMapsLock, LW_OPTION_WAIT_INFINITE)
#define __PCTLMAPS_UNLOCK()     API_SemaphoreMPost(_G_hPctlMapsLock)
/*********************************************************************************************************
** 函数名称: __pinCtrlStateCommit
** 功能描述: 使用指定的引脚状态
** 输　入  : ppinctl          引脚控制结构
**           ppctlstate       需要设置的引脚状态
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
static INT  __pinCtrlStateCommit (PLW_PINCTRL  ppinctl, PLW_PINCTRL_STATE  ppctlstate)
{
    PLW_PINCTRL_SETTING  ppctlsetting;
    PLW_PINCTRL_SETTING  ppctlsetting2;
    PLW_PINCTRL_STATE    ppctloldstate;
    PLW_LIST_LINE        plineTemp;
    INT                  iRet;

    ppctloldstate = ppinctl->PCTL_ppctlstate;                           /*  记录原来的引脚状态          */
    if (ppctloldstate) {                                                /*  如果原来有设定的引脚状态    */
        for (plineTemp  = ppinctl->PCTL_ppctlstate->PCTLS_plineSettings;/*  遍历原有引脚状态中的配置    */
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            ppctlsetting = _LIST_ENTRY(plineTemp, LW_PINCTRL_SETTING, PCTLS_lineManage);
            if (ppctlsetting->PCTLS_pinctlmaptype !=
                PIN_MAP_TYPE_MUX_GROUP) {                               /*  如果不是按组引脚复用配置    */
                continue;
            }

            API_PinMuxDisable(ppctlsetting);                            /*  禁用该引脚配置              */
         }
    }

    ppinctl->PCTL_ppctlstate = LW_NULL;                                 /*  将引脚状态清空              */

    for (plineTemp  = ppctlstate->PCTLS_plineSettings;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历待设置引脚状态中的配置  */

        ppctlsetting = _LIST_ENTRY(plineTemp, LW_PINCTRL_SETTING, PCTLS_lineManage);

        switch (ppctlsetting->PCTLS_pinctlmaptype) {

        case PIN_MAP_TYPE_MUX_GROUP:                                    /*  按组引脚复用                */
            iRet = API_PinMuxEnable(ppctlsetting);
            break;

        case PIN_MAP_TYPE_CONFIGS_PIN:                                  /*  单个引脚配置                */
        case PIN_MAP_TYPE_CONFIGS_GROUP:                                /*  按组引脚配置                */
            iRet = API_PinConfigApply(ppctlsetting);
            break;

        default:
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if (iRet < 0) {                                                 /*  失败时恢复原来的配置        */
            goto  __error_handle;
        }
    }

    ppinctl->PCTL_ppctlstate = ppctlstate;

    return  (ERROR_NONE);

__error_handle:
    for (plineTemp  = ppctlstate->PCTLS_plineSettings;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppctlsetting2 = _LIST_ENTRY(plineTemp, LW_PINCTRL_SETTING, PCTLS_lineManage);

        if (&ppctlsetting2->PCTLS_pdtnDev == &ppctlsetting->PCTLS_pdtnDev) {
            break;
        }

        if (ppctlsetting2->PCTLS_pinctlmaptype == PIN_MAP_TYPE_MUX_GROUP) {
            API_PinMuxDisable(ppctlsetting2);
        }
    }

    if (ppctloldstate) {                                                /*  重新设置原来的配置          */
        API_PinCtrlStateSelect(ppinctl, ppctloldstate);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlStateFind
** 功能描述: 在引脚控制状态链表中查找对应名称的引脚状态
** 输　入  : ppinctl        遍历的引脚控制
**           pcName         查找的引脚状态名称
** 输　出  : 找到的引脚状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_PINCTRL_STATE  __pinCtrlStateFind (PLW_PINCTRL  ppinctl, CPCHAR  pcName)
{
    PLW_PINCTRL_STATE  ppctlstate;
    PLW_LIST_LINE      plineTemp;

    for (plineTemp  = ppinctl->PCTL_plineStates;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppctlstate = _LIST_ENTRY(plineTemp, LW_PINCTRL_STATE, PCTLS_lineManage);
        if (!lib_strcmp(ppctlstate->PCTLS_pcName, pcName)) {
            return  (ppctlstate);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlStateCreate
** 功能描述: 创建新的引脚状态，插入到引脚控制状态链表中
** 输　入  : ppinctl        所属的引脚控制
**           pcName         创建的引脚状态名称
** 输　出  : 创建的引脚状态
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_PINCTRL_STATE  __pinCtrlStateCreate (PLW_PINCTRL  ppinctl, CPCHAR  pcName)
{
    PLW_PINCTRL_STATE  ppctlstate;

    ppctlstate = (PLW_PINCTRL_STATE)__SHEAP_ZALLOC(sizeof(LW_PINCTRL_STATE));
    if (ppctlstate == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    ppctlstate->PCTLS_pcName = pcName;

    _List_Line_Add_Ahead(&ppctlstate->PCTLS_lineManage, &ppinctl->PCTL_plineStates);

    return  (ppctlstate);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlSettingAdd
** 功能描述: 将引脚映射结构转换为引脚配置，并关联到引脚控制
** 输　入  : ppinctl          关联的引脚控制
**           ppctlmap         用于转换的引脚映射结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pinCtrlSettingAdd (PLW_PINCTRL  ppinctl, PLW_PINCTRL_MAP  ppctlmap)
{
    PLW_PINCTRL_STATE     ppctlstate;
    PLW_PINCTRL_SETTING   ppctlsetting;
    INT                   iRet;

    ppctlstate = __pinCtrlStateFind(ppinctl, ppctlmap->PCTLM_pcName);   /*  查找引脚状态                */
    if (!ppctlstate) {
        ppctlstate = __pinCtrlStateCreate(ppinctl,
                                          ppctlmap->PCTLM_pcName);      /*  若无，则创建引脚状态        */
        if (!ppctlstate) {
             return  (PX_ERROR);
        }
    }

    if (ppctlmap->PCTLM_pctlmaptype == PIN_MAP_TYPE_DUMMY_STATE) {      /*  DUMMY_STATE 不创建          */
        return  (ERROR_NONE);
    }

    ppctlsetting = (PLW_PINCTRL_SETTING)__SHEAP_ZALLOC(sizeof(LW_PINCTRL_SETTING));
    if (ppctlsetting == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    ppctlsetting->PCTLS_pinctlmaptype = ppctlmap->PCTLM_pctlmaptype;    /*  设置配置的映射类型          */
    ppctlsetting->PCTLS_pdtnDev = ppctlmap->PCTLM_pdtnDev;              /*  设置配置关联的设备树节点    */

    ppctlsetting->PCTLS_ppinctldev = API_PinCtrlDevGetByDevtreeNode(ppctlmap->PCTLM_pdtnCtrlNode);
    if (ppctlsetting->PCTLS_ppinctldev == LW_NULL) {                    /*  设置配置关联的引脚控制设备  */
        __SHEAP_FREE(ppctlsetting);
        if (ppctlmap->PCTLM_pdtnCtrlNode == ppctlmap->PCTLM_pdtnDev) {
            _ErrorHandle(-ENODEV);
            return  (PX_ERROR);
        }
        return  (-ERROR_DEVTREE_EPROBE_DEFER);
    }

    switch (ppctlmap->PCTLM_pctlmaptype) {                              /*  将引脚映射结构转换为引脚配置*/

    case PIN_MAP_TYPE_MUX_GROUP:                                        /*  按组引脚复用配置            */
        iRet = API_PinMuxMapToSetting(ppctlmap, ppctlsetting);
        break;

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  按照单个引脚功能配置        */
    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  按组引脚功能配置            */
        iRet = API_PinConfigMapToSetting(ppctlmap, ppctlsetting);
        break;

    default:
        iRet = -EINVAL;
        break;
    }

    if (iRet < 0) {
        __SHEAP_FREE(ppctlsetting);
        return  (iRet);
    }

    _List_Line_Add_Ahead(&ppctlsetting->PCTLS_lineManage,
                         &ppctlstate->PCTLS_plineSettings);             /*  链入引脚状态的配置链表      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlSettingFree
** 功能描述: 释放一个引脚配置
** 输　入  : bDisableSetting     释放时是否禁用该配置
**           ppctlsetting        是否的引脚配置
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pinCtrlSettingFree (BOOL  bDisableSetting, PLW_PINCTRL_SETTING  ppctlsetting)
{
    switch (ppctlsetting->PCTLS_pinctlmaptype) {

    case PIN_MAP_TYPE_MUX_GROUP:                                        /*  按组引脚复用                */
        if (bDisableSetting) {
            API_PinMuxDisable(ppctlsetting);
        }
        break;

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  单个引脚配置                */
    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  按组配置相关 (暂时不处理)   */
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** 函数名称: __pinCtrlFind
** 功能描述: 查找某个外设相关的引脚控制
** 输　入  : pdtnDev      外设设备树节点
** 输　出  : 查找到的引脚控制
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_PINCTRL  __pinCtrlFind (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL      ppinctl;
    PLW_LIST_LINE    plineTemp;

    if (_G_hPctlLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlLock = API_SemaphoreMCreate("ppinctl_lock", LW_PRIO_DEF_CEILING,
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    if (_G_hPctlMapsLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlMapsLock = API_SemaphoreMCreate("ppctlmaps_lock", LW_PRIO_DEF_CEILING,
                                                LW_OPTION_WAIT_PRIORITY |
                                                LW_OPTION_INHERIT_PRIORITY |
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    __PCTL_LOCK();
    for (plineTemp  = _G_plinePinCtrls;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历引脚控制链表            */

        ppinctl = _LIST_ENTRY(plineTemp, LW_PINCTRL, PCTL_lineManage);
        if (ppinctl->PCTL_pdtnDev == pdtnDev) {                         /*  匹配引脚控制关联的节点      */
            __PCTL_UNLOCK();
            return  (ppinctl);
        }
    }
    __PCTL_UNLOCK();

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __pinctrlFree
** 功能描述: 释放一个引脚控制
** 输　入  : ppinctl     需要释放的引脚控制
**           bInlist     是否同时从链表中移除
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pinCtrlFree (PLW_PINCTRL  ppinctl, BOOL  bInlist)
{
    PLW_PINCTRL_STATE    ppctlstate;
    PLW_PINCTRL_SETTING  ppctlsetting;
    PLW_LIST_LINE        plinestate;
    PLW_LIST_LINE        plinesetting;
    BOOL                 bDisableSetting;

    __PCTL_LOCK();

    for (plinestate  = ppinctl->PCTL_plineStates;
         plinestate != LW_NULL;
         plinestate  = _list_line_get_next(plinestate)) {               /*  遍历引脚控制的引脚状态链表  */

        ppctlstate = _LIST_ENTRY(plinestate, LW_PINCTRL_STATE, PCTLS_lineManage);

        for (plinesetting  = ppctlstate->PCTLS_plineSettings;
             plinesetting != LW_NULL;
             plinesetting  = _list_line_get_next(plinesetting)) {       /*  遍历引脚状态中的引脚配置    */

            ppctlsetting = _LIST_ENTRY(plinesetting, LW_PINCTRL_SETTING, PCTLS_lineManage);

            bDisableSetting = (ppctlstate == ppinctl->PCTL_ppctlstate);
            __pinCtrlSettingFree(bDisableSetting, ppctlsetting);        /*  注销该引脚配置              */

            _List_Line_Del(&ppctlsetting->PCTLS_lineManage,
                           &ppctlstate->PCTLS_plineSettings);           /*  移除该引脚配置节点          */
            __SHEAP_FREE(ppctlsetting);                                 /*  释放该引脚配置内存          */
        }

        _List_Line_Del(&ppctlstate->PCTLS_lineManage,
                       &ppinctl->PCTL_plineStates);                     /*  移除该引脚状态              */
        __SHEAP_FREE(ppctlstate);                                       /*  释放该引脚状态内存          */
    }

    API_DeviceTreePinCtrlMapsFree(ppinctl);                             /*  注销该引脚控制              */

    if (bInlist) {
        _List_Line_Del(&ppinctl->PCTL_lineManage, &_G_plinePinCtrls);
    }

    __SHEAP_FREE(ppinctl);                                              /*  释放该引脚内存              */

    __PCTL_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: __pinCtrlCreate
** 功能描述: 创建某个外设相关的引脚控制
** 输　入  : pdtnDev      外设设备树节点
** 输　出  : 创建的引脚控制
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_PINCTRL  __pinCtrlCreate (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL        ppinctl;
    PLW_PINCTRL_MAPS   ppctlmaps;
    PLW_PINCTRL_MAP    ppctlmap;
    PLW_LIST_LINE      plineTemp;
    INT                iRet;
    INT                i;

    ppinctl = (PLW_PINCTRL)__SHEAP_ZALLOC(sizeof(LW_PINCTRL));
    if (ppinctl == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    ppinctl->PCTL_pdtnDev = pdtnDev;                                    /*  引脚控制关联设备树节点      */

    iRet = API_DeviceTreePinCtrlMapsCreate(ppinctl);                    /*  将设备树解析成引脚映射结构  */
    if (iRet < 0) {
        __SHEAP_FREE(ppinctl);
        return  (LW_NULL);
    }

    __PCTLMAPS_LOCK();

    for (plineTemp  = _G_plinePinCtrlMaps;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历所有有效的引脚映射      */

        ppctlmaps = _LIST_ENTRY(plineTemp, LW_PINCTRL_MAPS, PCTLM_lineGlobalManage);

        for (i = 0, ppctlmap = &ppctlmaps->PCTLM_ppinctlmaps[i];
             i < ppctlmaps->PCTLM_uiMapsNum;
             i++, ppctlmap = &ppctlmaps->PCTLM_ppinctlmaps[i]) {

            if (ppctlmap->PCTLM_pdtnDev != pdtnDev) {                   /*  必须是当前设备              */
                continue;
            }

            iRet = __pinCtrlSettingAdd(ppinctl, ppctlmap);              /*  增加引脚配置                */
            if (iRet == -ERROR_DEVTREE_EPROBE_DEFER) {
                __pinCtrlFree(ppinctl, LW_FALSE);
                __PCTLMAPS_UNLOCK();
                return  (LW_NULL);
            }
        }
    }
    __PCTLMAPS_UNLOCK();

    if (iRet < 0) {
        __pinCtrlFree(ppinctl, LW_FALSE);
        return  (LW_NULL);
    }

    __PCTL_LOCK();
    _List_Line_Add_Ahead(&ppinctl->PCTL_lineManage, &_G_plinePinCtrls);
    __PCTL_UNLOCK();

    return  (ppinctl);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlPinGetByName
** 功能描述: 根据名称获取引脚序号
** 输　入  : ppctldev        引脚控制器设备
**           pcName          引脚名称
** 输　出  : 引脚序号
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlPinGetByName (PLW_PINCTRL_DEV  ppctldev, CPCHAR  pcName)
{
    PLW_PIN_DESC  pindesc;
    UINT          uiPin;
    UINT          i;

    if (!pcName   ||
        !ppctldev ||
        !ppctldev->PCTLD_ppinctldesc) {                                 /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    for (i = 0;
         i < ppctldev->PCTLD_ppinctldesc->PCTLD_uiPinsNum;
         i++) {                                                         /*  遍历引脚描述                */

        uiPin   = ppctldev->PCTLD_ppinctldesc->PCTLD_ppinctrlpindesc[i].PCTLPD_uiIndex;
                                                                        /*  获取引脚序号                */
        pindesc = API_PinCtrlPinDescGet(ppctldev, uiPin);               /*  获取该序号的引脚描述        */
        if (pindesc && !lib_strcmp(pcName, pindesc->PIN_pcName)) {      /*  比较引脚描述中的名称        */
            return  (uiPin);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlGroupSelectorGet
** 功能描述: 根据引脚组名称获取引脚组序号
** 输　入  : ppctldev       引脚控制器
**           pcPinGroup     引脚组名称
** 输　出  : 引脚组序号
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlGroupSelectorGet (PLW_PINCTRL_DEV  ppctldev, CPCHAR  pcPinGroup)
{
    PLW_PINCTRL_OPS  ppinctlops;
    CPCHAR           pcGroupName     = LW_NULL;
    UINT             uiGroupsNum     = 0;
    UINT             uiGroupSelector = 0;

    if (!ppctldev || !ppctldev->PCTLD_ppinctldesc) {                    /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctlops = ppctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
    if (!ppinctlops->pinctrlGroupCountGet ||
        !ppinctlops->pinctrlGroupNameGet) {                             /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    uiGroupsNum = ppinctlops->pinctrlGroupCountGet(ppctldev);           /*  获取引脚组数量              */

    while (uiGroupSelector < uiGroupsNum) {                             /*  遍历所有引脚组              */
        pcGroupName = ppinctlops->pinctrlGroupNameGet(ppctldev,
                                                      uiGroupSelector); /*  获得引脚组名称              */
        if (!lib_strcmp(pcGroupName, pcPinGroup)) {                     /*  找到匹配的引脚组            */
           return  (uiGroupSelector);
       }
        uiGroupSelector++;
    }

    PCTL_LOG(PCTL_LOG_BUG, "does not have pin group %s\r\n", pcPinGroup);
    _ErrorHandle(EINVAL);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlStateFind
** 功能描述: 查找指定名称的引脚状态
** 输　入  : ppinctl           引脚控制
**           pcName            引脚控制状态名称
** 输　出  : 找到的引脚控制
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_PINCTRL_STATE  API_PinCtrlStateFind (PLW_PINCTRL  ppinctl, CPCHAR  pcName)
{
    PLW_PINCTRL_STATE  ppctlstate;

    ppctlstate = __pinCtrlStateFind(ppinctl, pcName);                   /*  查找引脚状态                */
    if (!ppctlstate) {
        ppctlstate = (LW_NULL);
    }

    return  (ppctlstate);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlStateSelect
** 功能描述: 选择指定的引脚状态，应用对应引脚状态的设置
** 输　入  : ppinctl            引脚控制
**           ppctlstate         引脚控制状态
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlStateSelect (PLW_PINCTRL  ppinctl, PLW_PINCTRL_STATE  ppctlstate)
{
    if (!ppinctl) {                                                     /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (ppinctl->PCTL_ppctlstate == ppctlstate) {                       /*  若预设值状态已经为当前状态  */
        return  (ERROR_NONE);
    }

    return  (__pinCtrlStateCommit(ppinctl, ppctlstate));
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlMapDel
** 功能描述: 注销一个引脚控制的引脚映射结构
**           将引脚映射结构从全局的引脚映射链表移除
** 输　入  : ppctlmap      需要注销的引脚映射结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PinCtrlMapDel (PLW_PINCTRL_MAP  ppctlmap)
{
    PLW_PINCTRL_MAPS  ppctlmaps;
    PLW_LIST_LINE     plineTemp;

    __PCTLMAPS_LOCK();
    for (plineTemp  = _G_plinePinCtrlMaps;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppctlmaps = _LIST_ENTRY(plineTemp, LW_PINCTRL_MAPS, PCTLM_lineGlobalManage);
        if (ppctlmaps->PCTLM_ppinctlmaps == ppctlmap) {
            _List_Line_Del(&ppctlmaps->PCTLM_lineGlobalManage, &_G_plinePinCtrlMaps);
            break;
        }
    }
    __PCTLMAPS_UNLOCK();
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlMapAdd
** 功能描述: 注册引脚控制的引脚映射结构
**           将有效的引脚映射结构链入全局的引脚映射链表
** 输　入  : ppctlmaps      需要注册的引脚映射结构集合
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlMapAdd (PLW_PINCTRL_MAPS  ppctlmaps)
{
    PLW_PINCTRL_MAP   ppctlmap;
    UINT              uiNumMaps;
    INT               iRet;
    INT               i;

    if (!ppctlmaps) {                                                   /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppctlmap  = ppctlmaps->PCTLM_ppinctlmaps;
    uiNumMaps = ppctlmaps->PCTLM_uiMapsNum;

    for (i = 0; i < uiNumMaps; i++) {                                   /*  引脚映射有效性检查          */
        if (!ppctlmap[i].PCTLM_pdtnDev ||
            !ppctlmap[i].PCTLM_pcName) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if ((ppctlmap[i].PCTLM_pctlmaptype != PIN_MAP_TYPE_DUMMY_STATE) &&
            !ppctlmap[i].PCTLM_pdtnCtrlNode) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        switch (ppctlmap[i].PCTLM_pctlmaptype) {

        case PIN_MAP_TYPE_DUMMY_STATE:
            break;

        case PIN_MAP_TYPE_MUX_GROUP:                                    /*  引脚复用配置有效性检查      */
            iRet = API_PinMuxMapValidate(&ppctlmap[i], i);
            if (iRet < 0) {
                return  (iRet);
            }
            break;

        case PIN_MAP_TYPE_CONFIGS_PIN:
        case PIN_MAP_TYPE_CONFIGS_GROUP:                                /*  引脚功能配置有效性检查      */
            iRet = API_PinConfigMapValidate(&ppctlmap[i], i);
            if (iRet < 0) {
                return  (iRet);
            }
            break;

        default:
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    __PCTLMAPS_LOCK();
    _List_Line_Add_Ahead(&ppctlmaps->PCTLM_lineGlobalManage, &_G_plinePinCtrlMaps);
    __PCTLMAPS_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlGet
** 功能描述: 获取某个外设相关的引脚控制
** 输　入  : pdtnDev      外设设备树节点
** 输　出  : 查找到的引脚控制
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_PINCTRL  API_PinCtrlGet (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL  ppinctl;

    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    ppinctl = __pinCtrlFind(pdtnDev);                                   /*  查找外设相关的引脚控制      */
    if (ppinctl) {
        return  (ppinctl);
    }

    return  (__pinCtrlCreate(pdtnDev));                                 /*  查找不到则创建引脚控制      */
}
/*********************************************************************************************************
** 函数名称: API_PinBind
** 功能描述: 根据设备树节点中的信息对引脚进行设定
** 输　入  : pdevinstance     设备实体指针
** 输　出  : ERROR_CODE
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinBind (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DEV_PIN_INFO    pdevpininfo = LW_NULL;
    PLW_PINCTRL         ppinctl     = LW_NULL;
    PLW_PINCTRL_STATE   ppctlstate  = LW_NULL;
    PLW_DEVTREE_NODE    pdtnDev     = LW_NULL;
    INT                 iRet;

    if (!pdevinstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtnDev = pdevinstance->DEVHD_pdtnDev;

    pdevpininfo = __SHEAP_ZALLOC(sizeof(LW_DEV_PIN_INFO));
    if (!pdevpininfo) {
        pdevinstance->DEVHD_pdevpininfo = LW_NULL;
        return  (PX_ERROR);
    }
        
    ppinctl = API_PinCtrlGet(pdtnDev);                                  /*  从设备节点中获取引脚信息    */
    if (!ppinctl) {
        PCTL_LOG(PCTL_LOG_LOG, "DTN %s has no pin info.\r\n",
                 pdtnDev->DTN_pcFullName);
        iRet = ERROR_NONE;
        goto __error_handle;
    }
    pdevpininfo->DEVPIN_ppinctl = ppinctl;

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_DEFAULT);  /*  查找默认的设备引脚状态      */
    if (!ppctlstate) {
        iRet = PX_ERROR;
        goto __error_handle;
    }
    pdevpininfo->DEVPIN_ppctlstateDefault = ppctlstate;

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_INIT);     /*  查找 init 设备引脚状态      */
    if (!ppctlstate) {
        ppctlstate = pdevpininfo->DEVPIN_ppctlstateDefault;             /*  没有 init 引脚状态则默认    */
    } else {        
        pdevpininfo->DEVPIN_ppctlstateInit = ppctlstate;
    }

    iRet = API_PinCtrlStateSelect(ppinctl, ppctlstate);                 /*  设置引脚状态                */
    if (iRet) {
        PCTL_LOG(PCTL_LOG_ERR, "DTN %s pin state select failed.\r\n", 
                 pdtnDev->DTN_pcFullName);
        iRet = PX_ERROR;
        goto __error_handle;
    }

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_IDLE);     /*  查找空闲引脚状态            */
    if (ppctlstate) {
        pdevpininfo->DEVPIN_ppctlstateIdle = ppctlstate;
    }

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_SLEEP);    /*  查找睡眠引脚状态            */
    if (ppctlstate) {
        pdevpininfo->DEVPIN_ppctlstateSleep = ppctlstate;
    }

    pdevinstance->DEVHD_pdevpininfo = pdevpininfo;

    return  (ERROR_NONE);

__error_handle:
    __SHEAP_FREE(pdevpininfo);
    pdevinstance->DEVHD_pdevpininfo = LW_NULL;
    
    return  (iRet);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
