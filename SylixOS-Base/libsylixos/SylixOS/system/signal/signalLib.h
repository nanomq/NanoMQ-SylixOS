/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: signalLib.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 06 月 03 日
**
** 描        述: 这是系统信号内部处理相关定义
*********************************************************************************************************/

#ifndef __SIGNALLIB_H
#define __SIGNALLIB_H

#if  LW_CFG_SIGNAL_EN > 0

/*********************************************************************************************************
  内核对 sigsetjmp 与 siglongjmp 内部函数的声明
*********************************************************************************************************/

#ifdef __SYLIXOS_SETJMP
VOID  __sigsetjmpSetup(sigjmp_buf sigjmpEnv, INT iSaveSigs);
VOID  __siglongjmpSetup(sigjmp_buf sigjmpEnv, INT iVal);
#endif                                                                  /*  __SYLIXOS_SETJMP            */

#endif                                                                  /*  LW_CFG_SIGNAL_EN > 0        */
#endif                                                                  /*  __SIGNALLIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
