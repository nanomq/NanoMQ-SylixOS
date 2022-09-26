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
** ��   ��   ��: vutex.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2020 �� 12 �� 26 ��
**
** ��        ��: �ȴ�������.
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
** ��������: API_VutexPendEx
** ��������: �ȴ�һ����������ĳ��ֵ (��������)
** �䡡��  : piVar     �ȴ��ı�����ַ
**           iCompare  �ȽϷ���
**           iDesired  ��������ֵ
**           ulTimeout �ȴ�ʱ��
** �䡡��  : ERROR or 0: ����ʵ�ĵȴ�ֵ����, 1: �� WAKEALL ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
INT  API_VutexPendEx (INT  *piVar, INT  iCompare, INT  iDesired, ULONG  ulTimeout)
{
    INTREG         iregInterLevel;
    phys_addr_t    phyaddr;
    PLW_CLASS_TCB  ptcbCur;
    ULONG          ulTimeSave;                                          /*  ϵͳ�¼���¼                */
    INT            iSchedRet;

    if (!piVar) {
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

__wait_again:
    if (_VutexWakeIsMatch(LW_ACCESS_ONCE_PTR(INT, piVar),
                          iCompare, iDesired)) {                        /*  �Ƿ��Ѿ���������            */
        return  (ERROR_NONE);

    } else if (ulTimeout == LW_OPTION_NOT_WAIT) {                       /*  ���ȴ�                      */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  ��ʱ                        */
        return  (PX_ERROR);
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (_VutexWakeIsMatch(LW_ACCESS_ONCE_PTR(INT, piVar),
                          iCompare, iDesired)) {                        /*  �ڴ�ȷ���Ƿ��Ѿ���������    */
        __KERNEL_EXIT();
        return  (ERROR_NONE);
    }

#if LW_CFG_VMM_EN > 0
    if (__vmmLibVirtualToPhysical((addr_t)piVar, &phyaddr)) {           /*  ת��Ϊ�����ַ              */
        __KERNEL_EXIT();
        return  (PX_ERROR);
    }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
    phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  ��ַ��Ч                    */
        __KERNEL_EXIT();
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    LW_TCB_GET_CUR(ptcbCur);

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */

    ptcbCur->TCB_usStatus      |= LW_THREAD_STATUS_VUTEX;               /*  д״̬λ����ʼ�ȴ�          */
    ptcbCur->TCB_ucWaitTimeout  = LW_WAIT_TIME_CLEAR;                   /*  ��յȴ�ʱ��                */

    if (ulTimeout == LW_OPTION_WAIT_INFINITE) {                         /*  �Ƿ�������ȴ�              */
        ptcbCur->TCB_ulDelay = 0ul;
    } else {
        ptcbCur->TCB_ulDelay = ulTimeout;                               /*  ���ó�ʱʱ��                */
    }
    __KERNEL_TIME_GET_IGNIRQ(ulTimeSave, ULONG);                        /*  ��¼ϵͳʱ��                */

    _VutexWaitQueue(ptcbCur, phyaddr, iDesired, iCompare);              /*  ����ȴ���                  */

    iSchedRet = __KERNEL_EXIT_IRQ(iregInterLevel);                      /*  �˳��ں�                    */
    if (iSchedRet == LW_SIGNAL_EINTR) {
        _ErrorHandle(EINTR);                                            /*  ���źŴ��                  */
        return  (PX_ERROR);

    } else if (iSchedRet == LW_SIGNAL_RESTART) {
        ulTimeout = _sigTimeoutRecalc(ulTimeSave, ulTimeout);
        if (ulTimeout != LW_OPTION_NOT_WAIT) {
            goto    __wait_again;                                       /*  ���µȴ�                    */
        }

    } else if (ptcbCur->TCB_ucWaitTimeout == LW_WAIT_TIME_OUT) {        /*  �����ѻ�ʱ��              */
        _ErrorHandle(ERROR_THREAD_WAIT_TIMEOUT);                        /*  �ȴ���ʱ                    */
        return  (PX_ERROR);
    }

    return  (_VutexWakeIsAll(ptcbCur) ? 1 : 0);                         /*  ������                      */
}
/*********************************************************************************************************
** ��������: API_VutexPend
** ��������: �ȴ�һ����������ĳ��ֵ
** �䡡��  : piVar     �ȴ��ı�����ַ
**           iDesired  ��������ֵ
**           ulTimeout �ȴ�ʱ��
** �䡡��  : ERROR or 0: ����ʵ�ĵȴ�ֵ����, 1: �� WAKEALL ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
INT  API_VutexPend (INT  *piVar, INT  iDesired, ULONG  ulTimeout)
{
    return  (API_VutexPendEx(piVar, LW_OPTION_VUTEX_EQU, iDesired, ulTimeout));
}
/*********************************************************************************************************
** ��������: API_VutexPostEx
** ��������: �ı�һ������Ϊĳ��ֵ���ѵȴ��߳�
** �䡡��  : piVar     ������ַ
**           iValue    Ҫ���õ�ֵ
**           iFlags    flags
** �䡡��  : ERROR or ���ѵ�������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
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

    if (LW_CPU_GET_CUR_NESTING()) {                                     /*  �������ж��е���            */
        _ErrorHandle(ERROR_KERNEL_IN_ISR);
        return  (PX_ERROR);
    }

    if (LW_VUTEX_POST_CHECK_VALUE(iFlags)) {
        if (LW_ACCESS_ONCE_PTR(INT, piVar) == iValue) {                 /*  ��ȫ��������ֵ�Ѿ���ͬ      */
            return  (ERROR_NONE);
        }
    }

    __KERNEL_ENTER();                                                   /*  �����ں�                    */
    if (LW_VUTEX_POST_CHECK_VALUE(iFlags)) {
        if (LW_ACCESS_ONCE_PTR(INT, piVar) == iValue) {
            __KERNEL_EXIT();
            return  (ERROR_NONE);                                       /*  ��ȫ��������ֵ�Ѿ���ͬ      */
        }
    }

#if LW_CFG_VMM_EN > 0
    if (__vmmLibVirtualToPhysical((addr_t)piVar, &phyaddr)) {           /*  ת��Ϊ�����ַ              */
        __KERNEL_EXIT();
        return  (PX_ERROR);
    }
#else                                                                   /*  LW_CFG_VMM_EN > 0           */
    phyaddr = (phys_addr_t)piVar;
#endif                                                                  /*  LW_CFG_VMM_EN == 0          */

    if (phyaddr == LW_PHY_ADDR_INVALID) {                               /*  ��ַ��Ч                    */
        __KERNEL_EXIT();
        _ErrorHandle(EFAULT);
        return  (PX_ERROR);
    }

    if (!(iFlags & LW_OPTION_VUTEX_FLAG_DONTSET)) {                     /*  ������                      */
        LW_ACCESS_ONCE_PTR(INT, piVar) = iValue;
        KN_SMP_WMB();
    }

    iWakeCnt = _VutexWakeQueue(phyaddr, iValue, iFlags);                /*  ���Ѻ��ʵ�����              */

    __KERNEL_EXIT();                                                    /*  �˳��ں�                    */

    return  (iWakeCnt);
}
/*********************************************************************************************************
** ��������: API_VutexPost
** ��������: �ı�һ������Ϊĳ��ֵ
** �䡡��  : piVar     ������ַ
**           iValue    Ҫ���õ�ֵ
** �䡡��  : ERROR or ���ѵ�������
** ȫ�ֱ���:
** ����ģ��:
                                           API ����

                                       (�������ж��е���)
*********************************************************************************************************/
LW_API
INT  API_VutexPost (INT  *piVar, INT  iValue)
{
    return  (API_VutexPostEx(piVar, iValue, 0));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
