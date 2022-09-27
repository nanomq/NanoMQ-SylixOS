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
** 文   件   名: devtreeDev.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 06 月 21 日
**
** 描        述: 设备树接口设备相关接口实现
**
** 修改：
** 2019.10.22 驱动模型更改，修改此文件
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
#include "linux/bitops.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
  设备总线匹配表
*********************************************************************************************************/
static LW_DEVTREE_TABLE      _G_dttDefaultDevBus[] = {
    { .DTITEM_cCompatible = "simple-bus", },
    { .DTITEM_cCompatible = "simple-mfd", },
    { .DTITEM_cCompatible = "isa",        },
    LW_DEVTREE_TABLE_END
};
/*********************************************************************************************************
  设备总线不匹配表  
*********************************************************************************************************/
static LW_DEVTREE_TABLE      _G_dttSkipDevBus[] = {
    { .DTITEM_cCompatible = "operating-points-v2", },
    LW_DEVTREE_TABLE_END
};
/*********************************************************************************************************
** 函数名称: __deviceTreeIsCompatible
** 功能描述: 检查节点与匹配表项是否匹配
** 输　入  : pdtnDev         指定的节点
**           pcCompatible    匹配表项的兼容属性
**           pcType          匹配表项的类型属性
**           pcName          匹配表项的名称属性
** 输　出  : 匹配的权重值，越大表示越匹配
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeIsCompatible (PLW_DEVTREE_NODE  pdtnDev,
                                      PCHAR             pcCompatible,
                                      PCHAR             pcType,
                                      PCHAR             pcName)
{
    PLW_DEVTREE_PROPERTY  pdtproperty;
    CPCHAR                pcTemp;
    INT                   iIndex = 0;
    INT                   iScore = 0;

    if (pcCompatible && pcCompatible[0]) {                              /*  优先匹配 "compatible"       */
        pdtproperty = API_DeviceTreePropertyFind(pdtnDev, "compatible", LW_NULL);
        for (pcTemp  = API_DeviceTreePropertyStringNext(pdtproperty, LW_NULL);
             pcTemp != LW_NULL;
             pcTemp  = API_DeviceTreePropertyStringNext(pdtproperty, pcTemp), iIndex++) {

            if (lib_strcasecmp(pcTemp, pcCompatible) == 0) {            /*  忽略大小写时完全匹配        */
                iScore = INT_MAX / 2 - (iIndex << 2);                   /*  给予较大的权重              */
                break;
            }
        }

        if (!iScore) {                                                  /*  如果 "compatible" 不能匹配  */
            return  (0);                                                /*  那么此项直接认为不能匹配    */
        }
    }

    if (pcType && pcType[0]) {                                          /*  如果要求匹配 type 类型      */
        if (!pdtnDev->DTN_pcType ||
            lib_strcasecmp(pcType, pdtnDev->DTN_pcType)) {              /*  type 类型不能匹配           */
            return  (0);                                                /*  此项认为不能匹配            */
        }

        iScore += 2;                                                    /*  type 类型匹配，权重增加     */
    }

    if (pcName && pcName[0]) {                                          /*  如果要求匹配 name           */
        if (!pdtnDev->DTN_pcName ||
            lib_strcasecmp(pcName, pdtnDev->DTN_pcName)) {              /*  name 不能匹配               */
            return  (0);                                                /*  此项认为不能匹配            */
        }

        iScore++;                                                       /*  name 匹配，权重增加         */
    }

    return  (iScore);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodeMatch
** 功能描述: 检查节点与匹配表是否匹配
** 输　入  : pdtnDev         指定的节点
**           pdttMatch       匹配的表格
** 输　出  : 若有匹配项，返回 最合适的表项；若没有匹配项，返回 LW_NULL。
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static const PLW_DEVTREE_TABLE  __deviceTreeNodeMatch (PLW_DEVTREE_NODE   pdtnDev,
                                                       PLW_DEVTREE_TABLE  pdttMatch)
{
    PLW_DEVTREE_TABLE  pdttBestMatch = LW_NULL;
    INT                iScore;
    INT                iBestScore      = 0;

    if (!pdttMatch) {
        return  (LW_NULL);
    }

    for (;
         pdttMatch->DTITEM_cName[0] ||
         pdttMatch->DTITEM_cType[0] ||
         pdttMatch->DTITEM_cCompatible[0];
         pdttMatch++) {                                                 /*  逐个对比匹配表中的表项      */

        iScore = __deviceTreeIsCompatible(pdtnDev,
                                          pdttMatch->DTITEM_cCompatible,
                                          pdttMatch->DTITEM_cType,
                                          pdttMatch->DTITEM_cName);
        if (iScore > iBestScore) {                                      /*  找到最合适的表项            */
            pdttBestMatch   = pdttMatch;
            iBestScore      = iScore;
        }
    }

    return  (pdttBestMatch);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeDevCreate
** 功能描述: 创建设备
** 输　入  : pdtnDev             指定的节点
** 输　出  : 创建成功，返回设备实例；失败，返回 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_DEV_INSTANCE  __deviceTreeDevCreate (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEV_INSTANCE   pdevInstance;

    pdevInstance = __SHEAP_ZALLOC(sizeof(LW_DEV_INSTANCE));
    if (LW_NULL == pdevInstance) {
        return  (LW_NULL);
    }
    
    pdevInstance->DEVHD_pdtnDev = pdtnDev;
    pdevInstance->DEVHD_pcName  = pdtnDev->DTN_pcFullName;

    API_PlatformDeviceRegister(pdevInstance);
    __deviceTreeNodeFlagSet(pdtnDev, OF_POPULATED);                     /*  标记节点被加载              */
    
    return  (pdevInstance);
} 
/*********************************************************************************************************
** 函数名称: __deviceTreeDevPopulate
** 功能描述: 设备树设备节点与设备实例链表匹配
** 输　入  : pdtnDev             指定的节点
**           pdttMatch          指定的匹配表
** 输　出  : 匹配成功，返回匹配的设备实例；匹配失败，返回 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __deviceTreeDevPopulate (PLW_DEVTREE_NODE        pdtnDev,
                                    PLW_DEVTREE_TABLE       pdttMatch)
{
    INT                iRet = ERROR_NONE;
    PLW_DEVTREE_NODE   pdtnChild;
    PLW_DEV_INSTANCE   pdevInstance;
    PCHAR              pcCompat;

    pcCompat = API_DeviceTreePropertyGet(pdtnDev, "compatible", LW_NULL);
    if (pcCompat) {
        DEVTREE_MSG("%s() - node %s compatible : %s\r\n",
                    __func__, pdtnDev->DTN_pcFullName, pcCompat);
    } else {
        return  (ERROR_NONE);
    }

    if (__deviceTreeNodeFlagCheck(pdtnDev, OF_POPULATED_BUS)) {         /*  已经匹配过的不再匹配        */
        DEVTREE_MSG("%s() - skipping %s, already populated\r\n",
                    __func__, pdtnDev->DTN_pcFullName);

        return  (ERROR_NONE);
    }

    if (__deviceTreeNodeMatch(pdtnDev, _G_dttSkipDevBus)) {             /*  跳过不需要匹配的节点        */
        return  (ERROR_NONE);
    }
    
    pdevInstance = __deviceTreeDevCreate(pdtnDev);                      /*  创建平台设备                */
    if (LW_NULL == pdevInstance) {
        return  (ERROR_NONE);
    }

    if (!__deviceTreeNodeMatch(pdtnDev, pdttMatch)) {                   /*  进行总线兼容性匹配          */
        return  (ERROR_NONE);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  继续遍历该节点的子节点      */
        iRet = __deviceTreeDevPopulate(pdtnChild, pdttMatch);
        if (iRet) {
            break;
        }
    }

    return  (iRet);
}  
/*********************************************************************************************************
** 函数名称: API_DeviceTreeDrvMatchDev
** 功能描述: 设备和驱动对外匹配接口
** 输　入  : pdevInstance    设备实例指针
**           pdrvInstance    驱动实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDrvMatchDev (PLW_DEV_INSTANCE   pdevInstance,
                                PLW_DRV_INSTANCE   pdrvInstance)
{
    if (!pdevInstance || !pdrvInstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__deviceTreeNodeMatch(pdevInstance->DEVHD_pdtnDev,
                              pdrvInstance->DRVHD_pMatchTable)) {       /*  匹配设备实例的匹配表        */
        return  (ERROR_NONE);
    } 

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeDevPopulate
** 功能描述: 设备树设备节点匹配加载
** 输　入  : pdtnDev          指定的节点
**           pdttMatch        指定的匹配表
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDevPopulate (PLW_DEVTREE_NODE       pdtnDev,
                                PLW_DEVTREE_TABLE      pdttMatch)
{
    PLW_DEVTREE_NODE   pdtnChild;

    pdtnDev = pdtnDev ? pdtnDev : __deviceTreeFindNodeByPath("/");
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  继续遍历该节点的子节点      */
        if (__deviceTreeDevPopulate(pdtnChild, pdttMatch)) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeDevEarlyInit
** 功能描述: 设备提前初始化接口
** 输　入  : pdttMatch    匹配表
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDevEarlyInit (PLW_DEVTREE_TABLE  pdttMatch)
{
    PLW_DEVTREE_NODE   pdtnDev;
    DEVTREE_INIT_FUNC  pfuncInitCb;
    PLW_DEVTREE_TABLE  pdttMatchTmp;

    _LIST_EACH_OF_ALLNODES(pdtnDev) {                                   /*  遍历结点，加载中断控制器    */
        if (!API_DeviceTreePropertyGet(pdtnDev, "compatible", LW_NULL)) {
            DEVTREE_MSG("%s() - skipping %s, no compatible prop\r\n",
                        __func__, pdtnDev->DTN_pcName);
            continue;
        }

        if (__deviceTreeNodeFlagCheck(pdtnDev, OF_POPULATED)) {         /*  已经匹配过的不再匹配        */
            DEVTREE_MSG("%s() - skipping %s, already populated\r\n",
                        __func__, pdtnDev->DTN_pcName);
            continue;
        }

        pdttMatchTmp = __deviceTreeNodeMatch(pdtnDev, pdttMatch);
        if (LW_NULL != pdttMatchTmp) {
            __deviceTreeNodeFlagSet(pdtnDev, OF_POPULATED);             /*  标记节点被加载              */
            pfuncInitCb = pdttMatchTmp->DTITEM_pvData;
            if (pfuncInitCb) {                                          /*  如果找到匹配项              */
                pfuncInitCb(pdtnDev);                                   /*  执行设备实例的 Probe 函数   */
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeDefaultPopulate
** 功能描述: 设备树外设节点加载初始化入口
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDefaultPopulate (VOID)
{
    DEVTREE_MSG("Device populate from root.\r\n");
    
    return  (API_DeviceTreeDevPopulate(NULL, _G_dttDefaultDevBus));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIsCompatible
** 功能描述: 检查节点与匹配表项是否匹配
** 输　入  : pdtnDev      指定的节点
**           pcCompat     匹配表项的兼容属性
** 输　出  : 匹配的权重值，越大表示越匹配
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIsCompatible (PLW_DEVTREE_NODE  pdtnDev,
                                 PCHAR             pcCompat)
{
    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    return  (__deviceTreeIsCompatible(pdtnDev, pcCompat, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeDevGetMatchData
** 功能描述: 设备树设备节点获取匹配表数据
** 输　入  : pdevInstance     设备节点
** 输　出  : 匹配表数据
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PVOID  API_DeviceTreeDevGetMatchData (PLW_DEV_INSTANCE  pdevInstance)
{
    PLW_DEVTREE_TABLE  pdttMatch;

    if (!pdevInstance) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pdttMatch = __deviceTreeNodeMatch(pdevInstance->DEVHD_pdtnDev,
                                      pdevInstance->DEVHD_pdrvinstance->DRVHD_pMatchTable);
    if (!pdttMatch) {
        return  (LW_NULL);
    }

    return  ((PVOID)pdttMatch->DTITEM_pvData);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeDevGetMatchTable
** 功能描述: 判断设备树节点在指定的匹配表格组中是否有匹配项
** 输　入  : pdttMatches     匹配表格组
**           pdtnDev         指定的节点
** 输　出  : 若有匹配项，返回最合适的表项；若没有匹配项，返回 LW_NULL。
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_DEVTREE_TABLE  API_DeviceTreeDevGetMatchTable (PLW_DEVTREE_TABLE  pdttMatches,
                                                   PLW_DEVTREE_NODE   pdtnDev)
{
    if (!pdtnDev || !pdttMatches) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (__deviceTreeNodeMatch(pdtnDev, pdttMatches));
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
