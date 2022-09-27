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
** 文   件   名: ThreadIdSelf.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 07 月 18 日
**
** 描        述: 线程获得自己的句柄

** BUG
2007.07.18  加入 _DebugHandle() 功能
2009.10.12  加入提示注释.
2013.07.18  使用新的获取 TCB 的方法, 确保 SMP 系统安全.
2020.09.02  加入 Fast 获取方法.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
/*********************************************************************************************************
** 函数名称: API_ThreadIdSelf
** 功能描述: 当前线程 ID
** 输　入  : NONE
** 输　出  : ID
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
LW_OBJECT_HANDLE  API_ThreadIdSelf (VOID)
{
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;
    
#if LW_CFG_SMP_EN > 0
    INTREG          iregInterLevel;

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */

    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    ptcbCur = pcpuCur->CPU_ptcbTCBCur;                                  /*  当前线程                    */

    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

#else                                                                   /*  LW_CFG_SMP_EN > 0           */
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }
    
    ptcbCur = pcpuCur->CPU_ptcbTCBCur;                                  /*  当前线程                    */
#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
    
    return  (ptcbCur->TCB_ulId);
}
/*********************************************************************************************************
** 函数名称: API_ThreadIdSelfFast
** 功能描述: 当前线程 ID
** 输　入  : NONE
** 输　出  : ID
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
LW_OBJECT_HANDLE  API_ThreadIdSelfFast (VOID)
{
#if defined(LW_CFG_CPU_FAST_TLS) && (LW_CFG_CPU_FAST_TLS > 0)
    return  (__ARCH_FAST_TLS_TID());
#else                                                                   /*  LW_CFG_CPU_FAST_TLS > 0     */
    REGISTER PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur->TCB_ulId);
#endif                                                                  /*  LW_CFG_CPU_FAST_TLS == 0    */
}
/*********************************************************************************************************
** 函数名称: API_ThreadTcbSelf
** 功能描述: 当前线程 TCB (危险...)
** 输　入  : NONE
** 输　出  : TCB
** 全局变量: 
** 调用模块: 
                                           API 函数
                                           
                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API  
PLW_CLASS_TCB  API_ThreadTcbSelf (VOID)
{
    PLW_CLASS_CPU   pcpuCur;
    PLW_CLASS_TCB   ptcbCur;

#if LW_CFG_SMP_EN > 0
    INTREG          iregInterLevel;

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */

    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        KN_INT_ENABLE(iregInterLevel);                                  /*  打开中断                    */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    ptcbCur = pcpuCur->CPU_ptcbTCBCur;                                  /*  当前线程                    */

    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

#else                                                                   /*  LW_CFG_SMP_EN > 0           */
    pcpuCur = LW_CPU_GET_CUR();
    if (pcpuCur->CPU_ulInterNesting) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "called from ISR.\r\n");
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (LW_OBJECT_HANDLE_INVALID);
    }

    ptcbCur = pcpuCur->CPU_ptcbTCBCur;                                  /*  当前线程                    */
#endif                                                                  /*  LW_CFG_SMP_EN == 0          */
    
    return  (ptcbCur);
}
/*********************************************************************************************************
** 函数名称: API_ThreadTcbSelfFast
** 功能描述: 当前线程 TCB (危险...)
** 输　入  : NONE
** 输　出  : TCB
** 全局变量:
** 调用模块:
                                           API 函数

                                       (不得在中断中调用)
*********************************************************************************************************/
LW_API
PLW_CLASS_TCB  API_ThreadTcbSelfFast (VOID)
{
#if defined(LW_CFG_CPU_FAST_TLS) && (LW_CFG_CPU_FAST_TLS > 0)
    return  (__ARCH_FAST_TLS_TCB());
#else                                                                   /*  LW_CFG_CPU_FAST_TLS > 0     */
    REGISTER PLW_CLASS_TCB   ptcbCur;

    LW_TCB_GET_CUR_SAFE(ptcbCur);

    return  (ptcbCur);
#endif                                                                  /*  LW_CFG_CPU_FAST_TLS == 0    */
}
/*********************************************************************************************************
** 函数名称: API_ThreadIdInter
** 功能描述: 当前被中断线程 ID, BSP 中断程序使用
** 输　入  : NONE
** 输　出  : ID
** 全局变量: 
** 调用模块: 
** 注  意  : 只能在中断服务函数中使用
                                           API 函数
*********************************************************************************************************/
LW_API  
LW_OBJECT_HANDLE  API_ThreadIdInter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);

    return  (ptcbCur->TCB_ulId);
}
/*********************************************************************************************************
** 函数名称: API_ThreadTcbInter
** 功能描述: 当前被中断线程 TCB, BSP 中断程序使用
** 输　入  : NONE
** 输　出  : TCB
** 全局变量: 
** 调用模块: 
** 注  意  : 只能在中断服务函数中使用
                                           API 函数
*********************************************************************************************************/
LW_API  
PLW_CLASS_TCB  API_ThreadTcbInter (VOID)
{
    PLW_CLASS_TCB   ptcbCur;
    
    LW_TCB_GET_CUR(ptcbCur);

    return  (ptcbCur);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
