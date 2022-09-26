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
** ��   ��   ��: ThreadOnce.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2009 �� 02 �� 03 ��
**
** ��        ��: ����ϵͳ�� pthread_once ֧��.

** BUG:
2010.01.09  ʹ�� ATOMIC �����л���.
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
** ��������: __threadOnceCleanPush
** ��������: ��װһ������ִ�еĳ�ʼ������.
** �䡡��  : piOnce            once ����.
** �䡡��  : TRUE or FALSE
** ȫ�ֱ���:
** ����ģ��:
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

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */

    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;

    _LIST_MONO_LINK(&pclean->CUO_monoNext, ptex->TEX_pmonoOnceHeader);
    ptex->TEX_pmonoOnceHeader = &pclean->CUO_monoNext;

    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __threadOnceCleanPop
** ��������: �̰߳�ȫ�Ľ�ִ��һ��ָ������.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __threadOnceCleanPop (VOID)
{
    INTREG              iregInterLevel;
    PLW_CLASS_TCB       ptcbCur;
    __PLW_CLEANUP_ONCE  pclean;
    __PLW_THREAD_EXT    ptex;

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */

    LW_TCB_GET_CUR(ptcbCur);
    ptex = &ptcbCur->TCB_texExt;

    pclean = (__PLW_CLEANUP_ONCE)ptex->TEX_pmonoOnceHeader;
    if (pclean) {
        _list_mono_next(&ptex->TEX_pmonoOnceHeader);
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
        __KHEAP_FREE(pclean);

    } else {
        KN_INT_ENABLE(iregInterLevel);                                  /*  ���ж�                    */
    }
}
/*********************************************************************************************************
** ��������: API_ThreadOnce
** ��������: �̰߳�ȫ�Ľ�ִ��һ��ָ������.
** �䡡��  : piOnce            �����ʼ��Ϊ 0.
**           pfuncRoutine      ��Ҫִ�еĺ���.
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
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

        case __THREAD_ONCE_STATUS_DOWN:                                 /*  �Ѿ�ִ�й���                */
            return  (ERROR_NONE);

        case __THREAD_ONCE_STATUS_INIT:                                 /*  ���ڱ���������ִ��          */
            API_VutexPend(piOnce,
                          __THREAD_ONCE_STATUS_DOWN,
                          LW_OPTION_WAIT_INFINITE);                     /*  �ȴ���ʼ��ִ�����          */
            continue;

        case __THREAD_ONCE_STATUS_NONE:                                 /*  ���Գ��Գ�ʼ��              */
            iValue = __LW_ATOMIC_CAS(patomic,
                                     __THREAD_ONCE_STATUS_NONE,
                                     __THREAD_ONCE_STATUS_INIT);
            break;

        default:                                                        /*  ״̬����                    */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    } while (iValue != __THREAD_ONCE_STATUS_NONE);                      /*  CAS ʧ��                    */

    bPush = __threadOnceCleanPush(piOnce);                              /*  �����;�˳����յ�          */

    LW_SOFUNC_PREPARE(pfuncRoutine);
    pfuncRoutine();                                                     /*  ִ��                        */

    if (bPush) {
        __threadOnceCleanPop();                                         /*  ɾ�����յ�                  */
    }
    
    API_VutexPostEx(piOnce,
                    __THREAD_ONCE_STATUS_DOWN,
                    LW_OPTION_VUTEX_FLAG_WAKEALL);                      /*  ִ�����, ���ѵȴ�������    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ThreadOnce2
** ��������: �̰߳�ȫ�Ľ�ִ��һ��ָ������.
** �䡡��  : piOnce            �����ʼ��Ϊ 0.
**           pfuncRoutine      ��Ҫִ�еĺ���.
**           pvArg             ִ�в���
** �䡡��  : ERROR_NONE
** ȫ�ֱ���: 
** ����ģ��: 
                                           API ����
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

        case __THREAD_ONCE_STATUS_DOWN:                                 /*  �Ѿ�ִ�й���                */
            return  (ERROR_NONE);

        case __THREAD_ONCE_STATUS_INIT:                                 /*  ���ڱ���������ִ��          */
            API_VutexPend(piOnce,
                          __THREAD_ONCE_STATUS_DOWN,
                          LW_OPTION_WAIT_INFINITE);                     /*  �ȴ���ʼ��ִ�����          */
            continue;

        case __THREAD_ONCE_STATUS_NONE:                                 /*  ���Գ��Գ�ʼ��              */
            iValue = __LW_ATOMIC_CAS(patomic,
                                     __THREAD_ONCE_STATUS_NONE,
                                     __THREAD_ONCE_STATUS_INIT);
            break;

        default:                                                        /*  ״̬����                    */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    } while (iValue != __THREAD_ONCE_STATUS_NONE);                      /*  CAS ʧ��                    */

    bPush = __threadOnceCleanPush(piOnce);                              /*  �����;�˳����յ�          */

    LW_SOFUNC_PREPARE(pfuncRoutine);
    pfuncRoutine(pvArg);                                                /*  ִ��                        */

    if (bPush) {
        __threadOnceCleanPop();                                         /*  ɾ�����յ�                  */
    }
    
    API_VutexPostEx(piOnce,
                    __THREAD_ONCE_STATUS_DOWN,
                    LW_OPTION_VUTEX_FLAG_WAKEALL);                      /*  ִ�����, ���ѵȴ�������    */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_THREAD_EXT_EN > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
