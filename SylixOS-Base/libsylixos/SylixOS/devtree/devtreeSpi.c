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
** ��   ��   ��: devtreeSpi.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 20 ��
**
** ��        ��: SPI ����������豸����ؽӿ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
/*********************************************************************************************************
** ��������: __deviceTreeSpiDevInfoGet
** ��������: ��ȡ SPI �豸��Ϣ
** �䡡��  : pdtspidev     SPI �豸ָ��
**           pdtnDev       SPI �豸���豸���ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __deviceTreeSpiDevInfoGet (PLW_DT_SPI_DEVICE  pdtspidev, PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiValue = 0;
    INT     iRet;

    iRet = API_DeviceTreeModaliasGet(pdtnDev,
                                     pdtspidev->DTSPIDEV_cName,
                                     sizeof(pdtspidev->DTSPIDEV_cName));/*  ��ȡ SPI �豸����           */
    if (iRet) {
        DEVTREE_ERR("%s Get name failed supported\r\n", 
                    pdtnDev->DTN_pcFullName);
        return  (PX_ERROR);
    }

    /*
     *  ��ȡ������ģʽ��Ϣ����λ�����Եȣ�
     */
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-cpha")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_CPHA;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-cpol")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_CPOL;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-3wire")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_3WIRE;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-lsb-first")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_LSB_FIRST;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-cs-high")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_CS_HIGH;
    }

    /*
     *  ��ȡ SPI �������ߵ����ݿ�ȣ����õ�ģʽ��
     */
    if (!API_DeviceTreePropertyU32Read(pdtnDev, "spi-tx-bus-width", &uiValue)) {

        switch (uiValue) {

        case 1:
            break;

        case 2:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_TX_DUAL;
            break;

        case 4:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_TX_QUAD;
            break;

        case 8:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_TX_OCTAL;
            break;

        default:
            DEVTREE_ERR("%s spi-tx-bus-width %d not supported\r\n", 
                        pdtnDev->DTN_pcFullName, uiValue);
            break;
        }
    }

    /*
     *  ��ȡ SPI �������ߵ����ݿ�ȣ����õ�ģʽ��
     */
    if (!API_DeviceTreePropertyU32Read(pdtnDev, "spi-rx-bus-width", &uiValue)) {

        switch (uiValue) {

        case 1:
            break;

        case 2:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_RX_DUAL;
            break;

        case 4:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_RX_QUAD;
            break;

        case 8:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_RX_OCTAL;
            break;

        default:
            DEVTREE_ERR("%s spi-rx-bus-width %d not supported\r\n", 
                        pdtnDev->DTN_pcFullName, uiValue);
            break;
        }
    }
    
    /*
     *  ��ȡ�豸��ַ
     */
    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "reg", &uiValue);
    if (iRet) {
        DEVTREE_ERR("%s has no valid 'reg' property.\r\n", 
                    pdtnDev->DTN_pcFullName);
        
        return  (PX_ERROR);
    }
    pdtspidev->DTSPIDEV_ucChipSel = uiValue;

    /*
     *  ��ȡ SPI �豸���������
     */
    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "spi-max-frequency", &uiValue);
    if (iRet) {
        DEVTREE_ERR("%s has no valid 'spi-max-frequency' property.\r\n", 
                    pdtnDev->DTN_pcFullName);
        
        return  (PX_ERROR);
    }

    pdtspidev->DTSPIDEV_uiSpeedMax                = uiValue;
    pdtspidev->DTSPIDEV_atomicUsageCnt.counter    = 0;
    pdtspidev->DTSPIDEV_devinstance.DEVHD_pdtnDev = pdtnDev;
    pdtspidev->DTSPIDEV_devinstance.DEVHD_pcName  = pdtnDev->DTN_pcFullName;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeSpiCtrlRegister
** ��������: ���豸���н�����ע�� SPI �������¹��ص��豸
** �䡡��  : pdtspictrl    SPI ������ָ��
**           pdtnDev     SPI �豸�ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeSpiCtrlRegister (PLW_DT_SPI_CTRL      pdtspictrl,
                                    PLW_DEVTREE_NODE     pdtnDev,
                                    CPCHAR               pcName)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iRet;

    iRet = API_SpiCtrlRegister(pdtspictrl, pcName);                     /*  ע�� SPI ������             */
    if (iRet) {
        return  (iRet);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  �����ýڵ���ӽڵ�          */
        API_DeviceTreeSpiDevRegister(pdtspictrl, pdtnChild);            /*  ע�� SPI �豸               */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeSpiDevRegister
** ��������: ͨ���豸��ע�� SPI �豸
** �䡡��  : pdtspictrl    SPI ������ָ��
**           pdtnDev       SPI �豸�ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeSpiDevRegister (PLW_DT_SPI_CTRL    pdtspictrl,
                                   PLW_DEVTREE_NODE   pdtnDev)
{
    PLW_DT_SPI_DEVICE  pdtspidev;
    INT                iRet;

    if (!pdtspictrl || !pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtspidev = __SHEAP_ZALLOC(sizeof(LW_DT_SPI_DEVICE));
    if (!pdtspidev) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    pdtspidev->DTSPIDEV_pdtspictrl = pdtspictrl;                        /*  ���� SPI �豸���ӵĿ�����   */
    
    iRet = __deviceTreeSpiDevInfoGet(pdtspidev, pdtnDev);               /*  ���豸���л�ȡ SPI �豸��Ϣ */
    if (iRet) {
        goto  __error_handle;
    }
           
    iRet = API_SpiDevRegister(pdtspidev);                               /*  ע�� SPI �豸��ϵͳ         */
    if (iRet) {
        DEVTREE_ERR("SPI device %s register failed.\r\n",
                    pdtspidev->DTSPIDEV_cName);
        goto  __error_handle;
    } else {
        DEVTREE_MSG("SPI device %s register successfully.\r\n",
                    pdtspidev->DTSPIDEV_cName);
    }

    return  (ERROR_NONE);

__error_handle:
    __SHEAP_FREE(pdtspidev);
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
