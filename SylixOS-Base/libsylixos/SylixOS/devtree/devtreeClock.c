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
** 文   件   名: devtreeClock.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 09 月 02 日
**
** 描        述: 设备树接口时钟相关接口实现
*********************************************************************************************************/
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
** 函数名称: API_DeviceTreeClockGetByName
** 功能描述: 通过时钟名获取时钟结构, 如果 pcName 为 LW_NULL, 查找首个时钟.
** 输　入  : pdtnDev     设备树节点
**           pcName      时钟名称
** 输　出  : 时钟结构
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_DeviceTreeClockGetByName (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcName)
{
    PLW_CLOCK  pclk   = LW_NULL;
    INT        iIndex;

    while (pdtnDev) {
        iIndex = 0;                                                     /*  循环查找每次需要重置索引    */

        if (pcName) {
            iIndex = API_DeviceTreePropertyStringMatch(pdtnDev, "clock-names", pcName);
        }

        pclk = API_DeviceTreeClockGet(pdtnDev, iIndex);
        if (pclk) {
            return  (pclk);
        }

        if (pcName && (iIndex >= 0)) {
            break;
        }

        pdtnDev = pdtnDev->DTN_pdtnparent;
        if (pdtnDev && !API_DeviceTreePropertyGet(pdtnDev, "clock-ranges", LW_NULL)) {
            break;
        }
    }

    return  (pclk);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeParentClockNameGet
** 功能描述: 获取指定序号的父时钟名称
** 输　入  : pdtnDev     设备树节点
**           iIndex      指定的时钟序号
** 输　出  : 父时钟名称
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
CPCHAR  API_DeviceTreeParentClockNameGet (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex)
{
    LW_DEVTREE_PHANDLE_ARGS   pdtpaClkSpec;
    PLW_DEVTREE_PROPERTY      pdtproperty;
    PLW_CLOCK                 pclk;
    CPCHAR                    pcClkName;
    UINT                     *puiVal;
    UINT32                    uiPropVal;
    INT                       iRet;
    INT                       iCount;

    iRet = API_DeviceTreePhandleParseWithArgs(pdtnDev,
                                              "clocks",
                                              "#clock-cells",
                                              iIndex,
                                              &pdtpaClkSpec);
    if (iRet) {
        return  (LW_NULL);
    }

    iIndex = pdtpaClkSpec.DTPH_iArgsCount ? pdtpaClkSpec.DTPH_uiArgs[0] : 0;
    iCount = 0;

    _LIST_EACH_OF_UINT32_PROPERTY(pdtpaClkSpec.DTPH_pdtnDev,
                                  "clock-indices",
                                  pdtproperty,
                                  puiVal,
                                  uiPropVal) {
        if (iIndex == uiPropVal) {
            iIndex = iCount;
            break;
        }
        iCount++;
    }

    if (pdtproperty && !puiVal) {
        return  (LW_NULL);
    }

    if (API_DeviceTreePropertyStringIndexRead(pdtpaClkSpec.DTPH_pdtnDev,
                                              "clock-output-names",
                                              iIndex,
                                              &pcClkName) < 0) {
        pclk = API_ClockGetFromProvider(&pdtpaClkSpec);
        if (!pclk) {
            if (pdtpaClkSpec.DTPH_iArgsCount == 0) {
                pcClkName = pdtpaClkSpec.DTPH_pdtnDev->DTN_pcName;
            } else {
                pcClkName = LW_NULL;
            }
        } else {
            pcClkName = pclk->CLK_pcName;
        }
    }

    return  (pcClkName);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeClockGet
** 功能描述: 通过设备树节点获取时钟
** 输　入  : pdtnDev        设备树节点
**           iIndex         设备序号
** 输　出  : 获取的时钟
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_DeviceTreeClockGet (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex)
{
    LW_DEVTREE_PHANDLE_ARGS   pdtpaClkSpec;
    PLW_CLOCK                 pclk;
    INT                       iRet;

    iRet = API_DeviceTreePhandleParseWithArgs(pdtnDev,
                                              "clocks",
                                              "#clock-cells",
                                              iIndex,
                                              &pdtpaClkSpec);
    if (iRet) {
        return  (LW_NULL);
    }

    pclk = API_ClockGetFromProvider(&pdtpaClkSpec);

    return  (pclk);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
