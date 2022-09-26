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
** 文   件   名: platform.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 10 日
**
** 描        述: 平台设备处理库
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
** 函数名称: __platformMatch
** 功能描述: 平台总线上设备与驱动匹配接口
** 输　入  : pdevinstance      设备指针
**           pdrvinstance      驱动指针
** 输　出  : 匹配返回 0，不匹配返回其他
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __platformMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    return  (API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance));
}
/*********************************************************************************************************
** 函数名称: __platformProbe
** 功能描述: 平台总线 probe 接口
** 输　入  : pdevinstance      设备指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __platformProbe (PLW_DEV_INSTANCE  pdevinstance)
{
    /*
     *  在 platform 总线上的设备 probe 时可先进行电源、时钟等的设置
     *  然后再进行具体设备驱动的 probe
     */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  平台总线全局变量  
*********************************************************************************************************/
static LW_BUS_TYPE  _G_bustypePlatform = {
    .BUS_pcName     = "platform",
    .BUS_pfuncMatch = __platformMatch,
    .BUS_pfuncProbe = __platformProbe,
    .BUS_uiFlag     = BUS_AUTO_PROBE | BUS_FORCE_DRV_PROBE,
};
/*********************************************************************************************************
** 函数名称: API_PlatformBusInit
** 功能描述: 平台总线初始化
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PlatformBusInit (VOID)
{
    return  (API_BusInit(&_G_bustypePlatform));
}
/*********************************************************************************************************
** 函数名称: API_PlatformDeviceRegister
** 功能描述: 平台设备注册
** 输　入  : pdevinstance    平台设备指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PlatformDeviceRegister (PLW_DEV_INSTANCE  pdevinstance)
{
    if (!pdevinstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdevinstance->DEVHD_pbustype = &_G_bustypePlatform;
    
    return  (API_DeviceRegister(pdevinstance));
}
/*********************************************************************************************************
** 函数名称: API_PlatformDeviceUnregister
** 功能描述: 平台设备卸载
** 输　入  : pdevinstance    平台设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PlatformDeviceUnregister (PLW_DEV_INSTANCE  pdevinstance)
{
    if (!pdevinstance) {
        _ErrorHandle(EINVAL);
        return;
    }

    API_DeviceUnregister(pdevinstance);
}
/*********************************************************************************************************
** 函数名称: API_PlatformDriverRegister
** 功能描述: 平台设备驱动注册
** 输　入  : pdrvinstance    驱动指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PlatformDriverRegister (PLW_DRV_INSTANCE  pdrvinstance)
{
    if (!pdrvinstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdrvinstance->DRVHD_pbustype = &_G_bustypePlatform;

    return  (API_DriverRegister(pdrvinstance));
}
/*********************************************************************************************************
** 函数名称: API_PlatformDriverUnregister
** 功能描述: 平台设备驱动卸载
** 输　入  : pdrvinstance    驱动指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PlatformDriverUnregister (PLW_DRV_INSTANCE  pdrvinstance)
{
    if (!pdrvinstance) {
        _ErrorHandle(EINVAL);
        return;
    }

    API_DriverUnregister(pdrvinstance);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
