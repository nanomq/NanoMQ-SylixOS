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
** 文   件   名: vutexLib.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2020 年 12 月 26 日
**
** 描        述: 等待变量锁内部实现.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define LW_VUTEX_HASH_SIZE      1024
#define LW_VUTEX_HASH_MASK      0x3ff
#define LW_VUTEX_HASH_INDEX(a)  _VutexHash(a)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _k_plineVutexHashHeader[LW_VUTEX_HASH_SIZE];
static PLW_LIST_LINE         _k_plineVutexOp;
/*********************************************************************************************************
** 函数名称: _VutexHash
** 功能描述: 计算 vutex hash 值
** 输　入  : phyaddr   物理地址
** 输　出  : hash index
** 全局变量:
** 调用模块:
*********************************************************************************************************/
LW_INLINE static UINT32  _VutexHash (phys_addr_t  phyaddr)
{
             UINT32  uiTemp  = (UINT32)(phyaddr >> 2);
    REGISTER UINT8  *pucTemp = (UINT8 *)&uiTemp;

    return  ((pucTemp[0] + pucTemp[1] + pucTemp[2] + pucTemp[3]) & LW_VUTEX_HASH_MASK);
}
/*********************************************************************************************************
** 函数名称: _VutexWakeIsMatch
** 功能描述: 判断条件是否匹配
** 输　入  : iValue    真实数值
**           iCompare  比较方法
**           iDesired  期望数值
** 输　出  : 是否匹配
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  _VutexWakeIsMatch (INT  iValue, INT  iCompare, INT  iDesired)
{
    BOOL  bMatch = LW_FALSE;

    switch (iCompare) {

    case LW_OPTION_VUTEX_EQU:
        if (iValue == iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_NOT_EQU:
        if (iValue != iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_LESS:
        if (iValue < iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_LESS_EQU:
        if (iValue <= iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_GREATER:
        if (iValue > iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_GREATER_EQU:
        if (iValue >= iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_AND:
        if ((iValue & iDesired) == iDesired) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_NOT:
        if ((iValue & iDesired) == 0) {
            bMatch = LW_TRUE;
        }
        break;

    case LW_OPTION_VUTEX_OR:
        if (iValue & iDesired) {
            bMatch = LW_TRUE;
        }
        break;
    }

    return  (bMatch);
}
/*********************************************************************************************************
** 函数名称: _VutexWakeIsAll
** 功能描述: 获得唤醒类型是否为 WAKEALL
** 输　入  : ptcb      任务
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  _VutexWakeIsAll (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    return  (pvutex->VUTEX_bWakeAll);
}
/*********************************************************************************************************
** 函数名称: _VutexWaitQueue
** 功能描述: 将当前任务加入等待队列 (进入内核且关中断情况被调用)
** 输　入  : ptcbCur   当前任务
**           phyaddr   等待的物理地址
**           iDesired  期望的值
**           iCompare  比较方法
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexWaitQueue (PLW_CLASS_TCB  ptcbCur, phys_addr_t  phyaddr, INT32  iDesired, INT  iCompare)
{
    REGISTER PLW_CLASS_PCB      ppcb;
             PLW_VUTEX_CONTEXT  pvutex = &ptcbCur->TCB_vutex;

    pvutex->VUTEX_phyaddr = phyaddr;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  从就绪队列中删除            */

    pvutex->VUTEX_bWakeAll = LW_FALSE;
    pvutex->VUTEX_iCompare = iCompare;
    pvutex->VUTEX_iDesired = iDesired;
    pvutex->VUTEX_uiHash   = LW_VUTEX_HASH_INDEX(phyaddr);
    _List_Line_Add_Ahead(&pvutex->VUTEX_lineWait,                       /*  加入等待变量队列            */
                         &_k_plineVutexHashHeader[pvutex->VUTEX_uiHash]);

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  加入超时扫描链              */
    }
}
/*********************************************************************************************************
** 函数名称: _VutexWaitQueue
** 功能描述: 根据地址和期望数据唤醒目标任务 (进入内核情况被调用)
** 输　入  : phyaddr   物理地址
**           iValue    写入的值
**           iFlags    唤醒选项
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  _VutexWakeQueue (phys_addr_t  phyaddr, INT32  iValue, INT  iFlags)
{
    INTREG             iregInterLevel;
    UINT32             uiHash = LW_VUTEX_HASH_INDEX(phyaddr);
    INT                iWakeCnt = 0;
    BOOL               bMatch;
    PLW_CLASS_TCB      ptcb;
    PLW_CLASS_PCB      ppcb;
    PLW_VUTEX_CONTEXT  pvutex;
    PLW_LIST_LINE      plineTemp;

    iregInterLevel = KN_INT_DISABLE();                                  /*  关闭中断                    */

    _k_plineVutexOp = _k_plineVutexHashHeader[uiHash];
    while (_k_plineVutexOp) {
        plineTemp       = _k_plineVutexOp;
        _k_plineVutexOp = _list_line_get_next(plineTemp);               /*  下一个等待节点              */

        pvutex = _LIST_ENTRY(plineTemp, LW_VUTEX_CONTEXT, VUTEX_lineWait);
        if (pvutex->VUTEX_phyaddr == phyaddr) {
            bMatch = (iFlags & LW_OPTION_VUTEX_FLAG_WAKEALL)
                   ? LW_TRUE
                   : _VutexWakeIsMatch(iValue, pvutex->VUTEX_iCompare, pvutex->VUTEX_iDesired);
        } else {
            bMatch = LW_FALSE;
        }

        if (bMatch) {
            _List_Line_Del(&pvutex->VUTEX_lineWait,
                           &_k_plineVutexHashHeader[uiHash]);           /*  退出等待变量队列            */
            pvutex->VUTEX_phyaddr  = LW_PHY_ADDR_INVALID;               /*  删除等待信息                */
            pvutex->VUTEX_bWakeAll = (iFlags & LW_OPTION_VUTEX_FLAG_WAKEALL)
                                   ? LW_TRUE : LW_FALSE;

            ptcb = _LIST_ENTRY(pvutex, LW_CLASS_TCB, TCB_vutex);
            if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
                __DEL_FROM_WAKEUP_LINE(ptcb);                           /*  退出超时队列                */
                ptcb->TCB_ulDelay = 0ul;
            }

            if (ptcb->TCB_ucWaitTimeout) {
                ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;           /*  清除超时位                  */
            } else {
                ptcb->TCB_usStatus = (UINT16)(ptcb->TCB_usStatus & ~LW_THREAD_STATUS_VUTEX);
                if (__LW_THREAD_IS_READY(ptcb)) {                       /*  是否就绪                    */
                    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT; /*  调度激活方式                */
                    ppcb = _GetPcb(ptcb);
                    __ADD_TO_READY_RING(ptcb, ppcb);                    /*  加入到相对优先级就绪环      */
                }
            }

            KN_INT_ENABLE(iregInterLevel);                              /*  打开中断                    */

            iWakeCnt++;

            iregInterLevel = KN_INT_DISABLE();                          /*  关闭中断                    */
        }
    }

    KN_INT_ENABLE(iregInterLevel);                                      /*  打开中断                    */

    return  (iWakeCnt);
}
/*********************************************************************************************************
** 函数名称: _VutexInitCtx
** 功能描述: 初始化任务控制块 vutex 上下文
** 输　入  : ptcb      任务
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexInitCtx (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    pvutex->VUTEX_phyaddr = LW_PHY_ADDR_INVALID;
    _LIST_LINE_INIT_IN_CODE(pvutex->VUTEX_lineWait);
}
/*********************************************************************************************************
** 函数名称: _VutexUnQueue
** 功能描述: 将目标任务退出等待队列 (进入内核且关中断情况被调用)
** 输　入  : ptcb      目标任务
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  _VutexUnQueue (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    if (_k_plineVutexOp == &pvutex->VUTEX_lineWait) {
        _k_plineVutexOp =  _list_line_get_next(_k_plineVutexOp);        /*  移动至下一个节点            */
    }

    _List_Line_Del(&pvutex->VUTEX_lineWait,
                   &_k_plineVutexHashHeader[pvutex->VUTEX_uiHash]);     /*  退出等待变量队列            */

    pvutex->VUTEX_uiHash  = 0;
    pvutex->VUTEX_phyaddr = LW_PHY_ADDR_INVALID;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
