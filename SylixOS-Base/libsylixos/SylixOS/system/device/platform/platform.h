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
** 文   件   名: platform.h
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 10 日
**
** 描        述: 平台设备处理库头文件
*********************************************************************************************************/

#ifndef __PLATFORM_H
#define __PLATFORM_H

/*********************************************************************************************************
  裁剪宏
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
