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
** 文   件   名: pinMux.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 02 日
**
** 描        述: 引脚复用配置
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "pinCtrl.h"
/*********************************************************************************************************
** 函数名称: __pinMuxFuncNameToSelector
** 功能描述: 将引脚复用类别名称转变为序号
** 输　入  : ppinctldev        引脚控制器
**           pcFunction     引脚复用类别名称
** 输　出  : 转换出的序号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pinMuxFuncNameToSelector (PLW_PINCTRL_DEV  ppinctldev, CPCHAR  pcFunction)
{
    PLW_PINMUX_OPS  ppinmuxops;
    CPCHAR          pcFuncName;
    UINT            uiNFuncs;
    UINT            uiSelector = 0;

    ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    uiNFuncs   = ppinmuxops->pinmuxFuncCountGet(ppinctldev);            /*  获取引脚复用类别功能总数    */

    while (uiSelector < uiNFuncs) {
        pcFuncName = ppinmuxops->pinmuxFuncNameGet(ppinctldev,
                                                   uiSelector);         /*  获取引脚复用类别名称        */
        if (!lib_strcmp(pcFunction, pcFuncName)) {
            return  (uiSelector);
        }
        uiSelector++;
    }

    PCTL_LOG(PCTL_LOG_ERR, "does not support function %s\r\n", pcFunction);

    return  (-EINVAL);
}
/*********************************************************************************************************
** 函数名称: __pinmuxPinRequest
** 功能描述: 引脚复用的引脚申请
** 输　入  : ppinctldev     引脚控制器
**           iPin           引脚序号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pinMuxRequest (PLW_PINCTRL_DEV  ppinctldev,
                             INT              iPin)
{
    PLW_PIN_DESC     pindesc;
    PLW_PINMUX_OPS   ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    INT              iRet;

    pindesc = API_PinCtrlPinDescGet(ppinctldev, iPin);
    if (pindesc == LW_NULL) {                                           /*  如果引脚不存在              */
        return  (PX_ERROR);
    }

    if (ppinmuxops->pinmuxRequest) {
        iRet = ppinmuxops->pinmuxRequest(ppinctldev, iPin);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __pinMuxFree
** 功能描述: 引脚复用的引脚释放
** 输　入  : ppinctldev     引脚控制器
**           iPin           引脚序号
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static  VOID  __pinMuxFree (PLW_PINCTRL_DEV  ppinctldev,
                            INT              iPin)
{
    PLW_PINMUX_OPS   ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    PLW_PIN_DESC     pindesc;

    pindesc = API_PinCtrlPinDescGet(ppinctldev, iPin);
    if (pindesc == LW_NULL) {                                           /*  如果引脚不存在              */
        return;
    }

    if (ppinmuxops->pinmuxFree) {
        ppinmuxops->pinmuxFree(ppinctldev, iPin);
    }
}
/*********************************************************************************************************
** 函数名称: API_PinmuxSettingEnable
** 功能描述: 引脚复用配置使能
** 输　入  : ppinctlsetting      使能的引脚复用配置
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinMuxEnable (PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV         ppinctldev;
    PLW_PINCTRL_OPS         ppinctlops;
    PLW_PINMUX_OPS          ppinmuxops;
    PLW_PIN_DESC            pindesc;
    CPCHAR                  pcPinName   = LW_NULL;
    CPCHAR                  pcGroupName = LW_NULL;
    UINT                   *puiPins     = LW_NULL;
    UINT                    uiNumPins   = 0;
    INT                     iRet        = 0;
    INT                     i;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev) {                            /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;

    if (!ppinctldev->PCTLD_ppinctldesc ||
        !ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops ||
        !ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops) {             /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctlops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
    ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;

    if (ppinctlops->pinctrlGroupPinsGet) {
        iRet = ppinctlops->pinctrlGroupPinsGet(ppinctldev,
                                               ppinctlsetting->set_uiGroup,
                                               &puiPins,
                                               &uiNumPins);             /*  获得引脚集合                */
    }

    if (iRet) {
        if (ppinctlops->pinctrlGroupNameGet) {
            pcGroupName = ppinctlops->pinctrlGroupNameGet(ppinctldev,
                                                          ppinctlsetting->set_uiGroup);
        }
        PCTL_LOG(PCTL_LOG_LOG, "could not get pins for group %s\r\n", pcGroupName);
        uiNumPins = 0;
    }

    for (i = 0; i < uiNumPins; i++) {                                   /*  依次申请每个引脚            */
        iRet = __pinMuxRequest(ppinctldev, puiPins[i]);
        if (iRet) {                                                     /*  申请失败时的信息打印        */
            pindesc   = API_PinCtrlPinDescGet(ppinctldev, puiPins[i]);
            pcPinName = pindesc ? pindesc->PIN_pcName : "non-existing";
            if (ppinctlops->pinctrlGroupNameGet) {
                pcGroupName = ppinctlops->pinctrlGroupNameGet(ppinctldev,
                                                              ppinctlsetting->set_uiGroup);
            }
            PCTL_LOG(PCTL_LOG_LOG, "could not request pin %d (%s) from group %s\r\n",
                     puiPins[i], pcPinName, pcGroupName);
            goto  __error_handle;
        }
    }

    if (ppinmuxops->pinmuxMuxSet) {                                     /*  设置引脚复用功能            */
        iRet = ppinmuxops->pinmuxMuxSet(ppinctldev,
                                        ppinctlsetting->set_uiFunc,
                                        ppinctlsetting->set_uiGroup);
        if (iRet) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    while (--i >= 0) {
        __pinMuxFree(ppinctldev, puiPins[i]);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_PinMuxDisable
** 功能描述: 引脚复用配置禁能
** 输　入  : ppinctlsetting      禁能的引脚复用配置
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PinMuxDisable (PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV   ppinctldev;
    PLW_PINCTRL_OPS   ppinctlops;
    UINT             *puiPins;
    UINT              uiNumPins;
    INT               iRet = PX_ERROR;
    INT               i;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev) {                            /*  参数检查                    */
        return;
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;

    if (!ppinctldev->PCTLD_ppinctldesc ||
        !ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops) {             /*  参数检查                    */
        return;
    }

    ppinctlops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;

    if (ppinctlops->pinctrlGroupPinsGet) {                              /*  获得引脚集合                */
        iRet = ppinctlops->pinctrlGroupPinsGet(ppinctldev,
                                               ppinctlsetting->set_uiGroup,
                                               &puiPins,
                                               &uiNumPins);
    }

    if (iRet) {
        PCTL_LOG(PCTL_LOG_LOG, "could not get pins for group selector %d\r\n",
                 ppinctlsetting->set_uiGroup);
        uiNumPins = 0;
    }

    for (i = 0; i < uiNumPins; i++) {
        __pinMuxFree(ppinctldev, puiPins[i]);
    }
}
/*********************************************************************************************************
** 函数名称: API_PinMuxMapToSetting
** 功能描述: 将引脚映射结构转换为引脚复用配置
** 输　入  : ppinctlmap       用于转换的引脚映射结构
**           ppinctlsetting   转换出的引脚配置
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinMuxMapToSetting (PLW_PINCTRL_MAP  ppinctlmap, PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV   ppinctldev;
    PLW_PINMUX_OPS    ppinmuxops;
    CHAR            **ppGroups;
    UINT              uiNumGroups;
    CPCHAR            pGroup;
    BOOL              bFound = LW_FALSE;
    INT               iRet   = PX_ERROR;
    INT               i;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev ||
        !ppinctlsetting->PCTLS_ppinctldev->PCTLD_ppinctldesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;
    ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    if (!ppinmuxops) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iRet = __pinMuxFuncNameToSelector(ppinctldev,
                                      ppinctlmap->map_pcFunction);      /*  将引脚复用类别名转变为序号  */
    if (iRet < 0) {
        return  (iRet);
    }
    ppinctlsetting->set_uiFunc = iRet;                                  /*  设置引脚复用配置的序号      */

    if (ppinmuxops->pinmuxFuncGroupsGet) {                              /*  获得引脚复用组集合          */
       iRet = ppinmuxops->pinmuxFuncGroupsGet(ppinctldev,
                                              ppinctlsetting->set_uiFunc,
                                              &ppGroups,
                                              &uiNumGroups);            /*  获取引脚复用组集合          */
       if (iRet < 0) {
           return  (iRet);
       }

       if (!uiNumGroups) {                                              /*  对应的引脚组数量为 0        */
           _ErrorHandle(EINVAL);
           return  (PX_ERROR);
       }
    } else {
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    if (ppinctlmap->map_pcGroup) {                                      /*  如果引脚映射包括了组信息    */
        pGroup = ppinctlmap->map_pcGroup;
        for (i = 0; i < uiNumGroups; i++) {
            if (!lib_strcmp(pGroup, ppGroups[i])) {                     /*  找到对应的引脚组            */
                bFound = LW_TRUE;
                break;
            }
        }

        if (!bFound) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    } else {
        pGroup = ppGroups[0];
    }

    iRet = API_PinCtrlGroupSelectorGet(ppinctldev, pGroup);             /*  将引脚组转变为序号          */
    if (iRet < 0) {
        return  (iRet);
    }

    ppinctlsetting->set_uiGroup = iRet;                                 /*  设置引脚配置的引脚组序号    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinMuxMapValidate
** 功能描述: 引脚复用有效性检查
** 输　入  : ppinctlmap    用于检查的引脚映射结构
**           i             引脚映射序号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinMuxMapValidate (PLW_PINCTRL_MAP  ppinctlmap, INT  i)
{
    if (!ppinctlmap || !ppinctlmap->map_pcFunction) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
