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
** ��   ��   ��: platform.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 10 ��
**
** ��        ��: ƽ̨�豸�����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
** ��������: __platformMatch
** ��������: ƽ̨�������豸������ƥ��ӿ�
** �䡡��  : pdevinstance      �豸ָ��
**           pdrvinstance      ����ָ��
** �䡡��  : ƥ�䷵�� 0����ƥ�䷵������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __platformMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    return  (API_DeviceTreeDrvMatchDev(pdevinstance, pdrvinstance));
}
/*********************************************************************************************************
** ��������: __platformProbe
** ��������: ƽ̨���� probe �ӿ�
** �䡡��  : pdevinstance      �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __platformProbe (PLW_DEV_INSTANCE  pdevinstance)
{
    /*
     *  �� platform �����ϵ��豸 probe ʱ���Ƚ��е�Դ��ʱ�ӵȵ�����
     *  Ȼ���ٽ��о����豸������ probe
     */
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  ƽ̨����ȫ�ֱ���  
*********************************************************************************************************/
static LW_BUS_TYPE  _G_bustypePlatform = {
    .BUS_pcName     = "platform",
    .BUS_pfuncMatch = __platformMatch,
    .BUS_pfuncProbe = __platformProbe,
    .BUS_uiFlag     = BUS_AUTO_PROBE | BUS_FORCE_DRV_PROBE,
};
/*********************************************************************************************************
** ��������: API_PlatformBusInit
** ��������: ƽ̨���߳�ʼ��
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PlatformBusInit (VOID)
{
    return  (API_BusInit(&_G_bustypePlatform));
}
/*********************************************************************************************************
** ��������: API_PlatformDeviceRegister
** ��������: ƽ̨�豸ע��
** �䡡��  : pdevinstance    ƽ̨�豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_PlatformDeviceUnregister
** ��������: ƽ̨�豸ж��
** �䡡��  : pdevinstance    ƽ̨�豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_PlatformDriverRegister
** ��������: ƽ̨�豸����ע��
** �䡡��  : pdrvinstance    ����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_PlatformDriverUnregister
** ��������: ƽ̨�豸����ж��
** �䡡��  : pdrvinstance    ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
