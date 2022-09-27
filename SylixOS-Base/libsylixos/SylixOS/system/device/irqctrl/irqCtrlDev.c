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
** 文   件   名: irqCtrlDev.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 06 月 21 日
**
** 描        述: 中断控制器设备
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
#include "irqCtrlDev.h"
/*********************************************************************************************************
  驱动程序全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineIrqctrlDev;
/*********************************************************************************************************
** 函数名称: API_IrqCtrlDevCreate
** 功能描述: 创建一个中断控制器
** 输　入  : pcName           中断控制器的名字
**           pirqctrlfuncs    中断控制器的操作集
**           ulDirectMapMax   中断控制器支持直接映射的最大中断号
**           ulIrqMax         中断控制器需要映射的最大硬件中断号
** 输　出  : 中断控制器
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_IRQCTRL_DEV  API_IrqCtrlDevCreate (CPCHAR             pcName,
                                       PLW_IRQCTRL_FUNCS  pirqctrlfuncs,
                                       ULONG              ulDirectMapMax,
                                       ULONG              ulIrqMax)
{
    PLW_IRQCTRL_DEV  pirqctrldev;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (ulDirectMapMax > ulIrqMax) {                                    /*  参数不符合规则              */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pirqctrldev = (PLW_IRQCTRL_DEV)__SHEAP_ZALLOC(sizeof(LW_IRQCTRL_DEV) + lib_strlen(pcName));
    if (pirqctrldev == LW_NULL) {
        _ErrorHandle(ERROR_POWERM_FULL);
        return  (LW_NULL);
    }

    pirqctrldev->IRQCDEV_pirqctrlfuncs     = pirqctrlfuncs;             /*  允许操作指针集为空          */
    pirqctrldev->IRQCDEV_ulDirectMapIrqMax = ulDirectMapMax;
    pirqctrldev->IRQCDEV_ulIrqMax          = ulIrqMax;
    lib_strcpy(pirqctrldev->IRQCDEV_cName, pcName);

    if (ulDirectMapMax < ulIrqMax) {                                    /*  表示存在部分需要线性映射    */
        pirqctrldev->IRQCDEV_pulLinearMap = (ULONG *)__SHEAP_ZALLOC(ulIrqMax * sizeof(ULONG));
        if (pirqctrldev->IRQCDEV_pulLinearMap == LW_NULL) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            goto  __error_handle;
        }
    }

    _List_Line_Add_Ahead(&pirqctrldev->IRQCDEV_lineManage, &_G_plineIrqctrlDev);

    return  (pirqctrldev);

__error_handle:
    __SHEAP_FREE(pirqctrldev);

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_IrqCtrlDevDelete
** 功能描述: 删除一个中断控制器 (不推荐使用此函数)
** 输　入  : pirqctrldev     中断控制器
** 输　出  : ERROR or OK
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlDevDelete (PLW_IRQCTRL_DEV  pirqctrldev)
{
    if (!pirqctrldev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pirqctrldev->IRQCDEV_pulLinearMap) {
        __SHEAP_FREE(pirqctrldev->IRQCDEV_pulLinearMap);
    }

    _List_Line_Del(&pirqctrldev->IRQCDEV_lineManage, &_G_plineIrqctrlDev);
    __SHEAP_FREE(pirqctrldev);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_IrqCtrlDevFind
** 功能描述: 查询一个中断控制器
** 输　入  : pcName        中断控制器的名字
** 输　出  : 中断控制器
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_IRQCTRL_DEV  API_IrqCtrlDevFind (CPCHAR  pcName)
{
    PLW_LIST_LINE    plineTemp;
    PLW_IRQCTRL_DEV  pirqctrldev;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    for (plineTemp  = _G_plineIrqctrlDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pirqctrldev = _LIST_ENTRY(plineTemp, LW_IRQCTRL_DEV, IRQCDEV_lineManage);
        if (lib_strcmp(pirqctrldev->IRQCDEV_cName, pcName) == 0) {
            return  (pirqctrldev);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_IrqCtrlHwIrqMapToVector
** 功能描述: 将硬件中断号映射到软件中断号
** 输　入  : pirqctrldev     中断控制器
**           ulHwIrq         需要映射的硬件中断号
**           pulVector       映射出的软件中断号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlHwIrqMapToVector (PLW_IRQCTRL_DEV      pirqctrldev,
                                  ULONG                ulHwIrq,
                                  ULONG               *pulVector)
{
    INT  iRet;

    if (pirqctrldev &&
        pirqctrldev->IRQCDEV_pirqctrlfuncs &&
        pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMap) {
        iRet = pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMap(pirqctrldev,
                                                                         ulHwIrq,
                                                                         pulVector);
    } else {
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_IrqCtrlFindVectorByHwIrq
** 功能描述: 根据硬件中断号查找已经映射的软件中断号
** 输　入  : pirqctrldev     中断控制器
**           ulHwIrq         硬件中断号
**           pulVector       软件中断号
** 输　出  : 软件中断号，未找到时，返回 LW_CFG_INTER_INVALID
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlFindVectorByHwIrq (PLW_IRQCTRL_DEV      pirqctrldev,
                                   ULONG                ulHwIrq,
                                   ULONG               *pulVector)
{
    if (!pirqctrldev || !pulVector) {
        return  (PX_ERROR);
    }

    if (ulHwIrq < pirqctrldev->IRQCDEV_ulDirectMapIrqMax) {             /*  支持直接映射                */
        *pulVector = ulHwIrq;
        return  (ERROR_NONE);
    }

    if (ulHwIrq < pirqctrldev->IRQCDEV_ulIrqMax) {                      /*  进行线性映射                */
        *pulVector = pirqctrldev->IRQCDEV_pulLinearMap[ulHwIrq];
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_IrqCtrlDevtreeNodeMatch
** 功能描述: 根据设备树节点查找对应的中断控制器
** 输　入  : pdtnDev         设备树节点
** 输　出  : 中断控制器 或 LW_NULL
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_IRQCTRL_DEV  API_IrqCtrlDevtreeNodeMatch (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_LIST_LINE    plineTemp;
    PLW_IRQCTRL_DEV  pirqctrldev = LW_NULL;
    BOOL             bMatch;

    for (plineTemp  = _G_plineIrqctrlDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pirqctrldev = _LIST_ENTRY(plineTemp, LW_IRQCTRL_DEV, IRQCDEV_lineManage);
        if (pirqctrldev->IRQCDEV_pirqctrlfuncs &&
            pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMatch) {

            bMatch = pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMatch(pirqctrldev, pdtnDev);
            if (bMatch) {
                return  (pirqctrldev);
            }
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_IrqCtrlDevtreeTrans
** 功能描述: 根据设备树参数转换为硬件中断号
** 输　入  : pirqctrldev     中断控制器
**           pdtpaArgs       设备树参数
**           pulHwIrq        转换的硬件中断号
**           uiType          转换的中断类型
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlDevtreeTrans (PLW_IRQCTRL_DEV           pirqctrldev,
                              PLW_DEVTREE_PHANDLE_ARGS  pdtpaArgs,
                              ULONG                    *pulHwIrq,
                              UINT                     *uiType)
{
    INT  iRet;

    if (!pirqctrldev || !pdtpaArgs || !pulHwIrq || !uiType) {
        return  (PX_ERROR);
    }

    if (pirqctrldev->IRQCDEV_pirqctrlfuncs &&
        pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlTrans) {
        iRet = pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlTrans(pirqctrldev,
                                                                           pdtpaArgs,
                                                                           pulHwIrq,
                                                                           uiType);
    } else {
        *pulHwIrq = pdtpaArgs->DTPH_uiArgs[0];
        iRet      = ERROR_NONE;
    }

    return  (iRet);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
