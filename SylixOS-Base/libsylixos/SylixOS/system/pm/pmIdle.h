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
** 文   件   名: pmIdle.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2014 年 07 月 19 日
**
** 描        述: 电源管理设备空闲时间管理接口.
*********************************************************************************************************/

#ifndef __PMIDLE_H
#define __PMIDLE_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

/*********************************************************************************************************
  驱动程序调用接口
*********************************************************************************************************/

LW_API INT  API_PowerMDevSetWatchDog(PLW_PM_DEV  pmdev, ULONG  ulSecs);
LW_API INT  API_PowerMDevGetWatchDog(PLW_PM_DEV  pmdev, ULONG  *pulSecs);
LW_API INT  API_PowerMDevWatchDogOff(PLW_PM_DEV  pmdev);

#define pmDevSetWatchDog    API_PowerMDevSetWatchDog
#define pmDevGetWatchDog    API_PowerMDevGetWatchDog
#define pmDevWatchDogOff    API_PowerMDevWatchDogOff

#endif                                                                  /*  LW_CFG_POWERM_EN            */
#endif                                                                  /*  __PMIDLE_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
