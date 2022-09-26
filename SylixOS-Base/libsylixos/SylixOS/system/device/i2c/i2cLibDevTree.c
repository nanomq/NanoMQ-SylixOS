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
** 文   件   名: i2cLibDevTree.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2020 年 02 月 06 日
**
** 描        述: I2C 总线驱动框架库
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __I2C_BUS_LOCK(padpt)       API_SemaphoreBPend(padpt->DTI2CADPT_hBusLock, LW_OPTION_WAIT_INFINITE)
#define __I2C_BUS_UNLOCK(padpt)     API_SemaphoreBPost(padpt->DTI2CADPT_hBusLock)
/*********************************************************************************************************
** 函数名称: __i2cDevProbe
** 功能描述: I2C 设备 probe 函数
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __i2cDevProbe (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DT_I2C_DEVICE  pdti2cdevice = __i2cDevGet(pdevinstance);
    PLW_DT_I2C_DRIVER  pdti2cdriver = __i2cDrvGet(pdevinstance->DEVHD_pdrvinstance);
    INT                iRet;

    if (pdti2cdriver->I2CDRV_pfuncProbe) {
        iRet = pdti2cdriver->I2CDRV_pfuncProbe(pdti2cdevice);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __i2cDevRemove
** 功能描述: I2C 设备卸载
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __i2cDevRemove (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DT_I2C_DEVICE  pdti2cdevice = __i2cDevGet(pdevinstance);
    PLW_DT_I2C_DRIVER  pdti2cdriver = __i2cDrvGet(pdevinstance->DEVHD_pdrvinstance);

    if (pdti2cdriver->I2CDRV_pfuncRemove) {
        pdti2cdriver->I2CDRV_pfuncRemove(pdti2cdevice);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __i2cBusMatch
** 功能描述: I2C 总线上设备与驱动匹配判断
** 输　入  : pdevinstance    设备实例指针
**           pdrvinstance    驱动实例指针
** 输　出  : 匹配返回 0，不匹配返回其他
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __i2cBusMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    INT   iRet;

    iRet = API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance);       /*  通过设备树 compatible 匹配  */

    return  (iRet);
}
/*********************************************************************************************************
  I2C 总线全局变量
*********************************************************************************************************/
static LW_BUS_TYPE _G_bustypeI2c = {
    .BUS_pcName     = "i2c_bus",
    .BUS_pfuncMatch = __i2cBusMatch,
    .BUS_uiFlag     = BUS_AUTO_PROBE,
};
/*********************************************************************************************************
** 函数名称: API_I2cBusInit
** 功能描述: I2C 总线库初始化
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_I2cBusInit (VOID)
{
    INT  iRet;

    iRet = API_BusInit(&_G_bustypeI2c);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_I2cBusGet
** 功能描述: I2C 总线指针获取
** 输　入  : NONE
** 输　出  : I2C 总线指针
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_BUS_TYPE  API_I2cBusGet (VOID)
{
    return  (&_G_bustypeI2c);
}
/*********************************************************************************************************
** 函数名称: API_I2cAdapterRegister
** 功能描述: I2C 总线适配器(控制器)注册
** 输　入  : pdti2cadapter    I2C 控制器指针
**           pcName           I2C 控制器名称
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_I2cAdapterRegister (PLW_DT_I2C_ADAPTER  pdti2cadapter, CPCHAR  pcName)
{
    PLW_I2C_ADAPTER     pi2cadapter;
    
    if (!pdti2cadapter || !pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (LW_NULL == pdti2cadapter->DTI2CADPT_pi2cfuncs) {                /*  检查操作函数                */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "I2C: Failed to register "
                     "adapter %s with no funcs.\r\n", pcName);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  因为 I2C 控制器已作为平台设备注册，
     *  此处仅对控制器参数进行初始化
     */
    pdti2cadapter->DTI2CADPT_hBusLock = API_SemaphoreBCreate("i2c_buslock",
                                                             LW_TRUE,
                                                             LW_OPTION_WAIT_FIFO |
                                                             LW_OPTION_OBJECT_GLOBAL,
                                                             LW_NULL);
    if (LW_OBJECT_HANDLE_INVALID == pdti2cadapter->DTI2CADPT_hBusLock) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "I2C: Failed to create lock\r\n");
        return  (PX_ERROR);
    }

    if (0 == pdti2cadapter->DTI2CADPT_ulTimeout) {
        pdti2cadapter->DTI2CADPT_ulTimeout = LW_I2C_TIME_OUT_DEFAULT;
    }

    if (0 == pdti2cadapter->DTI2CADPT_iRetry) {
        pdti2cadapter->DTI2CADPT_iRetry = LW_I2C_RETRY_DEFAULT;
    }

    /*
     *  注册到以前的 bus 链表中
     */
    pi2cadapter = (PLW_I2C_ADAPTER)__SHEAP_ALLOC(sizeof(LW_I2C_ADAPTER));
    if (pi2cadapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    if (__busAdapterCreate(&pi2cadapter->I2CADAPTER_pbusadapter, pcName) != ERROR_NONE) {
        __SHEAP_FREE(pi2cadapter);
        return  (PX_ERROR);
    }

    pdti2cadapter->DTI2CADPT_pi2cadapter = pi2cadapter;
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_I2cAdapterUnregister
** 功能描述: I2C 总线适配器(控制器)卸载
** 输　入  : pdti2cadapter    I2C 控制器指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_I2cAdapterUnregister (PLW_DT_I2C_ADAPTER  pdti2cadapter)
{
    if (!pdti2cadapter) {
        _ErrorHandle(EINVAL);
        return;
    }

    if (pdti2cadapter->DTI2CADPT_hBusLock) {
        API_SemaphoreBDelete(&pdti2cadapter->DTI2CADPT_hBusLock);
    }

    if (pdti2cadapter->DTI2CADPT_pi2cadapter) {
        __busAdapterDelete(pdti2cadapter->DTI2CADPT_pi2cadapter->I2CADAPTER_pbusadapter.BUSADAPTER_cName);
        __SHEAP_FREE(pdti2cadapter->DTI2CADPT_pi2cadapter);
        pdti2cadapter->DTI2CADPT_pi2cadapter = LW_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: API_I2cDevRegister
** 功能描述: I2C 设备注册
** 输　入  : pdti2cdevice    I2C 设备指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_I2cDevRegister (PLW_DT_I2C_DEVICE  pdti2cdevice)
{
    PLW_DEV_INSTANCE    pdevinstance;
    PLW_DT_I2C_ADAPTER  pdti2cadapter;

    if (!pdti2cdevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdevinstance                 = &pdti2cdevice->DTI2CDEV_devinstance;
    pdevinstance->DEVHD_pbustype = &_G_bustypeI2c;                      /*  设置设备使用的总线类型      */

    if (API_DeviceRegister(pdevinstance) != ERROR_NONE) {               /*  注册设备                    */
        return  (PX_ERROR);
    }

    pdti2cadapter = pdti2cdevice->DTI2CDEV_pdti2cadapter;
    LW_BUS_INC_DEV_COUNT(&pdti2cadapter->DTI2CADPT_pi2cadapter->I2CADAPTER_pbusadapter);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_I2cDevDelete
** 功能描述: 删除 I2C 设备
** 输　入  : pdti2cdevice    I2C 设备指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_I2cDevDelete (PLW_DT_I2C_DEVICE  pdti2cdevice)
{
    PLW_DT_I2C_ADAPTER  pdti2cadapter;

    if (!pdti2cdevice) {
        _ErrorHandle(EINVAL);
        return;
    }

    pdti2cadapter = pdti2cdevice->DTI2CDEV_pdti2cadapter;

    LW_BUS_DEC_DEV_COUNT(&pdti2cadapter->DTI2CADPT_pi2cadapter->I2CADAPTER_pbusadapter);

    API_DeviceUnregister(&pdti2cdevice->DTI2CDEV_devinstance);

    __SHEAP_FREE(pdti2cdevice);
}
/*********************************************************************************************************
** 函数名称: API_I2cDrvRegister
** 功能描述: I2C 设备驱动注册
** 输　入  : pdti2cdriver    I2C 驱动指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_I2cDrvRegister (PLW_DT_I2C_DRIVER  pdti2cdriver)
{
    PLW_DRV_INSTANCE  pdrvinstance;
    INT               iRet;

    if (!pdti2cdriver) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdrvinstance = &pdti2cdriver->DTI2CDRV_drvinstance;
    pdrvinstance->DRVHD_pcName      = pdti2cdriver->DTI2CDRV_pcName;
    pdrvinstance->DRVHD_pbustype    = &_G_bustypeI2c;
    pdrvinstance->DRVHD_pfuncProbe  = __i2cDevProbe;
    pdrvinstance->DRVHD_pfuncRemove = __i2cDevRemove;

    iRet = API_DriverRegister(pdrvinstance);
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_I2cDrvUnregister
** 功能描述: I2C 设备驱动卸载
** 输　入  : pdti2cdriver    I2C 驱动指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_I2cDrvUnregister (PLW_DT_I2C_DRIVER  pdti2cdriver)
{
    if (!pdti2cdriver) {
        return;
    }

    API_DriverUnregister(&pdti2cdriver->DTI2CDRV_drvinstance);
}
/*********************************************************************************************************
** 函数名称: API_I2cBusTransfer
** 功能描述: I2C 总线传输函数
** 输　入  : pdti2cadapter  I2C 控制器指针
**           pdti2cmsg      I2C 传输消息控制块组
**           iNum           控制消息组中消息的数量
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_I2cBusTransfer (PLW_DT_I2C_ADAPTER   pdti2cadapter,
                         PLW_DT_I2C_MSG       pdti2cmsg,
                         INT                  iNum)
{
    INT  iRet = PX_ERROR;

    if (!pdti2cadapter || !pdti2cmsg || (iNum < 1)) {
        return  (PX_ERROR);
    }

    if (pdti2cadapter->DTI2CADPT_pi2cfuncs->DTI2CFUNC_pfuncMasterXfer) {
        __I2C_BUS_LOCK(pdti2cadapter);
        iRet = pdti2cadapter->DTI2CADPT_pi2cfuncs->DTI2CFUNC_pfuncMasterXfer(pdti2cadapter,
                                                                             pdti2cmsg,
                                                                             iNum);
        __I2C_BUS_UNLOCK(pdti2cadapter);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_I2cDevTransfer
** 功能描述: I2C 设备传输函数
** 输　入  : pdti2cdevice  I2C 设备指针
**           pdti2cmsg     I2C 传输消息控制块组
**           iNum          控制消息组中消息的数量
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_I2cDevTransfer (PLW_DT_I2C_DEVICE  pdti2cdevice, PLW_DT_I2C_MSG  pdti2cmsg, INT  iNum)
{
    PLW_DT_I2C_ADAPTER  pdti2cadapter;
    INT                 iRet;

    if (!pdti2cdevice || !pdti2cmsg || (iNum < 1)) {
        return  (PX_ERROR);
    }

    pdti2cadapter = pdti2cdevice->DTI2CDEV_pdti2cadapter;
    iRet = API_I2cBusTransfer(pdti2cadapter, pdti2cmsg, iNum);

    return  (iRet);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
