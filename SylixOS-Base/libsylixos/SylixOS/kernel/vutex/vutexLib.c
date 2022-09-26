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
** ��   ��   ��: vutexLib.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2020 �� 12 �� 26 ��
**
** ��        ��: �ȴ��������ڲ�ʵ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define LW_VUTEX_HASH_SIZE      1024
#define LW_VUTEX_HASH_MASK      0x3ff
#define LW_VUTEX_HASH_INDEX(a)  _VutexHash(a)
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER   _k_plineVutexHashHeader[LW_VUTEX_HASH_SIZE];
static PLW_LIST_LINE         _k_plineVutexOp;
/*********************************************************************************************************
** ��������: _VutexHash
** ��������: ���� vutex hash ֵ
** �䡡��  : phyaddr   �����ַ
** �䡡��  : hash index
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_INLINE static UINT32  _VutexHash (phys_addr_t  phyaddr)
{
             UINT32  uiTemp  = (UINT32)(phyaddr >> 2);
    REGISTER UINT8  *pucTemp = (UINT8 *)&uiTemp;

    return  ((pucTemp[0] + pucTemp[1] + pucTemp[2] + pucTemp[3]) & LW_VUTEX_HASH_MASK);
}
/*********************************************************************************************************
** ��������: _VutexWakeIsMatch
** ��������: �ж������Ƿ�ƥ��
** �䡡��  : iValue    ��ʵ��ֵ
**           iCompare  �ȽϷ���
**           iDesired  ������ֵ
** �䡡��  : �Ƿ�ƥ��
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: _VutexWakeIsAll
** ��������: ��û��������Ƿ�Ϊ WAKEALL
** �䡡��  : ptcb      ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  _VutexWakeIsAll (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    return  (pvutex->VUTEX_bWakeAll);
}
/*********************************************************************************************************
** ��������: _VutexWaitQueue
** ��������: ����ǰ�������ȴ����� (�����ں��ҹ��ж����������)
** �䡡��  : ptcbCur   ��ǰ����
**           phyaddr   �ȴ��������ַ
**           iDesired  ������ֵ
**           iCompare  �ȽϷ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexWaitQueue (PLW_CLASS_TCB  ptcbCur, phys_addr_t  phyaddr, INT32  iDesired, INT  iCompare)
{
    REGISTER PLW_CLASS_PCB      ppcb;
             PLW_VUTEX_CONTEXT  pvutex = &ptcbCur->TCB_vutex;

    pvutex->VUTEX_phyaddr = phyaddr;
    ppcb = _GetPcb(ptcbCur);
    __DEL_FROM_READY_RING(ptcbCur, ppcb);                               /*  �Ӿ���������ɾ��            */

    pvutex->VUTEX_bWakeAll = LW_FALSE;
    pvutex->VUTEX_iCompare = iCompare;
    pvutex->VUTEX_iDesired = iDesired;
    pvutex->VUTEX_uiHash   = LW_VUTEX_HASH_INDEX(phyaddr);
    _List_Line_Add_Ahead(&pvutex->VUTEX_lineWait,                       /*  ����ȴ���������            */
                         &_k_plineVutexHashHeader[pvutex->VUTEX_uiHash]);

    if (ptcbCur->TCB_ulDelay) {
        __ADD_TO_WAKEUP_LINE(ptcbCur);                                  /*  ���볬ʱɨ����              */
    }
}
/*********************************************************************************************************
** ��������: _VutexWaitQueue
** ��������: ���ݵ�ַ���������ݻ���Ŀ������ (�����ں����������)
** �䡡��  : phyaddr   �����ַ
**           iValue    д���ֵ
**           iFlags    ����ѡ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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

    iregInterLevel = KN_INT_DISABLE();                                  /*  �ر��ж�                    */

    _k_plineVutexOp = _k_plineVutexHashHeader[uiHash];
    while (_k_plineVutexOp) {
        plineTemp       = _k_plineVutexOp;
        _k_plineVutexOp = _list_line_get_next(plineTemp);               /*  ��һ���ȴ��ڵ�              */

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
                           &_k_plineVutexHashHeader[uiHash]);           /*  �˳��ȴ���������            */
            pvutex->VUTEX_phyaddr  = LW_PHY_ADDR_INVALID;               /*  ɾ���ȴ���Ϣ                */
            pvutex->VUTEX_bWakeAll = (iFlags & LW_OPTION_VUTEX_FLAG_WAKEALL)
                                   ? LW_TRUE : LW_FALSE;

            ptcb = _LIST_ENTRY(pvutex, LW_CLASS_TCB, TCB_vutex);
            if (ptcb->TCB_usStatus & LW_THREAD_STATUS_DELAY) {
                __DEL_FROM_WAKEUP_LINE(ptcb);                           /*  �˳���ʱ����                */
                ptcb->TCB_ulDelay = 0ul;
            }

            if (ptcb->TCB_ucWaitTimeout) {
                ptcb->TCB_ucWaitTimeout = LW_WAIT_TIME_CLEAR;           /*  �����ʱλ                  */
            } else {
                ptcb->TCB_usStatus = (UINT16)(ptcb->TCB_usStatus & ~LW_THREAD_STATUS_VUTEX);
                if (__LW_THREAD_IS_READY(ptcb)) {                       /*  �Ƿ����                    */
                    ptcb->TCB_ucSchedActivate = LW_SCHED_ACT_INTERRUPT; /*  ���ȼ��ʽ                */
                    ppcb = _GetPcb(ptcb);
                    __ADD_TO_READY_RING(ptcb, ppcb);                    /*  ���뵽������ȼ�������      */
                }
            }

            KN_INT_ENABLE(iregInterLevel);                              /*  ���ж�                    */

            iWakeCnt++;

            iregInterLevel = KN_INT_DISABLE();                          /*  �ر��ж�                    */
        }
    }

    KN_INT_ENABLE(iregInterLevel);                                      /*  ���ж�                    */

    return  (iWakeCnt);
}
/*********************************************************************************************************
** ��������: _VutexInitCtx
** ��������: ��ʼ��������ƿ� vutex ������
** �䡡��  : ptcb      ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexInitCtx (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    pvutex->VUTEX_phyaddr = LW_PHY_ADDR_INVALID;
    _LIST_LINE_INIT_IN_CODE(pvutex->VUTEX_lineWait);
}
/*********************************************************************************************************
** ��������: _VutexUnQueue
** ��������: ��Ŀ�������˳��ȴ����� (�����ں��ҹ��ж����������)
** �䡡��  : ptcb      Ŀ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  _VutexUnQueue (PLW_CLASS_TCB  ptcb)
{
    PLW_VUTEX_CONTEXT  pvutex = &ptcb->TCB_vutex;

    if (_k_plineVutexOp == &pvutex->VUTEX_lineWait) {
        _k_plineVutexOp =  _list_line_get_next(_k_plineVutexOp);        /*  �ƶ�����һ���ڵ�            */
    }

    _List_Line_Del(&pvutex->VUTEX_lineWait,
                   &_k_plineVutexHashHeader[pvutex->VUTEX_uiHash]);     /*  �˳��ȴ���������            */

    pvutex->VUTEX_uiHash  = 0;
    pvutex->VUTEX_phyaddr = LW_PHY_ADDR_INVALID;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
