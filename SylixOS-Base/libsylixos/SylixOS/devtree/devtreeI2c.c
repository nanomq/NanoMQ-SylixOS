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
** ��   ��   ��: devtreeI2c.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 20 ��
**
** ��        ��: I2C ����������豸����ؽӿ�
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
/*********************************************************************************************************
** ��������: __deviceTreeI2cDevInfoGet
** ��������: ��ȡ I2C �豸��Ϣ
** �䡡��  : pdti2cdevice   I2C �豸ָ��
**           pdtnDev        I2C �豸���豸���ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __deviceTreeI2cDevInfoGet (PLW_DT_I2C_DEVICE  pdti2cdevice, PLW_DEVTREE_NODE  pdtnDev)
{
    ULONG   ulIrqNum = 0;  
    UINT32  uiAddr   = 0;
    INT     iRet;

    API_DeviceTreeIrqGet(pdtnDev, 0, &ulIrqNum);                        /*  ��ȡ I2C �豸�жϺ�         */
    if (ulIrqNum > 0) {
        pdti2cdevice->DTI2CDEV_ulIrq = ulIrqNum;
    }

    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "reg", &uiAddr);      /*  ��ȡ I2C �豸��ַ           */
    if (iRet) {
        DEVTREE_ERR("%s has invalid address\n", pdtnDev->DTN_pcFullName);
        return  (PX_ERROR);
    }

    if (uiAddr & LW_I2C_TEN_BIT_ADDRESS) {
        uiAddr &= ~LW_I2C_TEN_BIT_ADDRESS;
        pdti2cdevice->DTI2CDEV_usFlag |= LW_I2C_CLIENT_TEN;
    }
    
    if (uiAddr & LW_I2C_OWN_SLAVE_ADDRESS) {
        uiAddr &= ~LW_I2C_OWN_SLAVE_ADDRESS;
        pdti2cdevice->DTI2CDEV_usFlag |= LW_I2C_CLIENT_SLAVE;
    }
    
    /*
     *  ����ַ�Ϸ���
     */
    if (pdti2cdevice->DTI2CDEV_usFlag & LW_I2C_CLIENT_TEN) {
        if (uiAddr > 0x3ff) {
            return  (PX_ERROR);
        }
    } else {
        if ((uiAddr == 0x00) || (uiAddr > 0x7f)) {
            return  (PX_ERROR);
        }
    }
    
    pdti2cdevice->DTI2CDEV_usAddr = uiAddr;
    
    snprintf(pdti2cdevice->DTI2CDEV_cName, LW_I2C_NAME_SIZE, "%s",
             pdtnDev->DTN_pcFullName);                                  /* ���� I2C �豸����            */

    pdti2cdevice->DTI2CDEV_atomicUsageCnt.counter    = 0;
    pdti2cdevice->DTI2CDEV_devinstance.DEVHD_pdtnDev = pdtnDev;
    pdti2cdevice->DTI2CDEV_devinstance.DEVHD_pcName  = pdtnDev->DTN_pcFullName;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeI2cAdapterRegister
** ��������: ���豸���н��� I2C �������¹��ص��豸
** �䡡��  : pdti2cadapter  I2C ������ָ��
**           pdtnDev        I2C ���������豸���ڵ�
**           pcName         I2C ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeI2cAdapterRegister (PLW_DT_I2C_ADAPTER  pdti2cadapter,
                                       PLW_DEVTREE_NODE    pdtnDev,
                                       CPCHAR              pcName)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iRet;

    iRet = API_I2cAdapterRegister(pdti2cadapter, pcName);               /*  ��ע�� I2C ������           */
    if (iRet) {
        return  (iRet);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  �����ýڵ���ӽڵ�          */
        API_DeviceTreeI2cDevRegister(pdti2cadapter, pdtnChild);         /*  ע�� I2C �豸               */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeI2cDevRegister
** ��������: ͨ���豸��ע�� I2C �豸
** �䡡��  : pdti2cadapter  I2C ������ָ��
**           pdtnDev        I2C �豸�ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeI2cDevRegister (PLW_DT_I2C_ADAPTER  pdti2cadapter,
                                   PLW_DEVTREE_NODE    pdtnDev)
{
    PLW_DT_I2C_DEVICE  pdti2cdevice;
    INT                iRet;
    
    pdti2cdevice = __SHEAP_ZALLOC(sizeof(LW_DT_I2C_DEVICE));
    if (!pdti2cdevice) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pdti2cdevice->DTI2CDEV_pdti2cadapter = pdti2cadapter;

    iRet = __deviceTreeI2cDevInfoGet(pdti2cdevice, pdtnDev);            /*  ���豸���л�ȡ I2C �豸��Ϣ */
    if (iRet) {
        __SHEAP_FREE(pdti2cdevice);
        return  (PX_ERROR);
    }
           
    iRet = API_I2cDevRegister(pdti2cdevice);                            /*  ע�� I2C �豸��ϵͳ         */
    if (iRet) {
        DEVTREE_ERR("I2C device %s register failed.\r\n", 
                    pdtnDev->DTN_pcFullName);
        __SHEAP_FREE(pdti2cdevice);

    } else {
        DEVTREE_MSG("Register I2C device %s at addr 0x%x.\r\n", 
                    pdtnDev->DTN_pcFullName, pdti2cdevice->DTI2CDEV_usAddr);
    }

    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
