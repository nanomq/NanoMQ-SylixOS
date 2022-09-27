/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: spiLibDevTree.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2020 �� 02 �� 06 ��
**
** ��        ��: SPI ����������ܿ�
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __SPI_BUS_LOCK(padpt)       API_SemaphoreBPend(padpt->DTSPICTRL_hBusLock, LW_OPTION_WAIT_INFINITE)
#define __SPI_BUS_UNLOCK(padpt)     API_SemaphoreBPost(padpt->DTSPICTRL_hBusLock)
/*********************************************************************************************************
** ��������: __spiDevProbe
** ��������: SPI �豸 probe ����
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __spiDevRemove
** ��������: SPI �豸ж��
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __spiBusMatch
** ��������: SPI �������豸������ƥ���ж�
** �䡡��  : pdevinstance    �豸ʵ��ָ��
**           pdrvinstance    ����ʵ��ָ��
** �䡡��  : ƥ�䷵�� 0����ƥ�䷵������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spiBusMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DT_SPI_DEVICE  pdtspidevice = __spiDevGet(pdevinstance);
    INT                iRet;

    iRet = API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance);       /*  ͨ���豸�� compatible ƥ��  */
    if (!iRet) {
        return  (iRet);
    }

    return  (lib_strcmp(pdtspidevice->DTSPIDEV_cName,
                        pdrvinstance->DRVHD_pcName));                   /*  ͨ���豸����������ƥ��      */
}
/*********************************************************************************************************
** ��������: __spiGpioNumbersGet
** ��������: ��ȡ SPI �豸���ڵ��Ƭѡ GPIO �š�
** �䡡��  : pdtspictrl      SPI ������ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
        return  (ERROR_NONE);                                           /*  ��ʹ�� GPIO ��ΪƬѡ�ź�    */
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
  SPI ����ȫ�ֱ���
*********************************************************************************************************/
static LW_BUS_TYPE  _G_bustypeSpi = {
    .BUS_pcName     = "spi_bus",
    .BUS_pfuncMatch = __spiBusMatch,
    .BUS_uiFlag     = BUS_AUTO_PROBE,
};
/*********************************************************************************************************
** ��������: API_SpiBusInit
** ��������: SPI ���߿��ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_SpiBusInit (VOID)
{
    INT  iRet;

    iRet = API_BusInit(&_G_bustypeSpi);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SpiBusGet
** ��������: SPI ����ָ���ȡ
** �䡡��  : NONE
** �䡡��  : SPI ����ָ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_BUS_TYPE  API_SpiBusGet (VOID)
{
    return  (&_G_bustypeSpi);
}

/*********************************************************************************************************
** ��������: API_SpiCtrlRegister
** ��������: SPI ���߿�����ע��
** �䡡��  : pdtspictrl      SPI ������ָ��
**           pcName          SPI ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_SpiCtrlRegister (PLW_DT_SPI_CTRL  pdtspictrl, CPCHAR  pcName)
{
    PLW_SPI_ADAPTER     pspiadapter;

    if (!pdtspictrl || !pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (LW_NULL == pdtspictrl->DTSPICTRL_pfuncXferOne) {                /*  ��鴫��ӿ�                */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI transfer function is empty.\r\n");
        return  (PX_ERROR);
    }

    /*
     *  ��Ϊ SPI ����������Ϊƽ̨�豸ע�ᣬ
     *  �˴����Կ������������г�ʼ��,
     */
    pdtspictrl->DTSPICTRL_hBusLock = API_SemaphoreBCreate("spi_buslock",
                                                          LW_TRUE,
                                                          LW_OPTION_WAIT_FIFO |
                                                          LW_OPTION_OBJECT_GLOBAL,
                                                          LW_NULL);
    /*
     *  ע�ᵽ��ǰ�� bus ������
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
** ��������: API_SpiCtrlUnregister
** ��������: SPI ���߿�����ж��
** �䡡��  : pdtspictrl      SPI ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiDevRegister
** ��������: SPI �豸ע��
** �䡡��  : pdtspidevice     SPI �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
    pdevinstance->DEVHD_pbustype = &_G_bustypeSpi;                      /*  �����豸ʹ�õ���������      */

    if (API_DeviceRegister(pdevinstance) != ERROR_NONE) {               /*  ע���豸                    */
        return  (PX_ERROR);
    }

    pdtspictrl = pdtspidevice->DTSPIDEV_pdtspictrl;
    LW_BUS_INC_DEV_COUNT(&pdtspictrl->DTSPICTRL_pspiadapter->SPIADAPTER_pbusadapter);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpiDevDelete
** ��������: ɾ�� SPI �豸
** �䡡��  : pdtspidevice    SPI �豸ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiDrvRegister
** ��������: SPI �豸����ע��
** �䡡��  : pdtspidriver    SPI ����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiDrvUnregister
** ��������: SPI �豸����ж��
** �䡡��  : pdtspidriver    SPI ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiDevSetup
** ��������: SPI �豸����ǰ�ı�Ҫ��������Ҫ�Ǽ���豸��������ܹ�ƥ�乤����
**           Ҳ����ͨ���������Ļص��������ڿ���������Щ��Ҫ�����û��飬
**           һ������ SPI �豸�� probe �����е���
** �䡡��  : pdtspidevice    SPI �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
     *  ����ͬʱѡ�� DUAL ģʽ �� QUAD ģʽ
     */
    if (((usMode & LW_SPI_TX_DUAL) && (usMode & LW_SPI_TX_QUAD)) ||
        ((usMode & LW_SPI_RX_DUAL) && (usMode & LW_SPI_RX_QUAD))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI: can not select "
                     "dual and quad at the same time\r\n");
        return  (-EINVAL);
    }

    /*
     *  SPI ������ģʽ����ʹ�� DUAL �� QUAD ģʽ
     */
    if ((usMode & LW_SPI_3WIRE) && (usMode &
        (LW_SPI_TX_DUAL | LW_SPI_TX_QUAD |
         LW_SPI_RX_DUAL | LW_SPI_RX_QUAD))) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "SPI: can not select "
                     "dual and quad with 3 wire mode.\r\n");
        return  (-EINVAL);
    }

    /*
     *  ����ģʽ����Ч�ı�־
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
     *  ������Ҫ�Ǽ�� SPI �豸�Ĳ����Ƿ��������ƥ�䣬
     *  �����������󣬵��ÿ�����ע��� setup �ص�����
     */
    if (pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetup) {
        iRet = pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetup(pdtspidevice);
    }

    /*
     *  Ĭ��ȡ���� SPI �豸��ʹ��
     */
    if (pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetCs) {
        pdtspidevice->DTSPIDEV_pdtspictrl->DTSPICTRL_pfuncSetCs(pdtspidevice, LW_FALSE);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SpiDevTransfer
** ��������: SPI �豸���亯��
** �䡡��  : pdtspidevice    SPI �豸ָ��
**           pspixfer      SPI ������Ϣ���ƿ���
**           iNum          ������Ϣ������Ϣ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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

    __SPI_BUS_LOCK(pdtspictrl);                                         /*  ���� SPI ����               */

    if (pdtspictrl->DTSPICTRL_pfuncPrepareXfer) {
        iRet = pdtspictrl->DTSPICTRL_pfuncPrepareXfer(pdtspidevice);    /*  ����ǰ׼��                  */
        if (iRet) {
            goto  __xfer_ret;
        }
    }

    if (pdtspictrl->DTSPICTRL_pfuncSetCs) {
        pdtspictrl->DTSPICTRL_pfuncSetCs(pdtspidevice, LW_TRUE);        /*  ʹ�� CS ѡͨ                */
    }

    for (i = 0; i < iNum; i++) {
        iRet = pdtspictrl->DTSPICTRL_pfuncXferOne(pdtspidevice,
                                                pspixfer + i);          /*  ���д���                    */
        if (iRet) {
            break;
        }
    }

    if (pdtspictrl->DTSPICTRL_pfuncSetCs) {
        pdtspictrl->DTSPICTRL_pfuncSetCs(pdtspidevice, LW_FALSE);       /*  ȡ�� CS ѡͨ                */
    }

    if (pdtspictrl->DTSPICTRL_pfuncUnprepareXfer) {
        pdtspictrl->DTSPICTRL_pfuncUnprepareXfer(pdtspidevice);         /*  ȡ������׼��                */
    }

__xfer_ret:
    __SPI_BUS_UNLOCK(pdtspictrl);                                       /*  ���� SPI ����               */

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_SpiWrite
** ��������: SPI �豸���亯��
** �䡡��  : pdtspidevice    SPI �豸ָ��
**           pvBuf         Ҫд�������ָ��
**           iLen          Ҫд������ݳ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiRead
** ��������: SPI �豸������
** �䡡��  : pdtspidevice  SPI �豸ָ��
**           pvBuf         Ҫ����������ָ��
**           iLen          Ҫ���������ݳ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiWriteThenRead
** ��������: SPI �豸��д����ӿ�
** �䡡��  : pdtspidevice   SPI �豸ָ��
**           pvTxBuf        Ҫд�������ָ��
**           iTxLen         Ҫд������ݳ���
**           pvRxBuf        Ҫ����������ָ��
**           iRxLen         Ҫ���������ݳ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiW8R8
** ��������: ���� SPI ͨ��д 8 bit���ٶ� 8 bit ��������
** �䡡��  : pdtspidevice  SPI �豸
**           ucCmd         ��֮ǰ����Ĳ���
** �䡡��  : ������ص��Ǹ�ֵ�������
**           ������ص��ǷǸ�ֵ����Ϊ���ص�����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_SpiW8R16
** ��������: ���� SPI ͨ��д 8 bit���ٶ� 16 bit ��������
** �䡡��  : pdtspidevice  SPI �豸
**           ucCmd         ��֮ǰ����Ĳ���
** �䡡��  : ������ص��Ǹ�ֵ�������
**           ������ص��ǷǸ�ֵ����Ϊ���ص�����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
