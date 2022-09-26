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
** 文   件   名: vutex.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2020 年 12 月 26 日
**
** 描        述: 等待变量锁.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "vutexLib.h"
/*********************************************************************************************************
  Post check value
*********************************************************************************************************/
#define LW_VUTEX_POST_CHECK_VALUE(flags)    \
        (!((flags) & LW_OPTION_VUTEX_FLAG_WAKEALL) && !((flags) & LW_OPTION_VUTEX_FLAG_DEEPWAKE))
/*********************************************************************************************************
** 函数名称: API_VutexPendEx
** 功能描述: 等待一个变量到达某个值 (带有条件)
** 输　入  : piVar     等待的变量地址
**           iCompare  比较方法
**           iDesired  期望的数值
**           ulTimeout 等待时间
** 输　出  : ERROR or 0: 被真实的等待值激活, 1: 被 WAKEALL 激活.
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
INT  API_VutexPendEx (INT  *piVar, INT  iCompare, INT  iDesired, ULONG  ulTimeout)
{
    INTREG         iregInterLevel;
    phys_addr_t    phyaddr;
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulTimeSave;                                          /*  系统事件记录                */
    INT            iSchedRet;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

__wait_again:
    if (_VutexWakeIsMatch(LW_ACCESS_ONCE_PTR(INT, piVar),
                          iCompare, iDesired)) {                        /*  是否已经满足条件            */
        return  (ERROR_NONE);

    } else if (ulTimeout == LW_OPTION_NOT_WAIT) {                       /*  不等待                      */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  超时                        */
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (_VutexWakeIsMatch(LW_ACCESS_ONCE_PTR(INT, piVar),
                          iCompare, iDesired)) {                        /*  在次确认是否已经满足条件    */
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }

#if LW_CFG_VMM_EN > 0
    if (__vmmLibVirtualToPhysical((addr_t)piVar, &phyaddr)) {           /*  转换为物理地址              */
        __KERNEL_EXIT();
        return  (PX_ERROR);
    }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
    phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  地址无效                    */
        __KERNEL_EXIT();
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    LW_TCB_GET_CUR(ptcbCur);

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */

    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_VUTEX;               /*  写状态位，开始等待          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  清空等待时间                */

    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  是否是无穷等待              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  设置超时时间                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  记录系统时间                */

    _VutexWaitQueue(ptcbCur, phyaddr, iDesired, iCompare);              /*  加入等待表                  */

    iSchedRet = __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  退出内核                    */
    if (iSchedRet == LW_SIGNAL_EINTR) {
        _ErrorHandle(EINTR);                                            /*  被信号打断                  */
        return  (PX_ERROR);

    } else if (iSchedRet == LW_SIGNAL_RESTART) {
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);
        if (ulTimeout != LW_OPTION_NOT_WAIT) {
            goto    __wait_again;                                       /*  重新等待                    */
        }

    } else if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {        /*  被唤醒或超时了              */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  等待超时                    */
        return  (PX_ERROR);
    }

    return  (_VutexWakeIsAll(ptcbCur) ? 1 : 0);                         /*  被唤醒                      */
}
/*********************************************************************************************************
** 函数名称: API_VutexPend
** 功能描述: 等待一个变量到达某个值
** 输　入  : piVar     等待的变量地址
**           iDesired  期望的数值
**           ulTimeout 等待时间
** 输　出  : ERROR or 0: 被真实的等待值激活, 1: 被 WAKEALL 激活.
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
INT  API_VutexPend (INT  *piVar, INT  iDesired, ULONG  ulTimeout)
{
    return  (API_VutexPendEx(piVar, LW_OPTION_VUTEX_EQU, iDesired, ulTimeout));
}
/*********************************************************************************************************
** 函数名称: API_VutexPostEx
** 功能描述: 改变一个变量为某个值唤醒等待线程
** 输　入  : piVar     变量地址
**           iValue    要设置的值
**           iFlags    flags
** 输　出  : ERROR or 唤醒的任务数
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
INT  API_VutexPostEx (INT  *piVar, INT  iValue, INT  iFlags)
{
    INT          iWakeCnt;
    phys_addr_t  phyaddr;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  不能在中断中调用            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (LW_VUTEX_POST_CHECK_VALUE(iFlags)) {
        if (LW_ACCESS_ONCE_PTR(INT, piVar) == iValue) {                 /*  非全部唤醒且值已经相同      */
            return  (ERROR_NONE);
        }
    }

    __KERNEL_ENTER();                                                   /*  进入内核                    */
    if (LW_VUTEX_POST_CHECK_VALUE(iFlags)) {
        if (LW_ACCESS_ONCE_PTR(INT, piVar) == iValue) {
            __KERNEL_EXIT();
            return  (ERROR_NONE);                                       /*  非全部唤醒且值已经相同      */
        }
    }

#if LW_CFG_VMM_EN > 0
    if (__vmmLibVirtualToPhysical((addr_t)piVar, &phyaddr)) {           /*  转换为物理地址              */
        __KERNEL_EXIT();
        return  (PX_ERROR);
    }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
    phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  地址无效                    */
        __KERNEL_EXIT();
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (!(iFlags & LW_OPTION_VUTEX_FLAG_DONTSET)) {                     /*  仅唤醒                      */
        LW_ACCESS_ONCE_PTR(INT, piVar) = iValue;
        KN_SMP_WMB();
    }

    iWakeCnt = _VutexWakeQueue(phyaddr, iValue, iFlags);                /*  唤醒合适的任务              */

    __KERNEL_EXIT();                                                    /*  退出内核                    */

    return  (iWakeCnt);
}
/*********************************************************************************************************
** 函数名称: API_VutexPost
** 功能描述: 改变一个变量为某个值
** 输　入  : piVar     变量地址
**           iValue    要设置的值
** 输　出  : ERROR or 唤醒的任务数
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
INT  API_VutexPost (INT  *piVar, INT  iValue)
{
    return  (API_VutexPostEx(piVar, iValue, 0));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
