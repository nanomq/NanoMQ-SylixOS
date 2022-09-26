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
** 文   件   名: pty.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 06 月 15 日
**
** 描        述: 虚拟终端接口部分.
                 虚拟终端分为两个端口: 设备端和主控端! 
                 设备端是用软件仿真一个硬件串口.
                 主控端可以看成就是一个 TTY 设备.
*********************************************************************************************************/

#ifndef __PTY_H
#define __PTY_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SIO_DEVICE_EN > 0) && (LW_CFG_PTY_DEVICE_EN > 0)

LW_API INT  API_PtyDrvInstall(VOID);

LW_API INT  API_PtyDevCreate(PCHAR   pcName, 
                             size_t  stRdBufSize,
                             size_t  stWrtBufSize);

LW_API INT  API_PtyDevRemove(PCHAR   pcName);

/*********************************************************************************************************
  VxWorks 兼容宏
*********************************************************************************************************/

#define ptyDrv          API_PtyDrvInstall
#define ptyDevCreate    API_PtyDevCreate
#define ptyDevRemove    API_PtyDevRemove

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_SIO_DEVICE_EN > 0)  */
                                                                        /*  (LW_CFG_PTY_DEVICE_EN > 0)  */
#endif                                                                  /*  __PTY_H                     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
