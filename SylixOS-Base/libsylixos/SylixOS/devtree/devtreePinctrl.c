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
** 文   件   名: devtreePinctrl.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 02 日
**
** 描        述: 设备树接口引脚相关接口实现
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
** 函数名称: __deviceTreeMapFree
** 功能描述: 释放设备树使用的引脚映射结构
** 输　入  : ppinctrldev      引脚控制器
**           ppinctrlmap      释放的引脚映射结构
**           uiNumMaps        引脚映射结构的元素数量
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreeMapFree (PLW_PINCTRL_DEV       ppinctrldev,
                                  PLW_PINCTRL_MAP       ppinctrlmap,
                                  UINT                  uiNumMaps)
{
    PLW_PINCTRL_OPS  ppinctrlops;

    if (ppinctrldev) {
        ppinctrlops = ppinctrldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
        ppinctrlops->pinctrlMapFree(ppinctrldev, ppinctrlmap, uiNumMaps);

    } else {
        __SHEAP_FREE(ppinctrlmap);
    }
}
/*********************************************************************************************************
** 函数名称: __deviceTreeRememberOrFreeMap
** 功能描述: 增加非 DUMMY 的引脚复用映射结构
** 输　入  : ppinctrl        引脚控制
**           pcStateName     关联的状态
**           ppinctrldev     关联的引脚控制器
**           ppinctrlmap     引脚映射结构
**           uiNumMaps       引脚映射结构元素数量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeRememberOrFreeMap (PLW_PINCTRL           ppinctrl,
                                           CPCHAR                pcStateName,
                                           PLW_PINCTRL_DEV       ppinctrldev,
                                           PLW_PINCTRL_MAP       ppinctrlmap,
                                           UINT                  uiNumMaps)
{
    PLW_PINCTRL_MAPS    ppinctlmaps;
    INT                 i;

    for (i = 0; i < uiNumMaps; i++) {                                   /*  填充引脚映射结构中剩余部分  */
        ppinctrlmap[i].PCTLM_pdtnDev = ppinctrl->PCTL_pdtnDev;          /*  记录该引脚配置的设备树节点  */
        ppinctrlmap[i].PCTLM_pcName  = pcStateName;                     /*  记录该引脚配置的状态名称    */
        if (ppinctrldev) {
            ppinctrlmap[i].PCTLM_pdtnCtrlNode = ppinctrldev->PCTLD_pdtnDev;
                                                                        /*  记录引脚控制器的设备树节点  */
        }
    }

    ppinctlmaps = (PLW_PINCTRL_MAPS)__SHEAP_ALLOC(sizeof(LW_PINCTRL_MAPS));
    if (!ppinctlmaps) {
        __deviceTreeMapFree(ppinctrldev, ppinctrlmap, uiNumMaps);
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    ppinctlmaps->PCTLM_ppinctldev   = ppinctrldev;                      /*  记录引脚控制器              */
    ppinctlmaps->PCTLM_ppinctlmaps  = ppinctrlmap;                      /*  记录引脚映射数组            */
    ppinctlmaps->PCTLM_uiMapsNum    = uiNumMaps;                        /*  记录引脚映射数组元素个数    */
    _List_Line_Add_Ahead(&ppinctlmaps->PCTLM_lineManage,
                         &ppinctrl->PCTL_plinemaps);                    /*  添加引脚映射结构节点        */

    return  (API_PinCtrlMapAdd(ppinctlmaps));                           /*  注册引脚映射结构集合        */
}
/*********************************************************************************************************
** 函数名称: __deviceTreeRememberDummyState
** 功能描述: 增加虚拟的引脚映射结构
** 输　入  : ppinctrl        引脚控制
**           pcStateName     关联的状态
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeRememberDummyState (PLW_PINCTRL  ppinctrl, CPCHAR  pcStateName)
{
    PLW_PINCTRL_MAP       ppinctrlmap;

    ppinctrlmap = (PLW_PINCTRL_MAP)__SHEAP_ALLOC(sizeof(LW_PINCTRL_MAP));
    if (!ppinctrlmap) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    ppinctrlmap->PCTLM_pctlmaptype = PIN_MAP_TYPE_DUMMY_STATE;

    return  (__deviceTreeRememberOrFreeMap(ppinctrl, pcStateName, LW_NULL, ppinctrlmap, 1));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeOneConfigMap
** 功能描述: 解析一条设备树引脚配置转换为引脚映射结构
** 输　入  : ppinctrl        引脚控制
**           pcStateName     对应的状态
**           pdtnConfig      引脚配置的设备树节点
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeOneConfigMap (PLW_PINCTRL           ppinctrl,
                                      CPCHAR                pcStateName,
                                      PLW_DEVTREE_NODE      pdtnConfig)
{
    PLW_DEVTREE_NODE      pdtnPinctrlDev;
    PLW_PINCTRL_DEV       ppinctrldev;
    PLW_PINCTRL_OPS       ppinctrlops;
    PLW_PINCTRL_MAP       ppinctrlmap = LW_NULL;
    UINT                  uiNunMaps;
    INT                   iRet;

    pdtnPinctrlDev = pdtnConfig;
    while (1) {
        pdtnPinctrlDev = pdtnPinctrlDev->DTN_pdtnparent;                /*  查找引脚控制器节点          */
        if (!pdtnPinctrlDev ||                                          /*  如果没有父节点              */
            !pdtnPinctrlDev->DTN_pdtnparent) {                          /*  或者父节点为根节点          */
            _ErrorHandle(ERROR_DEVTREE_EPROBE_DEFER);
            return  (PX_ERROR);
        }

        ppinctrldev = API_PinCtrlDevGetByDevtreeNode(pdtnPinctrlDev);   /*  找到引脚控制器              */
        if (ppinctrldev) {
            break;
        }

        if (pdtnPinctrlDev == ppinctrl->PCTL_pdtnDev) {                 /*  若引脚控制关联的外设是引脚  */
            _ErrorHandle(ENODEV);                                       /*  控制器                      */
            return  (PX_ERROR);
        }
    }

    ppinctrlops = ppinctrldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
    if (!ppinctrlops->pinctrlMapCreate) {                               /*  若没有定义设备树转换方法    */
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    iRet = ppinctrlops->pinctrlMapCreate(ppinctrldev,
                                         pdtnConfig,
                                         &ppinctrlmap,
                                         &uiNunMaps);                   /*  调用引脚控制器解析接口      */
    if (iRet < 0) {
        return  (iRet);
    }

    if (!ppinctrlmap) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (__deviceTreeRememberOrFreeMap(ppinctrl, pcStateName, ppinctrldev, ppinctrlmap, uiNunMaps));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePinCtrlMapsFree
** 功能描述: 释放设备树的引脚映射结构
** 输　入  : ppinctrl         引脚控制
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_DeviceTreePinCtrlMapsFree (PLW_PINCTRL  ppinctrl)
{
    PLW_PINCTRL_MAPS    pinctrlmaps;
    PLW_LIST_LINE       plineTemp;

    if (!ppinctrl) {
        return;
    }

    for (plineTemp  = ppinctrl->PCTL_plinemaps;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历引脚控制的映射链表      */

        pinctrlmaps = _LIST_ENTRY(plineTemp, LW_PINCTRL_MAPS, PCTLM_lineManage);

        API_PinCtrlMapDel(pinctrlmaps->PCTLM_ppinctlmaps);              /*  从全局链表中移除            */

        _List_Line_Del(&pinctrlmaps->PCTLM_lineManage,
                       &ppinctrl->PCTL_plinemaps);                      /*  从引脚控制链表中移除        */

        __deviceTreeMapFree(pinctrlmaps->PCTLM_ppinctldev,
                            pinctrlmaps->PCTLM_ppinctlmaps,
                            pinctrlmaps->PCTLM_uiMapsNum);              /*  释放引脚映射内存            */
        __SHEAP_FREE(pinctrlmaps);
    }
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePinCtrlMapsCreate
** 功能描述: 将引脚控制对应的设备树解析成引脚映射结构
** 输　入  : ppinctrl        引脚控制
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePinCtrlMapsCreate (PLW_PINCTRL   ppinctrl)
{
    PLW_DEVTREE_NODE      pdtnDev;
    PLW_DEVTREE_NODE      pdtnConfig;
    PLW_DEVTREE_PROPERTY  pdtproperty;
    CPCHAR                pcStateName;
    CHAR                  cPropName[30];
    UINT32               *puiList;
    UINT32                uiPhandle;
    INT                   iState = 0;
    INT                   iConfig;
    INT                   iSize;
    INT                   iRet;

    if (!ppinctrl) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtnDev = ppinctrl->PCTL_pdtnDev;

    while (1) {                                                         /*  按序号查找引脚控制属性      */
        snprintf(cPropName, 30, "pinctrl-%d", iState);
        pdtproperty = API_DeviceTreePropertyFind(pdtnDev, cPropName, &iSize);
        if (!pdtproperty) {                                             /*  如果已经找不到对应序号      */
            if (0 == iState) {
                return  (PX_ERROR);
            }
            break;
        }

        puiList = pdtproperty->DTP_pvValue;                             /*  获取 phandle                */
        iSize  /= sizeof(UINT32);

        iRet = API_DeviceTreePropertyStringIndexRead(pdtnDev,
                                                     "pinctrl-names",
                                                     iState,
                                                     &pcStateName);     /*  查找对应引脚名称            */
        if (iRet < 0) {                                                 /*  未找到名称属性时的名称方式  */
            pcStateName = pdtproperty->DTP_pcName + 8;
        }

        for (iConfig = 0; iConfig < iSize; iConfig++) {
            uiPhandle   = BE32_TO_CPU(puiList++);                       /*  根据引脚控制获取 phandle    */
            pdtnConfig = API_DeviceTreeFindNodeByPhandle(uiPhandle);    /*  由 phandle 找对应设备树节点 */
            if (!pdtnConfig) {
                DEVTREE_ERR("prop %s index %i invalid phandle\r\n",
                            pdtproperty->DTP_pcName, iConfig);
                iRet = -EINVAL;
                goto  __error_handle;
            }

            iRet = __deviceTreeOneConfigMap(ppinctrl,
                                            pcStateName,
                                            pdtnConfig);                /*  解析该引脚控制配置          */
            if (iRet < 0) {
                goto  __error_handle;
            }
        }

        if (!iSize) {                                                   /*  如果引脚控制没有属性值      */
            iRet = __deviceTreeRememberDummyState(ppinctrl, pcStateName);
            if (iRet < 0) {
                goto  __error_handle;
            }
        }

        iState++;
    }

    return  (ERROR_NONE);

__error_handle:
    API_DeviceTreePinCtrlMapsFree(ppinctrl);
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePinCtrlDevGet
** 功能描述: 根据设备树节点获取引脚控制器
** 输　入  : pdtnDev      设备树节点
** 输　出  : 引脚控制器
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DEV  API_DeviceTreePinCtrlDevGet (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL_DEV  ppinctrldev;

    ppinctrldev = API_PinCtrlDevGetByDevtreeNode(pdtnDev);

    return  (ppinctrldev);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
