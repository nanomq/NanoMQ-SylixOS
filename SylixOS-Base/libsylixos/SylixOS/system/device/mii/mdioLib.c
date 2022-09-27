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
** 文   件   名: mdioLib.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 20 日
**
** 描        述: mdio 总线驱动框架库
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __MDIO_BUS_UNLOCK(pAdpt)    API_SemaphoreMPost(pAdpt->MDIOADPT_hBusLock)
#define __MDIO_BUS_LOCK(pAdpt)      API_SemaphoreMPend(pAdpt->MDIOADPT_hBusLock, \
                                                       LW_OPTION_WAIT_INFINITE)
/*********************************************************************************************************
** 函数名称: __mdioDevProbe
** 功能描述: mdio 设备 probe 函数
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __mdioDevProbe (PLW_DEV_INSTANCE  pdevinstance)
{
    PMDIO_DEVICE  pmdiodev = __mdioDevGet(pdevinstance);
    PMDIO_DRIVER  pmdiodrv = __mdioDrvGet(pdevinstance->DEVHD_pdrvinstance);
    INT           iRet;

    if (pmdiodrv->MDIODRV_pfuncProbe) {
        iRet = pmdiodrv->MDIODRV_pfuncProbe(pmdiodev);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __mdioDevRemove
** 功能描述: mdio 设备卸载
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __mdioDevRemove (PLW_DEV_INSTANCE  pdevinstance)
{
    PMDIO_DEVICE  pmdiodev = __mdioDevGet(pdevinstance);
    PMDIO_DRIVER  pmdiodrv = __mdioDrvGet(pdevinstance->DEVHD_pdrvinstance);

    if (pmdiodrv->MDIODRV_pfuncRemove) {
        pmdiodrv->MDIODRV_pfuncRemove(pmdiodev);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __mdioBusMatch
** 功能描述: mdio 总线上设备与驱动匹配判断
** 输　入  : pdevinstance    设备实例指针
**           pdrvinstance    驱动实例指针
** 输　出  : 匹配返回 0，不匹配返回其他
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __mdioBusMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    PMDIO_DEVICE  pmdiodev = __mdioDevGet(pdevinstance);
    INT           iRet;

    iRet = API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance);       /*  通过设备树 compatibale 匹配 */
    if (!iRet) {
        return  (iRet);
    }

    if (pmdiodev->MDIODEV_pfuncBusMatch) {                              /*  通过挂接的匹配函数匹配      */
        return  (pmdiodev->MDIODEV_pfuncBusMatch(pdevinstance, pdrvinstance));
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
  MDIO 总线全局变量
*********************************************************************************************************/
static LW_BUS_TYPE _G_bustypeMdio = {
    .BUS_pcName     = "mdio_bus",
    .BUS_pfuncMatch = __mdioBusMatch,
    .BUS_uiFlag     = BUS_AUTO_PROBE,
};
/*********************************************************************************************************
** 函数名称: API_MdioBusInit
** 功能描述: MDIO 总线初始化
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioBusInit (VOID)
{
    return  (API_BusInit(&_G_bustypeMdio));
}
/*********************************************************************************************************
** 函数名称: API_MdioBusGet
** 功能描述: MDIO 总线指针获取
** 输　入  : NONE
** 输　出  : MDIO 总线指针
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_BUS_TYPE  API_MdioBusGet (VOID)
{
    return  (&_G_bustypeMdio);
}
/*********************************************************************************************************
** 函数名称: API_MdioAdapterRegister
** 功能描述: MDIO 总线适配器(控制器)注册
** 输　入  : pmdioadapter    MDIO 控制器指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioAdapterRegister (PMDIO_ADAPTER  pmdioadapter)
{
    if (!pmdioadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmdioadapter->MDIOADPT_hBusLock = API_SemaphoreMCreate("mdiobus_lock", LW_PRIO_DEF_CEILING,
                                                           LW_OPTION_WAIT_PRIORITY |
                                                           LW_OPTION_INHERIT_PRIORITY |
                                                           LW_OPTION_DELETE_SAFE |
                                                           LW_OPTION_OBJECT_GLOBAL, LW_NULL);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_MdioDrvRegister
** 功能描述: MDIO 驱动注册
** 输　入  : pmdiodrv    MDIO 驱动指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioDrvRegister (PMDIO_DRIVER  pmdiodrv)
{
    PLW_DRV_INSTANCE  pdrvinstance;

    if (!pmdiodrv) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  对通用驱动实例进行赋值
     */
    pdrvinstance = &pmdiodrv->MDIODRV_drvinstance;
    pdrvinstance->DRVHD_pcName      = pmdiodrv->MDIODRV_pcName;
    pdrvinstance->DRVHD_pbustype    = &_G_bustypeMdio;
    pdrvinstance->DRVHD_pfuncProbe  = __mdioDevProbe;
    pdrvinstance->DRVHD_pfuncRemove = __mdioDevRemove;

    return  (API_DriverRegister(pdrvinstance));                         /*  注册通用驱动                */
}
/*********************************************************************************************************
** 函数名称: API_MdioDevCreate
** 功能描述: MDIO 设备创建
** 输　入  : pmdioadapter   MDIO 适配器指针
**           uiAddr         MDIO 设备地址
**           uiFlag         MDIO 设备类型标志
** 输　出  : 成功返回创建的 MDIO 设备指针, 失败返回 LW_NULL
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
PMDIO_DEVICE  API_MdioDevCreate (PMDIO_ADAPTER  pmdioadapter,
                                 UINT           uiAddr,
                                 UINT           uiFlag)
{
    PMDIO_DEVICE  pmdiodev;

    pmdiodev = __SHEAP_ZALLOC(sizeof(MDIO_DEVICE));                     /*  MDIO 设备分配内存           */
    if (LW_NULL == pmdiodev) {
        return  (LW_NULL);
    }

    /*
     *  初始化 MDIO 设备
     */
    pmdiodev->MDIODEV_uiFlag        = uiFlag;
    pmdiodev->MDIODEV_pmdioadapter  = pmdioadapter;
    pmdiodev->MDIODEV_uiAddr        = uiAddr;
    snprintf(pmdiodev->MDIODEV_cName, MDIO_NAME_SIZE, "%s:%02x",
             pmdioadapter->MDIOADPT_pcName, uiAddr);
    
    pmdiodev->MDIODEV_devinstance.DEVHD_pbustype = &_G_bustypeMdio;

    if (uiFlag & MDIO_DEVICE_IS_PHY) {
        pmdiodev->MDIODEV_pvPriv = API_PhyDevCreate(pmdiodev);          /*  创建 phy 设备               */
    }

    return  (pmdiodev);
}
/*********************************************************************************************************
** 函数名称: API_MdioDevDelete
** 功能描述: 删除 MDIO 设备
** 输　入  : pmdiodev    MDIO 设备指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_MdioDevDelete (PMDIO_DEVICE  pmdiodev)
{
    if (pmdiodev) {
        __SHEAP_FREE(pmdiodev);
    }
}
/*********************************************************************************************************
** 函数名称: API_MdioDevRegister
** 功能描述: MDIO 设备注册
** 输　入  : pmdiodev    MDIO 设备指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioDevRegister (PMDIO_DEVICE  pmdiodev)
{
    PMDIO_ADAPTER     pmdioadpter;
    PLW_DEV_INSTANCE  pdevinstance;
    UINT              uiDevAddr;

    if (!pmdiodev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmdioadpter  =  pmdiodev->MDIODEV_pmdioadapter;
    pdevinstance = &pmdiodev->MDIODEV_devinstance;
    uiDevAddr    =  pmdiodev->MDIODEV_uiAddr;
    pmdioadpter->MDIOADPT_DevMap[uiDevAddr] = pmdiodev;                 /*  放入设备表中，用地址索引    */

    return  (API_DeviceRegister(pdevinstance));                         /*  注册设备                    */
}
/*********************************************************************************************************
** 函数名称: API_MdioBusRead
** 功能描述: MDIO 总线读函数
** 输　入  : pmdioadapter   MDIO 控制器指针
**           uiAddr         MDIO 设备地址
**           uiRegNum       MDIO 设备寄存器地址
** 输　出  : 返回读取的数据
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioBusRead (PMDIO_ADAPTER  pmdioadapter, UINT  uiAddr, UINT  uiRegNum)
{
    INT  iValue;

    if (!pmdioadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __MDIO_BUS_LOCK(pmdioadapter);
    iValue = pmdioadapter->MDIOADPT_pfuncRead(pmdioadapter, uiAddr, uiRegNum);
    __MDIO_BUS_UNLOCK(pmdioadapter);
    
    return  (iValue);
}
/*********************************************************************************************************
** 函数名称: API_MdioBusWrite
** 功能描述: MDIO 总线写函数
** 输　入  : pmdioadapter   MDIO 控制器指针
**           uiAddr         MDIO 设备地址
**           uiRegNum       MDIO 设备寄存器地址
**           usValue        要写入的数据
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioBusWrite (PMDIO_ADAPTER    pmdioadapter,
                       UINT             uiAddr,
                       UINT             uiRegNum,
                       UINT16           usValue)
{
    INT  iRet;

    if (!pmdioadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __MDIO_BUS_LOCK(pmdioadapter);
    iRet = pmdioadapter->MDIOADPT_pfuncWrite(pmdioadapter, uiAddr, uiRegNum, usValue);
    __MDIO_BUS_UNLOCK(pmdioadapter);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_MdioBusReset
** 功能描述: MDIO 总线复位函数
** 输　入  : pmdioadapter    MDIO 控制器指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioBusReset (PMDIO_ADAPTER  pmdioadapter)
{
    INT  iRet;

    if (!pmdioadapter) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __MDIO_BUS_LOCK(pmdioadapter);
    iRet = pmdioadapter->MDIOADPT_pfuncReset(pmdioadapter);
    __MDIO_BUS_UNLOCK(pmdioadapter);
    
    return  (iRet);
}   
/*********************************************************************************************************
** 函数名称: API_MdioDevRead
** 功能描述: MDIO 设备读函数
** 输　入  : pmdiodev    MDIO 设备指针
**           uiRegNum    要读取的寄存器地址
** 输　出  : 返回读取的数据
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioDevRead (PMDIO_DEVICE  pmdiodev, UINT  uiRegNum)
{
    PMDIO_ADAPTER pmdioadapter = pmdiodev->MDIODEV_pmdioadapter;
    UINT          uiAddr       = pmdiodev->MDIODEV_uiAddr;

    if (!pmdiodev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmdioadapter = pmdiodev->MDIODEV_pmdioadapter;
    uiAddr       = pmdiodev->MDIODEV_uiAddr;

    return  (API_MdioBusRead(pmdioadapter, uiAddr, uiRegNum));
}
/*********************************************************************************************************
** 函数名称: API_MdioDevWrite
** 功能描述: MDIO设备写函数
** 输　入  : pmdiodev    MDIO 设备指针
**           uiRegNum    要写入的寄存器地址
**           usValue     要写入的数据
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_MdioDevWrite (PMDIO_DEVICE  pmdiodev, UINT  uiRegNum, UINT16  usValue)
{    
    PMDIO_ADAPTER pmdioadapter;
    UINT          uiAddr;

    if (!pmdiodev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pmdioadapter = pmdiodev->MDIODEV_pmdioadapter;
    uiAddr       = pmdiodev->MDIODEV_uiAddr;
    
    return  (API_MdioBusWrite(pmdioadapter, uiAddr, uiRegNum, usValue));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
