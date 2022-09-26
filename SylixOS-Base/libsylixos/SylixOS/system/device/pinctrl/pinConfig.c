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
** 文   件   名: pinConfig.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 12 日
**
** 描        述: pinctrl 子系统中的引脚配置实现
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
** 函数名称: API_PinConfigApply
** 功能描述: 应用指定的引脚配置
** 输　入  : ppinctlsetting        指定的引脚配置
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinConfigApply (PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV  ppinctldev;
    PLW_PINCONF_OPS  ppinconfops;
    INT              iRet;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev ||
        !ppinctlsetting->PCTLS_ppinctldev->PCTLD_ppinctldesc) {         /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev  = ppinctlsetting->PCTLS_ppinctldev;
    ppinconfops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinconfops;
    if (!ppinconfops) {                                                 /*  配置函数集为空              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    switch (ppinctlsetting->PCTLS_pinctlmaptype) {

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  配置单个引脚                */
        if (!ppinconfops->pinConfigSet) {
            _ErrorHandle(ENOSYS);
            return  (PX_ERROR);
        }

        iRet = ppinconfops->pinConfigSet(ppinctldev,
                                         ppinctlsetting->set_uiGroupOrPin,
                                         ppinctlsetting->set_pulConfigs,
                                         ppinctlsetting->set_uiNumConfigs);
        if (iRet < 0) {
            return  (iRet);
        }
        break;

    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  按组进行引脚配置            */
        if (!ppinconfops->pinConfigGroupSet) {
            _ErrorHandle(ENOSYS);
            return  (PX_ERROR);
        }

        iRet = ppinconfops->pinConfigGroupSet(ppinctldev,
                                              ppinctlsetting->set_uiGroupOrPin,
                                              ppinctlsetting->set_pulConfigs,
                                              ppinctlsetting->set_uiNumConfigs);
        if (iRet < 0) {
            return  (iRet);
        }
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinConfigMapToSetting
** 功能描述: 将引脚映射结构转换为引脚配置
** 输　入  : ppinctrlmap           用于转换的引脚映射结构
**           ppinctlsetting        转换出的引脚配置
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinConfigMapToSetting (PLW_PINCTRL_MAP  ppinctrlmap, PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV  ppinctldev;
    INT              iPin;

    if (!ppinctrlmap || !ppinctlsetting) {                              /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;

    switch (ppinctlsetting->PCTLS_pinctlmaptype) {

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  转换单个引脚功能配置        */
        iPin = API_PinCtrlPinGetByName(ppinctldev,
                                       ppinctrlmap->map_pcGroupOrPin);  /*  根据名称获取引脚序号        */
        if (iPin < 0) {
            PCTL_LOG(PCTL_LOG_BUG, "map pin config \"%s\" error\r\n",
                     ppinctrlmap->map_pcGroupOrPin);
            return  (iPin);
        }
        ppinctlsetting->set_uiGroupOrPin = iPin;
        break;

    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  转换按组引脚功能配置        */
        iPin = API_PinCtrlGroupSelectorGet(ppinctldev,
                                           ppinctrlmap->map_pcGroupOrPin);
                                                                        /*  根据名称获取引脚组序号      */
        if (iPin < 0) {
            PCTL_LOG(PCTL_LOG_BUG, "map group config \"%s\" error\r\n",
                     ppinctrlmap->map_pcGroupOrPin);
            return  (iPin);
        }
        ppinctlsetting->set_uiGroupOrPin = iPin;
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctlsetting->set_uiNumConfigs = ppinctrlmap->map_uiNumConfigs;   /*  设置引脚功能配置数据        */
    ppinctlsetting->set_pulConfigs   = ppinctrlmap->map_pulConfigs;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinConfigMapValidate
** 功能描述: 引脚映射结构有效性检查
** 输　入  : ppinctrlmap   用于检查的引脚映射结构
**           i             引脚映射结构元素序号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinConfigMapValidate (PLW_PINCTRL_MAP  ppinctrlmap, INT  i)
{
    if (!ppinctrlmap) {                                                 /*  参数检查                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!ppinctrlmap->map_pcGroupOrPin) {
        PCTL_LOG(PCTL_LOG_BUG, "register map %s (%d) failed: no group/pin given\r\n",
                 ppinctrlmap->PCTLM_pcName, i);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!ppinctrlmap->map_uiNumConfigs ||
        !ppinctrlmap->map_pulConfigs) {
        PCTL_LOG(PCTL_LOG_BUG, "register map %s (%d) failed: no configs given\r\n",
                 ppinctrlmap->PCTLM_pcName, i);
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
