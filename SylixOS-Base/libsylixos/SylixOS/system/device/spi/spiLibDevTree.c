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
** 文   件   名: spiLibDevTree.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2020 年 02 月 06 日
**
** 描        述: SPI 总线驱动框架库
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
#define __SPI_BUS_LOCK(padpt)       API_SemaphoreBPend(padpt->DTSPICTRL_hBusLock, LW_OPTION_WAIT_INFINITE)
#define __SPI_BUS_UNLOCK(padpt)     API_SemaphoreBPost(padpt->DTSPICTRL_hBusLock)
/*********************************************************************************************************
** 函数名称: __spiDevProbe
** 功能描述: SPI 设备 probe 函数
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spiDevProbe (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DT_SPI_DEVICE  pdtspidevice = __spiDevGet(pdevinstance);
    PLW_DT_SPI_DRIVER  pdtspidriver = __spiDrvGet(pdevinstance->DEVHD_pdrvinstance);
    INT                iRet;

    if (pdtspidriver->DTSPIDRV_pfuncProbe) {
        iRet = pdtspidriver->DTSPIDRV_pfuncProbe(pdtspidevice);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __spiDevRemove
** 功能描述: SPI 设备卸载
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spiDevRemove (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DT_SPI_DEVICE  pdtspidevice = __spiDevGet(pdevinstance);
    PLW_DT_SPI_DRIVER  pdtspidriver = __spiDrvGet(pdevinstance->DEVHD_pdrvinstance);

    if (pdtspidriver->DTSPIDRV_pfuncRemove) {
        pdtspidriver->DTSPIDRV_pfuncRemove(pdtspidevice);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spiBusMatch
** 功能描述: SPI 总线上设备与驱动匹配判断
** 输　入  : pdevinstance    设备实例指针
**           pdrvinstance    驱动实例指针
** 输　出  : 匹配返回 0，不匹配返回其他
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spiBusMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DT_SPI_DEVICE  pdtspidevice = __spiDevGet(pdevinstance);
    INT                iRet;

    iRet = API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance);       /*  通过设备树 compatible 匹配  */
    if (!iRet) {
        return  (iRet);
    }

    return  (lib_strcmp(pdtspidevice->DTSPIDEV_cName,
                        pdrvinstance->DRVHD_pcName));                   /*  通过设备和驱动名称匹配      */
}
/*********************************************************************************************************
** 函数名称: __spiGpioNumbersGet
** 功能描述: 获取 SPI 设备树节点的片选 GPIO 号。
** 输　入  : pdtspictrl      SPI 控制器指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spiGpioNumbersGet (PLW_DT_SPI_CTRL  pdtspictrl)
{
    PLW_DEVTREE_NODE        pdtnDev;
    INT                     iCount;
    INT                     i;

    if (!pdtspictrl || !pdtspictrl->DTSPICTRL_pdevinstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtnDev = pdtspictrl->DTSPICTRL_pdevinstance->DEVHD_pdtnDev;

    iCount = API_DeviceTreeGpioNamedCountGet(pdtnDev, "cs-gpios");
    if (iCount <= 0) {
        pdtspictrl->DTSPICTRL_usChipSelNums = 0;
        return  (ERROR_NONE);                                           /*  不使用 GPIO 作为片选信号    */
    }
    pdtspictrl->DTSPICTRL_usChipSelNums   = iCount;

    pdtspictrl->DTSPICTRL_puiChipSelGpios = (UINT32 *)__SHEAP_ZALLOC(
            pdtspictrl->DTSPICTRL_usChipSelNums * sizeof(UINT));
    if (!pdtspictrl->DTSPICTRL_puiChipSelGpios) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    for (i = 0; i < iCount; i++) {
        pdtspictrl->DTSPICTRL_puiChipSelGpios[i] =
                API_DeviceTreeGpioNamedGpioGet(pdtnDev, "cs-gpios", i);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  SPI 总线全局变量
*********************************************************************************************************/
static LW_BUS_TYPE  _G_bustypeSpi = {
    .BUS_pcName     = "spi_bus",
    .BUS_pfuncMatch = __spiBusMatch,
    .BUS_uiFlag     = BUS_AUTO_PROBE,
};
/*********************************************************************************************************
** 函数名称: API_SpiBusInit
** 功能描述: SPI 总线库初始化
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiBusInit (VOID)
{
    INT  iRet;

    iRet = API_BusInit(&_G_bustypeSpi);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiBusGet
** 功能描述: SPI 总线指针获取
** 输　入  : NONE
** 输　出  : SPI 总线指针
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_BUS_TYPE  API_SpiBusGet (VOID)
{
    return  (&_G_bustypeSpi);
}

/*********************************************************************************************************
** 函数名称: API_SpiCtrlRegister
** 功能描述: SPI 总线控制器注册
** 输　入  : pdtspictrl      SPI 控制器指针
**           pcName          SPI 控制器名称
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiCtrlRegister (PLW_DT_SPI_CTRL  pdtspictrl, CPCHAR  pcName)
{
    PLW_SPI_ADAPTER     pspiadapter;

    if (!pdtspictrl || !pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (LW_NULL == pdtspictrl->DTSPICTRL_pfuncXferOne) {                /*  检查传输接口                */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI transfer function is empty.\r\n");
        return  (PX_ERROR);
    }

    /*
     *  因为 SPI 控制器已作为平台设备注册，
     *  此处仅对控制器参数进行初始化,
     */
    pdtspictrl->DTSPICTRL_hBusLock = API_SemaphoreBCreate("spi_buslock",
                                                          LW_TRUE,
                                                          LW_OPTION_WAIT_FIFO |
                                                          LW_OPTION_OBJECT_GLOBAL,
                                                          LW_NULL);
    /*
     *  注册到以前的 bus 链表中
     */
    pspiadapter = (PLW_SPI_ADAPTER)__SHEAP_ALLOC(sizeof(LW_SPI_ADAPTER));
    if (pspiadapter == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    if (__busAdapterCreate(&pspiadapter->SPIADAPTER_pbusadapter, pcName) != ERROR_NONE) {
        __SHEAP_FREE(pspiadapter);
        return  (PX_ERROR);
    }

    if (__spiGpioNumbersGet(pdtspictrl) != ERROR_NONE) {
        __SHEAP_FREE(pspiadapter);
        return  (PX_ERROR);
    }

    pdtspictrl->DTSPICTRL_pspiadapter = pspiadapter;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiCtrlUnregister
** 功能描述: SPI 总线控制器卸载
** 输　入  : pdtspictrl      SPI 控制器指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_SpiCtrlUnregister (PLW_DT_SPI_CTRL  pdtspictrl)
{
    if (!pdtspictrl) {
        return;
    }

    if (pdtspictrl->DTSPICTRL_hBusLock) {
        API_SemaphoreBDelete(&pdtspictrl->DTSPICTRL_hBusLock);
    }

    if (pdtspictrl->DTSPICTRL_pspiadapter) {
        __busAdapterDelete(pdtspictrl->DTSPICTRL_pspiadapter->SPIADAPTER_pbusadapter.BUSADAPTER_cName);
        __SHEAP_FREE(pdtspictrl->DTSPICTRL_pspiadapter);
        pdtspictrl->DTSPICTRL_pspiadapter = LW_NULL;
    }

    if (pdtspictrl->DTSPICTRL_usChipSelNums > 0) {
        __SHEAP_FREE(pdtspictrl->DTSPICTRL_puiChipSelGpios);
    }
}
/*********************************************************************************************************
** 函数名称: API_SpiDevRegister
** 功能描述: SPI 设备注册
** 输　入  : pdtspidevice     SPI 设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiDevRegister (PLW_DT_SPI_DEVICE  pdtspidevice)
{
    PLW_DEV_INSTANCE  pdevinstance;
    PLW_DT_SPI_CTRL   pdtspictrl;

    if (!pdtspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdevinstance                 = &pdtspidevice->DTSPIDEV_devinstance;
    pdevinstance->DEVHD_pbustype = &_G_bustypeSpi;                      /*  设置设备使用的总线类型      */

    if (API_DeviceRegister(pdevinstance) != ERROR_NONE) {               /*  注册设备                    */
        return  (PX_ERROR);
    }

    pdtspictrl = pdtspidevice->DTSPIDEV_pdtspictrl;
    LW_BUS_INC_DEV_COUNT(&pdtspictrl->DTSPICTRL_pspiadapter->SPIADAPTER_pbusadapter);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpiDevDelete
** 功能描述: 删除 SPI 设备
** 输　入  : pdtspidevice    SPI 设备指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_SpiDevDelete (PLW_DT_SPI_DEVICE  pdtspidevice)
{
    PLW_DT_SPI_CTRL  pdtspictrl;

    if (!pdtspidevice) {
        _ErrorHandle(EINVAL);
        return;
    }

    pdtspictrl = pdtspidevice->DTSPIDEV_pdtspictrl;

    LW_BUS_DEC_DEV_COUNT(&pdtspictrl->DTSPICTRL_pspiadapter->SPIADAPTER_pbusadapter);

    API_DeviceUnregister(&pdtspidevice->DTSPIDEV_devinstance);

    __SHEAP_FREE(pdtspidevice);
}
/*********************************************************************************************************
** 函数名称: API_SpiDrvRegister
** 功能描述: SPI 设备驱动注册
** 输　入  : pdtspidriver    SPI 驱动指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiDrvRegister (PLW_DT_SPI_DRIVER  pdtspidriver)
{
    PLW_DRV_INSTANCE  pdrvinstance;
    INT               iRet;

    if (!pdtspidriver) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdrvinstance                    = &pdtspidriver->DTSPIDRV_drvinstance;
    pdrvinstance->DRVHD_pcName      = pdtspidriver->DTSPIDRV_pcName;
    pdrvinstance->DRVHD_pbustype    = &_G_bustypeSpi;
    pdrvinstance->DRVHD_pfuncProbe  = __spiDevProbe;
    pdrvinstance->DRVHD_pfuncRemove = __spiDevRemove;

    iRet = API_DriverRegister(pdrvinstance);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiDrvUnregister
** 功能描述: SPI 设备驱动卸载
** 输　入  : pdtspidriver    SPI 驱动指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_SpiDrvUnregister (PLW_DT_SPI_DRIVER  pdtspidriver)
{
    if (!pdtspidriver) {
        return;
    }

    API_DriverUnregister(&pdtspidriver->DTSPIDRV_drvinstance);
}
/*********************************************************************************************************
** 函数名称: API_SpiDevSetup
** 功能描述: SPI 设备创建前的必要操作，主要是检查设备与控制器能够匹配工作，
**           也可以通过控制器的回调函数，在控制器上做些必要的设置或检查，
**           一般是在 SPI 设备的 probe 函数中调用
** 输　入  : pdtspidevice    SPI 设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiDevSetup (PLW_DT_SPI_DEVICE  pdtspidevice)
{
    UINT16  usBadBits;
    UINT16  usUglyBits;
    UINT16  usMode;
    INT     iRet   = ERROR_NONE;

    if (!pdtspidevice) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    usMode = pdtspidevice->DTSPIDEV_uiMode;

    /*
     *  不能同时选择 DUAL 模式 和 QUAD 模式
     */
    if (((usMode & LW_SPI_TX_DUAL) && (usMode & LW_SPI_TX_QUAD)) ||
        ((usMode & LW_SPI_RX_DUAL) && (usMode & LW_SPI_RX_QUAD))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI: can not select "
                     "dual and quad at the same time\r\n");
        return  (-EINVAL);
    }

    /*
     *  SPI 的三线模式不能使用 DUAL 和 QUAD 模式
     */
    if ((usMode & LW_SPI_3WIRE) && (usMode &
        (LW_SPI_TX_DUAL | LW_SPI_TX_QUAD |
         LW_SPI_RX_DUAL | LW_SPI_RX_QUAD))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI: can not select "
                     "dual and quad with 3 wire mode.\r\n");
        return  (-EINVAL);
    }

    /*
     *  处理模式中无效的标志
     */
    usBadBits  = usMode & (~pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_uiMode);
    usUglyBits = usBadBits &
                 (LW_SPI_TX_DUAL | LW_SPI_TX_QUAD |
                  LW_SPI_RX_DUAL | LW_SPI_RX_QUAD);

    if (usUglyBits) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI: ignoring unsupported mode bits %x\r\n",
                     usUglyBits);
        usMode      &= ~usUglyBits;
        usBadBits   &= ~usUglyBits;
        pdtspidevice->DTSPIDEV_uiMode = usMode;
    }

    if (usBadBits) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI: unsupported mode bits %x\r\n",
                     usBadBits);
        return  (-EINVAL);
    }

    if (!pdtspidevice->DTSPIDEV_ucBitsPerWord) {
        pdtspidevice->DTSPIDEV_ucBitsPerWord = 8;
    }

    if (!pdtspidevice->DTSPIDEV_uiSpeedMax) {
        pdtspidevice->DTSPIDEV_uiSpeedMax = pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_uiSpeedMax;
    }

    /*
     *  以上主要是检查 SPI 设备的参数是否与控制器匹配，
     *  做完上述检查后，调用控制器注册的 setup 回调函数
     */
    if (pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetup) {
        iRet = pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetup(pdtspidevice);
    }

    /*
     *  默认取消此 SPI 设备的使能
     */
    if (pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetCs) {
        pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetCs(pdtspidevice, LW_FALSE);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiDevTransfer
** 功能描述: SPI 设备传输函数
** 输　入  : pdtspidevice    SPI 设备指针
**           pspixfer      SPI 传输消息控制块组
**           iNum          控制消息组中消息的数量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiDevTransfer (PLW_DT_SPI_DEVICE  pdtspidevice,
                         PLW_DT_SPI_XFER    pspixfer,
                         INT                iNum)
{
    PLW_DT_SPI_CTRL  pdtspictrl;
    INT              iRet;
    INT              i;

    if (!pdtspidevice || !pspixfer || (iNum < 1)) {
        return  (PX_ERROR);
    }

    pdtspictrl = pdtspidevice->DTSPIDEV_pdtspictrl;

    __SPI_BUS_LOCK(pdtspictrl);                                         /*  锁定 SPI 总线               */

    if (pdtspictrl->DTSPICTRL_pfuncPrepareXfer) {
        iRet = pdtspictrl->DTSPICTRL_pfuncPrepareXfer(pdtspidevice);    /*  传输前准备                  */
        if (iRet) {
            goto  __xfer_ret;
        }
    }

    if (pdtspictrl->DTSPICTRL_pfuncSetCs) {
        pdtspictrl->DTSPICTRL_pfuncSetCs(pdtspidevice, LW_TRUE);        /*  使能 CS 选通                */
    }

    for (i = 0; i < iNum; i++) {
        iRet = pdtspictrl->DTSPICTRL_pfuncXferOne(pdtspidevice,
                                                pspixfer + i);          /*  进行传输                    */
        if (iRet) {
            break;
        }
    }

    if (pdtspictrl->DTSPICTRL_pfuncSetCs) {
        pdtspictrl->DTSPICTRL_pfuncSetCs(pdtspidevice, LW_FALSE);       /*  取消 CS 选通                */
    }

    if (pdtspictrl->DTSPICTRL_pfuncUnprepareXfer) {
        pdtspictrl->DTSPICTRL_pfuncUnprepareXfer(pdtspidevice);         /*  取消传输准备                */
    }

__xfer_ret:
    __SPI_BUS_UNLOCK(pdtspictrl);                                       /*  解锁 SPI 总线               */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiWrite
** 功能描述: SPI 设备传输函数
** 输　入  : pdtspidevice    SPI 设备指针
**           pvBuf         要写入的数据指针
**           iLen          要写入的数据长度
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiWrite (PLW_DT_SPI_DEVICE  pdtspidevice, PVOID  pvBuf, INT  iLen)
{
    LW_DT_SPI_XFER  spixfer;
    INT             iRet;

    if ((LW_NULL == pdtspidevice) ||
        (LW_NULL == pvBuf)        ||
        (0 <= iLen)) {
        return  (PX_ERROR);
    }

    lib_bzero(&spixfer, sizeof(spixfer));
    spixfer.DTSPIXFER_pvTxBuf = pvBuf;
    spixfer.DTSPIXFER_uiLen   = iLen;

    iRet = API_SpiDevTransfer(pdtspidevice, &spixfer, 1);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiRead
** 功能描述: SPI 设备读函数
** 输　入  : pdtspidevice  SPI 设备指针
**           pvBuf         要读出的数据指针
**           iLen          要读出的数据长度
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiRead  (PLW_DT_SPI_DEVICE  pdtspidevice, PVOID  pvBuf, INT  iLen)
{
    LW_DT_SPI_XFER  spixfer;
    INT             iRet;

    if ((LW_NULL == pdtspidevice) ||
        (LW_NULL == pvBuf)        ||
        (0 <= iLen))  {
        return  (PX_ERROR);
    }

    lib_bzero(&spixfer, sizeof(spixfer));
    spixfer.DTSPIXFER_pvRxBuf = pvBuf;
    spixfer.DTSPIXFER_uiLen   = iLen;

    iRet = API_SpiDevTransfer(pdtspidevice, &spixfer, 1);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiWriteThenRead
** 功能描述: SPI 设备先写后读接口
** 输　入  : pdtspidevice   SPI 设备指针
**           pvTxBuf        要写入的数据指针
**           iTxLen         要写入的数据长度
**           pvRxBuf        要读出的数据指针
**           iRxLen         要读出的数据长度
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_SpiWriteThenRead (PLW_DT_SPI_DEVICE  pdtspidevice,
                           PVOID              pvTxBuf,
                           INT                iTxLen,
                           PVOID              pvRxBuf,
                           INT                iRxLen)
{
    LW_DT_SPI_XFER  spixfer[2];
    INT             iRet;

    if (LW_NULL == pdtspidevice) {
        return  (PX_ERROR);
    }

    lib_bzero(spixfer, sizeof(spixfer));
    spixfer[0].DTSPIXFER_pvTxBuf = pvTxBuf;
    spixfer[0].DTSPIXFER_pvRxBuf = LW_NULL;
    spixfer[0].DTSPIXFER_uiLen   = iTxLen;

    spixfer[1].DTSPIXFER_pvTxBuf = LW_NULL;
    spixfer[1].DTSPIXFER_pvRxBuf = pvRxBuf;
    spixfer[1].DTSPIXFER_uiLen   = iRxLen;

    iRet = API_SpiDevTransfer(pdtspidevice, spixfer, 2);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_SpiW8R8
** 功能描述: 进行 SPI 通道写 8 bit，再读 8 bit 操作函数
** 输　入  : pdtspidevice  SPI 设备
**           ucCmd         读之前传入的参数
** 输　出  : 如果返回的是负值，则出错；
**           如果返回的是非负值，则为返回的数据
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
ssize_t  API_SpiW8R8 (PLW_DT_SPI_DEVICE  pdtspidevice, UINT8  ucCmd)
{
    ssize_t  stStatus;
    UINT8    ucResult;

    stStatus = API_SpiWriteThenRead(pdtspidevice, &ucCmd, 1, &ucResult, 1);

    return  ((stStatus < 0) ? stStatus : ucResult);
}
/*********************************************************************************************************
** 函数名称: API_SpiW8R16
** 功能描述: 进行 SPI 通道写 8 bit，再读 16 bit 操作函数
** 输　入  : pdtspidevice  SPI 设备
**           ucCmd         读之前传入的参数
** 输　出  : 如果返回的是负值，则出错；
**           如果返回的是非负值，则为返回的数据
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
ssize_t  API_SpiW8R16 (PLW_DT_SPI_DEVICE  pdtspidevice, UINT8  ucCmd)
{
    ssize_t  stStatus;
    UINT16   usResult;

    stStatus = API_SpiWriteThenRead(pdtspidevice, &ucCmd, 1, (UINT8 *)&usResult, 2);

    return  ((stStatus < 0) ? stStatus : usResult);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
