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
** ��   ��   ��: devtreeMdio.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 20 ��
**
** ��        ��: MDIO ����������豸����ؽӿ�
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
** ��������: __deviceTreeMdioAddrGet
** ��������: ͨ���豸����ȡ MDIO �豸��ַ
** �䡡��  : pdtnDev        �豸���ڵ�ָ��
** �䡡��  : �� 0 ʱΪ��Ч��ַ������ʱΪ PX_ERROR
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __deviceTreeMdioAddrGet (PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiAddr = 0;
    INT     iRet;

    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "reg", &uiAddr);      /*  ��ȡ MDIO �豸��ַ          */
    if (iRet) {
        DEVTREE_ERR("%s has invalid PHY address\n", pdtnDev->DTN_pcFullName);
        return  (PX_ERROR);
    }

    if (uiAddr >= MDIO_MAX_ADDR) {                                      /*  ��ַ����������Χ������ʧ��  */
        DEVTREE_ERR("%s PHY address %d is too large\n", 
                    pdtnDev->DTN_pcFullName, uiAddr);
        return  (PX_ERROR);
    }
        
    return  (uiAddr);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeMdioDevFind
** ��������: ͨ���豸���ڵ���� MDIO �豸
** �䡡��  : pdtnDev   �豸���ڵ�
** �䡡��  : �ɹ����ز��ҵ��� MDIO �豸ָ��,
**           ʧ�ܷ���   LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
PMDIO_DEVICE  API_DeviceTreeMdioDevFind (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEV_INSTANCE  pdevinstance;
    PMDIO_DEVICE      pmdiodev;
    
    if (!pdtnDev) {
        return  (LW_NULL);
    }
    
    pdevinstance = API_BusFindDevice(API_MdioBusGet(), pdtnDev);        /*  �� MDIO �����ϲ����豸      */
    if (pdevinstance) {
        pmdiodev = __mdioDevGet(pdevinstance);                          /*  ��ȡ MDIO �豸�ṹ��        */
    } else {
        pmdiodev = LW_NULL;
    }

    return  (pmdiodev);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeMdioRegister
** ��������: ���豸���н��� MDIO �������¹��ص��豸
** �䡡��  : pmdioadapter    MDIO ������ָ��
**           pdtnDev         MDIO ���������豸���ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeMdioRegister (PMDIO_ADAPTER      pmdioadapter,
                                 PLW_DEVTREE_NODE   pdtnDev)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iDevAddr;                                       /*  MDIO �豸��ַ               */
    INT                 iRet;

    iRet = API_MdioAdapterRegister(pmdioadapter);                       /*  ��ע�� MDIO ������          */
    if (iRet) {
        return  (iRet);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  �����ýڵ���ӽڵ�          */
        iDevAddr = __deviceTreeMdioAddrGet(pdtnChild);                  /*  ������ reg ��ַ����         */
        if (iDevAddr < 0) {
            continue;
        }
            
        API_DeviceTreeMdioDevRegister(pmdioadapter,
                                      pdtnChild, iDevAddr);             /*  ע�� MDIO �豸              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeMdioDevRegister
** ��������: ͨ���豸��ע�� MDIO �豸
** �䡡��  : pmdioadapter   MDIO ������ָ��
**           pdtnDev        MDIO ���������豸���ڵ�
**           uiAddr         MDIO �豸��ַ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeMdioDevRegister (PMDIO_ADAPTER      pmdioadapter,
                                    PLW_DEVTREE_NODE   pdtnDev,
                                    UINT               uiAddr)
{
    PMDIO_DEVICE    pmdiodevice;
    ULONG           ulIrqNum = 0;                                       /*  MDIO �豸���жϺ�           */
    UINT            uiFlag   = 0;
    INT             iRet;

    if (!pmdioadapter || !pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (API_DeviceTreePropertyFind(pdtnDev, "phy", LW_NULL)) {
        uiFlag |= MDIO_DEVICE_IS_PHY;                                   /*  phy �豸���� phy ��־       */
    }

    if (API_DeviceTreePropertyFind(pdtnDev, "c45", LW_NULL)) {
        uiFlag |= MDIO_DEVICE_IS_C45;                                   /*  MDIO c45 Э������ c45 ��־  */
    }

    API_DeviceTreeIrqGet(pdtnDev, 0, &ulIrqNum);
    if (ulIrqNum > 0) {
        pmdioadapter->MDIOADPT_uiIrqMap[uiAddr] = ulIrqNum;             /*  ��ȡ���洢�豸���жϺ�      */
    }

    DEVTREE_MSG("Mdio addr = 0x%x ,flag = 0x%x\r\n", uiAddr, uiFlag);
           
    pmdiodevice = API_MdioDevCreate(pmdioadapter, uiAddr, uiFlag);      /*  ���� MDIO �豸              */
    if (LW_NULL == pmdiodevice) {
        return  (PX_ERROR);
    }
    
    pmdiodevice->MDIODEV_devinstance.DEVHD_pdtnDev = pdtnDev;
                                                                        /*  ���� MDIO �豸���豸���ڵ�  */
    pmdiodevice->MDIODEV_devinstance.DEVHD_pcName  = pdtnDev->DTN_pcFullName;
                                                                        /*  ʹ�ýڵ�����Ϊ�豸��        */

    iRet = API_MdioDevRegister(pmdiodevice);                            /*  ע�� MDIO �豸��ϵͳ        */
    if (iRet) {
        DEVTREE_ERR("Mdio device %s register faild.\r\n", pdtnDev->DTN_pcFullName);
        API_MdioDevDelete(pmdiodevice);
    }

    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
