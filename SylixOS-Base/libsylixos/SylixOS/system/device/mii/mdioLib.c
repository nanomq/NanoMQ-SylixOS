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
** ��   ��   ��: mdioLib.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 20 ��
**
** ��        ��: mdio ����������ܿ�
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __MDIO_BUS_UNLOCK(pAdpt)    API_SemaphoreMPost(pAdpt->MDIOADPT_hBusLock)
#define __MDIO_BUS_LOCK(pAdpt)      API_SemaphoreMPend(pAdpt->MDIOADPT_hBusLock, \
                                                       LW_OPTION_WAIT_INFINITE)
/*********************************************************************************************************
** ��������: __mdioDevProbe
** ��������: mdio �豸 probe ����
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __mdioDevRemove
** ��������: mdio �豸ж��
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __mdioBusMatch
** ��������: mdio �������豸������ƥ���ж�
** �䡡��  : pdevinstance    �豸ʵ��ָ��
**           pdrvinstance    ����ʵ��ָ��
** �䡡��  : ƥ�䷵�� 0����ƥ�䷵������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __mdioBusMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    PMDIO_DEVICE  pmdiodev = __mdioDevGet(pdevinstance);
    INT           iRet;

    iRet = API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance);       /*  ͨ���豸�� compatibale ƥ�� */
    if (!iRet) {
        return  (iRet);
    }

    if (pmdiodev->MDIODEV_pfuncBusMatch) {                              /*  ͨ���ҽӵ�ƥ�亯��ƥ��      */
        return  (pmdiodev->MDIODEV_pfuncBusMatch(pdevinstance, pdrvinstance));
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
  MDIO ����ȫ�ֱ���
*********************************************************************************************************/
static LW_BUS_TYPE _G_bustypeMdio = {
    .BUS_pcName     = "mdio_bus",
    .BUS_pfuncMatch = __mdioBusMatch,
    .BUS_uiFlag     = BUS_AUTO_PROBE,
};
/*********************************************************************************************************
** ��������: API_MdioBusInit
** ��������: MDIO ���߳�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_MdioBusInit (VOID)
{
    return  (API_BusInit(&_G_bustypeMdio));
}
/*********************************************************************************************************
** ��������: API_MdioBusGet
** ��������: MDIO ����ָ���ȡ
** �䡡��  : NONE
** �䡡��  : MDIO ����ָ��
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_BUS_TYPE  API_MdioBusGet (VOID)
{
    return  (&_G_bustypeMdio);
}
/*********************************************************************************************************
** ��������: API_MdioAdapterRegister
** ��������: MDIO ����������(������)ע��
** �䡡��  : pmdioadapter    MDIO ������ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_MdioDrvRegister
** ��������: MDIO ����ע��
** �䡡��  : pmdiodrv    MDIO ����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
     *  ��ͨ������ʵ�����и�ֵ
     */
    pdrvinstance = &pmdiodrv->MDIODRV_drvinstance;
    pdrvinstance->DRVHD_pcName      = pmdiodrv->MDIODRV_pcName;
    pdrvinstance->DRVHD_pbustype    = &_G_bustypeMdio;
    pdrvinstance->DRVHD_pfuncProbe  = __mdioDevProbe;
    pdrvinstance->DRVHD_pfuncRemove = __mdioDevRemove;

    return  (API_DriverRegister(pdrvinstance));                         /*  ע��ͨ������                */
}
/*********************************************************************************************************
** ��������: API_MdioDevCreate
** ��������: MDIO �豸����
** �䡡��  : pmdioadapter   MDIO ������ָ��
**           uiAddr         MDIO �豸��ַ
**           uiFlag         MDIO �豸���ͱ�־
** �䡡��  : �ɹ����ش����� MDIO �豸ָ��, ʧ�ܷ��� LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
PMDIO_DEVICE  API_MdioDevCreate (PMDIO_ADAPTER  pmdioadapter,
                                 UINT           uiAddr,
                                 UINT           uiFlag)
{
    PMDIO_DEVICE  pmdiodev;

    pmdiodev = __SHEAP_ZALLOC(sizeof(MDIO_DEVICE));                     /*  MDIO �豸�����ڴ�           */
    if (LW_NULL == pmdiodev) {
        return  (LW_NULL);
    }

    /*
     *  ��ʼ�� MDIO �豸
     */
    pmdiodev->MDIODEV_uiFlag        = uiFlag;
    pmdiodev->MDIODEV_pmdioadapter  = pmdioadapter;
    pmdiodev->MDIODEV_uiAddr        = uiAddr;
    snprintf(pmdiodev->MDIODEV_cName, MDIO_NAME_SIZE, "%s:%02x",
             pmdioadapter->MDIOADPT_pcName, uiAddr);
    
    pmdiodev->MDIODEV_devinstance.DEVHD_pbustype = &_G_bustypeMdio;

    if (uiFlag & MDIO_DEVICE_IS_PHY) {
        pmdiodev->MDIODEV_pvPriv = API_PhyDevCreate(pmdiodev);          /*  ���� phy �豸               */
    }

    return  (pmdiodev);
}
/*********************************************************************************************************
** ��������: API_MdioDevDelete
** ��������: ɾ�� MDIO �豸
** �䡡��  : pmdiodev    MDIO �豸ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_MdioDevDelete (PMDIO_DEVICE  pmdiodev)
{
    if (pmdiodev) {
        __SHEAP_FREE(pmdiodev);
    }
}
/*********************************************************************************************************
** ��������: API_MdioDevRegister
** ��������: MDIO �豸ע��
** �䡡��  : pmdiodev    MDIO �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
    pmdioadpter->MDIOADPT_DevMap[uiDevAddr] = pmdiodev;                 /*  �����豸���У��õ�ַ����    */

    return  (API_DeviceRegister(pdevinstance));                         /*  ע���豸                    */
}
/*********************************************************************************************************
** ��������: API_MdioBusRead
** ��������: MDIO ���߶�����
** �䡡��  : pmdioadapter   MDIO ������ָ��
**           uiAddr         MDIO �豸��ַ
**           uiRegNum       MDIO �豸�Ĵ�����ַ
** �䡡��  : ���ض�ȡ������
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_MdioBusWrite
** ��������: MDIO ����д����
** �䡡��  : pmdioadapter   MDIO ������ָ��
**           uiAddr         MDIO �豸��ַ
**           uiRegNum       MDIO �豸�Ĵ�����ַ
**           usValue        Ҫд�������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_MdioBusReset
** ��������: MDIO ���߸�λ����
** �䡡��  : pmdioadapter    MDIO ������ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_MdioDevRead
** ��������: MDIO �豸������
** �䡡��  : pmdiodev    MDIO �豸ָ��
**           uiRegNum    Ҫ��ȡ�ļĴ�����ַ
** �䡡��  : ���ض�ȡ������
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_MdioDevWrite
** ��������: MDIO�豸д����
** �䡡��  : pmdiodev    MDIO �豸ָ��
**           uiRegNum    Ҫд��ļĴ�����ַ
**           usValue     Ҫд�������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
