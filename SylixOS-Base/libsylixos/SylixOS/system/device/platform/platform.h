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
** ��   ��   ��: platform.h
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 10 ��
**
** ��        ��: ƽ̨�豸�����ͷ�ļ�
*********************************************************************************************************/

#ifndef __PLATFORM_H
#define __PLATFORM_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

LW_API INT      API_PlatformBusInit(VOID);

LW_API INT      API_PlatformDeviceRegister(PLW_DEV_INSTANCE  pdevinstance);

LW_API VOID     API_PlatformDeviceUnregister(PLW_DEV_INSTANCE  pdevinstance);

LW_API INT      API_PlatformDriverRegister(PLW_DRV_INSTANCE  pdrvinstance);

LW_API VOID     API_PlatformDriverUnregister(PLW_DRV_INSTANCE  pdrvinstance);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PLATFORM_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
