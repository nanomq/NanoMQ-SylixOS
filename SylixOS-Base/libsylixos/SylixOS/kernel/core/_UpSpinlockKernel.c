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
** 文   件   名: _UpSpinlockKernel.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2015 年 11 月 13 日
**
** 描        述: 单 CPU 系统内核锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_SMP_EN == 0
/*********************************************************************************************************
** 函数名称: _UpSpinLockIgnIrq
** 功能描述: 内核自旋锁加锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpKernelLockIgnIrq (VOID)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpKernelUnlockIgnIrq
** 功能描述: 内核自旋锁解锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpKernelUnlockIgnIrq (VOID)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解锁任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpKernelLockQuick
** 功能描述: 内核自旋锁加锁操作, 连同锁定中断
** 输　入  : piregInterLevel   中断锁定信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpKernelLockQuick (INTREG  *piregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur;

    *piregInterLevel = KN_INT_DISABLE();

    pcpuCur = LW_CPU_GET_CUR();
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_INC(pcpuCur->CPU_ptcbTCBCur);                     /*  锁定任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpKernelUnlockQuick
** 功能描述: 内核自旋锁解锁操作, 连同解锁中断, 不进行尝试调度
** 输　入  : iregInterLevel    中断锁定信息
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpKernelUnlockQuick (INTREG  iregInterLevel)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(pcpuCur->CPU_ptcbTCBCur);                     /*  解锁任务在当前 CPU          */
    }
    
    KN_INT_ENABLE(iregInterLevel);
}
/*********************************************************************************************************
** 函数名称: _UpKernelUnlockSched
** 功能描述: 内核 SMP 调度器切换完成后专用释放函数 (关中断状态下被调用)
** 输　入  : ptcbOwner     锁的持有者
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  _UpKernelUnlockSched (PLW_CLASS_TCB  ptcbOwner)
{
    PLW_CLASS_CPU   pcpuCur = LW_CPU_GET_CUR();
    
    if (!pcpuCur->CPU_ulInterNesting) {
        __THREAD_LOCK_DEC(ptcbOwner);                                   /*  解锁任务在当前 CPU          */
    }
}
/*********************************************************************************************************
** 函数名称: _UpKernTimeLockIgnIrq
** 功能描述: 内核时间自旋锁加锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _UpKernTimeLockIgnIrq (VOID)
{
}
/*********************************************************************************************************
** 函数名称: _UpKernTimeUnlockIgnIrq
** 功能描述: 内核时间自旋锁解锁操作, 忽略中断锁定 (必须在中断关闭的状态下被调用)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _UpKernTimeUnlockIgnIrq (VOID)
{
}
/*********************************************************************************************************
** 函数名称: _UpKernTimeLockQuick
** 功能描述: 内核时间自旋锁加锁操作, 连同锁定中断
** 输　入  : piregInterLevel   中断锁定信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _UpKernTimeLockQuick (INTREG  *piregInterLevel)
{
    *piregInterLevel = KN_INT_DISABLE();
}
/*********************************************************************************************************
** 函数名称: _UpKernTimeUnlockQuick
** 功能描述: 内核时间自旋锁解锁操作, 连同解锁中断, 不进行尝试调度
** 输　入  : iregInterLevel    中断锁定信息
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _UpKernTimeUnlockQuick (INTREG  iregInterLevel)
{
    KN_INT_ENABLE(iregInterLevel);
}

#endif                                                                  /*  LW_CFG_SMP_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
