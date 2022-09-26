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
** ��   ��   ��: i2cLibDevTree.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2020 �� 02 �� 06 ��
**
** ��        ��: I2C ����������ܿ�
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
#define __I2C_BUS_LOCK(padpt)       API_SemaphoreBPend(padpt->DTI2CADPT_hBusLock, LW_OPTION_WAIT_INFINITE)
#define __I2C_BUS_UNLOCK(padpt)     API_SemaphoreBPost(padpt->DTI2CADPT_hBusLock)
/*********************************************************************************************************
** ��������: __i2cDevProbe
** ��������: I2C �豸 probe ����
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __i2cDevRemove
** ��������: I2C �豸ж��
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __i2cBusMatch
** ��������: I2C �������豸������ƥ���ж�
** �䡡��  : pdevinstance    �豸ʵ��ָ��
**           pdrvinstance    ����ʵ��ָ��
** �䡡��  : ƥ�䷵�� 0����ƥ�䷵������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __i2cBusMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    INT   iRet;

    iRet = API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance);       /*  ͨ���豸�� compatible ƥ��  */

    return  (iRet);
}
/*********************************************************************************************************
  I2C ����ȫ�ֱ���
*********************************************************************************************************/
static LW_BUS_TYPE _G_bustypeI2c = {
    .BUS_pcName     = "i2c_bus",
    .BUS_pfuncMatch = __i2cBusMatch,
    .BUS_uiFlag     = BUS_AUTO_PROBE,
};
/*********************************************************************************************************
** ��������: API_I2cBusInit
** ��������: I2C ���߿��ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_I2cBusInit (VOID)
{
    INT  iRet;

    iRet = API_BusInit(&_G_bustypeI2c);

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_I2cBusGet
** ��������: I2C ����ָ���ȡ
** �䡡��  : NONE
** �䡡��  : I2C ����ָ��
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_BUS_TYPE  API_I2cBusGet (VOID)
{
    return  (&_G_bustypeI2c);
}
/*********************************************************************************************************
** ��������: API_I2cAdapterRegister
** ��������: I2C ����������(������)ע��
** �䡡��  : pdti2cadapter    I2C ������ָ��
**           pcName           I2C ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_I2cAdapterRegister (PLW_DT_I2C_ADAPTER  pdti2cadapter, CPCHAR  pcName)
{
    PLW_I2C_ADAPTER     pi2cadapter;
    
    if (!pdti2cadapter || !pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (LW_NULL == pdti2cadapter->DTI2CADPT_pi2cfuncs) {                /*  ����������                */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "I2C: Failed to register "
                     "adapter %s with no funcs.\r\n", pcName);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    /*
     *  ��Ϊ I2C ����������Ϊƽ̨�豸ע�ᣬ
     *  �˴����Կ������������г�ʼ��
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
     *  ע�ᵽ��ǰ�� bus ������
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
** ��������: API_I2cAdapterUnregister
** ��������: I2C ����������(������)ж��
** �䡡��  : pdti2cadapter    I2C ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_I2cDevRegister
** ��������: I2C �豸ע��
** �䡡��  : pdti2cdevice    I2C �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
    pdevinstance->DEVHD_pbustype = &_G_bustypeI2c;                      /*  �����豸ʹ�õ���������      */

    if (API_DeviceRegister(pdevinstance) != ERROR_NONE) {               /*  ע���豸                    */
        return  (PX_ERROR);
    }

    pdti2cadapter = pdti2cdevice->DTI2CDEV_pdti2cadapter;
    LW_BUS_INC_DEV_COUNT(&pdti2cadapter->DTI2CADPT_pi2cadapter->I2CADAPTER_pbusadapter);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_I2cDevDelete
** ��������: ɾ�� I2C �豸
** �䡡��  : pdti2cdevice    I2C �豸ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_I2cDrvRegister
** ��������: I2C �豸����ע��
** �䡡��  : pdti2cdriver    I2C ����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_I2cDrvUnregister
** ��������: I2C �豸����ж��
** �䡡��  : pdti2cdriver    I2C ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_I2cBusTransfer
** ��������: I2C ���ߴ��亯��
** �䡡��  : pdti2cadapter  I2C ������ָ��
**           pdti2cmsg      I2C ������Ϣ���ƿ���
**           iNum           ������Ϣ������Ϣ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_I2cDevTransfer
** ��������: I2C �豸���亯��
** �䡡��  : pdti2cdevice  I2C �豸ָ��
**           pdti2cmsg     I2C ������Ϣ���ƿ���
**           iNum          ������Ϣ������Ϣ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
