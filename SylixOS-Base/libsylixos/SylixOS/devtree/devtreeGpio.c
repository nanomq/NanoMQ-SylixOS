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
** 文   件   名: devtreeGpio.c
**
** 创   建   人: Zhao.Bing (赵冰)
**
** 文件创建日期: 2021 年 11 月 04 日
**
** 描        述: 设备树接口 GPIO 相关接口实现
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_GPIO_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree.h"
/*********************************************************************************************************
** 函数名称: __gpioDevtreeNodeMatch
** 功能描述: 比较 GPIO 控制器中设备树节点与传入节点是否一致
** 输　入  : pgchip            GPIO 控制器
**           pvData            待比较的设备树节点
** 输　出  : LW_TRUE  一致, LW_FALSE  不一致
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __gpioDevtreeNodeMatch (PLW_GPIO_CHIP  pgchip, PVOID  pvData)
{
    PLW_DT_GPIO_CTRL   pdtgpioctrl = (PLW_DT_GPIO_CTRL)pgchip;
    PLW_DEVTREE_NODE   pdtnDev     = pdtgpioctrl->DTGPIOCTRL_pdevinstance->DEVHD_pdtnDev;
    PLW_DEVTREE_NODE   pdtnCmpNode = (PLW_DEVTREE_NODE)pvData;

    return  (pdtnDev == pdtnCmpNode);
}
/*********************************************************************************************************
** 函数名称: __gpioNamedGpioDescGet
** 功能描述: 获取设备树节点中 GPIO 列表的指定位置的 GPIO 描述结构
** 输　入  : pdtnDev            设备树节点
**           pcListName         GPIO 列表的属性名
**           iIndex             列表中的位置索引
** 输　出  : GPIO 描述结构
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_GPIO_DESC  __gpioNamedGpioDescGet (PLW_DEVTREE_NODE  pdtnDev,
                                              CPCHAR            pcListName,
                                              INT               iIndex)
{
    INT                         iRet;
    LW_DEVTREE_PHANDLE_ARGS     dtpaGpio;
    PLW_GPIO_CHIP               pgchip;
    PLW_GPIO_DESC               pgdesc;
    UINT                        uiOffset;

    /*
     *  获取指定位置的 Phandle
     */
    iRet = API_DeviceTreePhandleParseWithArgs(pdtnDev, pcListName, "#gpio-cells",
                                              iIndex, &dtpaGpio);
    if (iRet != ERROR_NONE) {
        return  (LW_NULL);
    }

    /*
     *  查找对应的 GPIO 控制器
     */
    pgchip = API_GpioChipFind(dtpaGpio.DTPH_pdtnDev, __gpioDevtreeNodeMatch);
    if (!pgchip) {
        _ErrorHandle(ENODEV);
        return  (LW_NULL);
    }

    uiOffset = dtpaGpio.DTPH_uiArgs[0];
    if (uiOffset >= pgchip->GC_uiNGpios) {                              /*  GPIO 偏移超出范围           */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pgdesc = &(pgchip->GC_gdDesc[uiOffset]);

    return  (pgdesc);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeGpioCtrlRegister
** 功能描述: GPIO 控制器注册
** 输　入  : pdtgpioctrl      GPIO 控制器指针
**           pcName           GPIO 控制器名称
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioCtrlRegister (PLW_DT_GPIO_CTRL  pdtgpioctrl, CPCHAR  pcName)
{
    return  (API_GpioCtrlRegister(pdtgpioctrl, pcName));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeGpioCtrlRemove
** 功能描述: 移除 GPIO 控制器
** 输　入  : pdtgpioctrl      GPIO 控制器指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_DeviceTreeGpioCtrlRemove (PLW_DT_GPIO_CTRL  pdtgpioctrl)
{
    API_GpioCtrlUnregister(pdtgpioctrl);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeGpioPinRangeAdd
** 功能描述: 向 PINCTRL 控制器中添加 GPIO 引脚范围
** 输　入  : pdtgpioctrl      GPIO 控制器指针
**           ppinctldev       PINCTRL 控制器指针
**           uiGpioOffset     GPIO 偏移
**           uiPinOffset      引脚偏移
**           uiNPins          引脚数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioPinRangeAdd (PLW_DT_GPIO_CTRL   pdtgpioctrl,
                                    PLW_PINCTRL_DEV    ppinctldev,
                                    UINT               uiGpioOffset,
                                    UINT               uiPinOffset,
                                    UINT               uiNPins)
{
    PLW_GPIO_PIN_RANGE      pgpiopinrange;

    pgpiopinrange = (PLW_GPIO_PIN_RANGE)__SHEAP_ZALLOC(sizeof(LW_GPIO_PIN_RANGE));
    if (!pgpiopinrange) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    pgpiopinrange->GPRANGE_ppinctldev = ppinctldev;

    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_pcName     = pdtgpioctrl->DTGPIOCTRL_cName;
    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_uiGpioBase = pdtgpioctrl->DTGPIOCTRL_uiBase + uiGpioOffset;
    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_uiPinBase  = uiPinOffset;
    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_uiNPins    = uiNPins;

    if (API_PinCtrlGpioRangeAdd(ppinctldev, &pgpiopinrange->GPRANGE_gpioRange) != ERROR_NONE) {
        __SHEAP_FREE(pgpiopinrange);
        return  (PX_ERROR);
    }

    _List_Line_Add_Ahead(&pgpiopinrange->GPRANGE_lineManage, &pdtgpioctrl->DTGPIOCTRL_plineGpioRange);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeGpioPinRangeRemove
** 功能描述: 从 PINCTRL 控制器中移除 GPIO 控制器对应的所有引脚范围
** 输　入  : pdtgpioctrl      GPIO 控制器指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_DeviceTreeGpioPinRangeRemove (PLW_DT_GPIO_CTRL  pdtgpioctrl)
{
    PLW_LIST_LINE           plineTemp;
    PLW_GPIO_PIN_RANGE      pgpiopinrange;

    for (plineTemp  = pdtgpioctrl->DTGPIOCTRL_plineGpioRange;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历引脚控制的映射链表      */

        pgpiopinrange = _LIST_ENTRY(plineTemp, LW_GPIO_PIN_RANGE, GPRANGE_lineManage);

        API_PinCtrlGpioRangeRemove(pgpiopinrange->GPRANGE_ppinctldev,
                                   &pgpiopinrange->GPRANGE_gpioRange);
        _List_Line_Del(plineTemp, &pdtgpioctrl->DTGPIOCTRL_plineGpioRange);
        __SHEAP_FREE(pgpiopinrange);
    }
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeGpioNamedGpioGet
** 功能描述: 获取设备树节点中 GPIO 列表的指定位置的 GPIO 号
** 输　入  : pdtnDev         设备树节点
**           pcListName      GPIO 列表的属性名
**           iIndex          列表中的索引
** 输　出  : GPIO 号
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioNamedGpioGet (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcListName, INT  iIndex)
{
    PLW_GPIO_DESC   pgdesc;

    pgdesc = __gpioNamedGpioDescGet(pdtnDev, pcListName, iIndex);
    if (!pgdesc) {
        return  (PX_ERROR);
    }

    return  (DESC_TO_GPIO(pgdesc));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeGpioNamedCountGet
** 功能描述: 获取设备树节点中 GPIO 列表的项目数量
** 输　入  : pdtnDev         设备树节点
**           pcListName      GPIO 列表的属性名
** 输　出  : 项目数量
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioNamedCountGet (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcListName)
{
    return  (API_DeviceTreePhandleCountWithArgs(pdtnDev, pcListName, "#gpio-cells"));
}

#endif                                                                  /*  (LW_CFG_GPIO_EN > 0) &&     */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
