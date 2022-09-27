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
** 文   件   名: ThreadOnce.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2009 年 02 月 03 日
**
** 描        述: 这是系统类 pthread_once 支持.

** BUG:
2010.01.09  使用 ATOMIC 锁进行互斥.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
  STATUS
*********************************************************************************************************/
#define __THREAD_ONCE_STATUS_NONE   0
#define __THREAD_ONCE_STATUS_INIT   1
#define __THREAD_ONCE_STATUS_DOWN   2
/*********************************************************************************************************
** 函数名称: __threadOnceCleanPush
** 功能描述: 安装一个正在执行的初始化操作.
** 输　入  : piOnce            once 变量.
** 输　出  : TRUE or FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_THREAD_EXT_EN > 0

static BOOL  __threadOnceCleanPush (INT  *piOnce)
{
    INTREG              iregInterLevel;
    PLW_CLASS_TCB       ptcbCur;
    __PLW_CLEANUP_ONCE  pclean;
    __PLW_THREAD_EXT    ptex;

    pclean = (__PLW_CLEANUP_ONCE)__KHEAP_ALLOC(sizeof(__LW_CLEANUP_ONCE));
    if (pclean) {
        pclean->CUO_piOnce = piOnce;
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "Once buffer low memory.\r\n");
        return  (LW_FALSE);
    }

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */

    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;

    _LIST_MONO_LINK(&pclean->CUO_monoNext, ptex->TEX_pmonoOnceHeader);
    ptex->TEX_pmonoOnceHeader = &pclean->CUO_monoNext;

    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __threadOnceCleanPop
** 功能描述: 线程安全的仅执行一遍指定函数.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __threadOnceCleanPop (VOID)
{
    INTREG              iregInterLevel;
    PLW_CLASS_TCB       ptcbCur;
    __PLW_CLEANUP_ONCE  pclean;
    __PLW_THREAD_EXT    ptex;

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */

    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;

    pclean = (__PLW_CLEANUP_ONCE)ptex->TEX_pmonoOnceHeader;
    if (pclean) {
        _list_mono_next(&ptex->TEX_pmonoOnceHeader);
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        __KHEAP_FREE(pclean);

    } else {
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
    }
}
/*********************************************************************************************************
** 函数名称: API_ThreadOnce
** 功能描述: 线程安全的仅执行一遍指定函数.
** 输　入  : piOnce            必须初始化为 0.
**           pfuncRoutine      需要执行的函数.
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_ThreadOnce (INT  *piOnce, VOIDFUNCPTR  pfuncRoutine)
{
    BOOL               bPush;
    INT                iValue;
    atomic_t          *patomic;

    if (!piOnce || !pfuncRoutine) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patomic = (atomic_t *)piOnce;

    do {
        iValue = __LW_ATOMIC_GET(patomic);
        switch (iValue) {

        case __THREAD_ONCE_STATUS_DOWN:                                 /*  已经执行过了                */
            return  (ERROR_NONE);

        case __THREAD_ONCE_STATUS_INIT:                                 /*  正在被其他任务执行          */
            API_VutexPend(piOnce,
                          __THREAD_ONCE_STATUS_DOWN,
                          LW_OPTION_WAIT_INFINITE);                     /*  等待初始化执行完毕          */
            continue;

        case __THREAD_ONCE_STATUS_NONE:                                 /*  可以尝试初始化              */
            iValue = __LW_ATOMIC_CAS(patomic,
                                     __THREAD_ONCE_STATUS_NONE,
                                     __THREAD_ONCE_STATUS_INIT);
            break;

        default:                                                        /*  状态错误                    */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    } while (iValue != __THREAD_ONCE_STATUS_NONE);                      /*  CAS 失败                    */

    bPush = __threadOnceCleanPush(piOnce);                              /*  添加中途退出回收点          */

    LW_SOFUNC_PREPARE(pfuncRoutine);
    pfuncRoutine();                                                     /*  执行                        */

    if (bPush) {
        __threadOnceCleanPop();                                         /*  删除回收点                  */
    }
    
    API_VutexPostEx(piOnce,
                    __THREAD_ONCE_STATUS_DOWN,
                    LW_OPTION_VUTEX_FLAG_WAKEALL);                      /*  执行完毕, 唤醒等待的任务    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_ThreadOnce2
** 功能描述: 线程安全的仅执行一遍指定函数.
** 输　入  : piOnce            必须初始化为 0.
**           pfuncRoutine      需要执行的函数.
**           pvArg             执行参数
** 输　出  : ERROR_NONE
** 全局变量: 
** 调用模块: 
                                           API 函数
*********************************************************************************************************/
LW_API  
INT  API_ThreadOnce2 (INT  *piOnce, VOIDFUNCPTR  pfuncRoutine, PVOID  pvArg)
{
    BOOL               bPush;
    INT                iValue;
    atomic_t          *patomic;

    if (!piOnce || !pfuncRoutine) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    patomic = (atomic_t *)piOnce;

    do {
        iValue = __LW_ATOMIC_GET(patomic);
        switch (iValue) {

        case __THREAD_ONCE_STATUS_DOWN:                                 /*  已经执行过了                */
            return  (ERROR_NONE);

        case __THREAD_ONCE_STATUS_INIT:                                 /*  正在被其他任务执行          */
            API_VutexPend(piOnce,
                          __THREAD_ONCE_STATUS_DOWN,
                          LW_OPTION_WAIT_INFINITE);                     /*  等待初始化执行完毕          */
            continue;

        case __THREAD_ONCE_STATUS_NONE:                                 /*  可以尝试初始化              */
            iValue = __LW_ATOMIC_CAS(patomic,
                                     __THREAD_ONCE_STATUS_NONE,
                                     __THREAD_ONCE_STATUS_INIT);
            break;

        default:                                                        /*  状态错误                    */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    } while (iValue != __THREAD_ONCE_STATUS_NONE);                      /*  CAS 失败                    */

    bPush = __threadOnceCleanPush(piOnce);                              /*  添加中途退出回收点          */

    LW_SOFUNC_PREPARE(pfuncRoutine);
    pfuncRoutine(pvArg);                                                /*  执行                        */

    if (bPush) {
        __threadOnceCleanPop();                                         /*  删除回收点                  */
    }
    
    API_VutexPostEx(piOnce,
                    __THREAD_ONCE_STATUS_DOWN,
                    LW_OPTION_VUTEX_FLAG_WAKEALL);                      /*  执行完毕, 唤醒等待的任务    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
