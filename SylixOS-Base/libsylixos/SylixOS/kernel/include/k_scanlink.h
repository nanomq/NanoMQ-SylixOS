/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k_scanlink.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2010 �� 01 �� 22 ��
**
** ��        ��: ����ϵͳ����ɨ����(��ʱ���� & ���Ź���)��
*********************************************************************************************************/

#ifndef __K_SCANLINK_H
#define __K_SCANLINK_H

/*********************************************************************************************************
  ��ȡ���Ѷ��е�һ���ڵ�
*********************************************************************************************************/

#define __WAKEUP_GET_FIRST(pwu, pwun) \
        if ((pwu)->WU_plineHeader) { \
            (pwun) = _LIST_ENTRY((pwu)->WU_plineHeader, LW_CLASS_WAKEUP_NODE, WUN_lineManage); \
        } else { \
            (pwun) = LW_NULL; \
        }

/*********************************************************************************************************
  ���Ѷ���ɨ��
*********************************************************************************************************/

#define __WAKEUP_PASS_FIRST(pwu, pwun, ulCounter) \
        (pwu)->WU_plineOp = (pwu)->WU_plineHeader;  \
        while ((pwu)->WU_plineOp) {    \
            (pwun) = _LIST_ENTRY((pwu)->WU_plineOp, LW_CLASS_WAKEUP_NODE, WUN_lineManage);  \
            (pwu)->WU_plineOp = _list_line_get_next((pwu)->WU_plineOp); \
            if ((pwun)->WUN_ulCounter > ulCounter) {    \
                (pwun)->WUN_ulCounter -= ulCounter; \
                break;  \
            } else {    \
                ulCounter -= (pwun)->WUN_ulCounter; \
                (pwun)->WUN_ulCounter = 0;
                    
#define __WAKEUP_PASS_SECOND() \
            }
            
#define __WAKEUP_PASS_END() \
        }
        
/*********************************************************************************************************
  ��ʼ��
*********************************************************************************************************/

#define __WAKEUP_INIT(pwu, pfuncWakeup, pvArg)   \
        do {    \
            (pwu)->WU_plineHeader = LW_NULL;     \
            (pwu)->WU_plineOp     = LW_NULL;     \
            (pwu)->WU_pfuncWakeup = pfuncWakeup; \
            (pwu)->WU_pvWakeupArg = pvArg;       \
        } while (0)
        
#define __WAKEUP_NODE_INIT(pwun)    \
        do {    \
            _LIST_LINE_INIT_IN_CODE((pwun)->WUN_lineManage);    \
            (pwun)->WUN_bInQ = LW_FALSE; \
            (pwun)->WUN_ulCounter = 0ul;    \
        } while (0)
        
/*********************************************************************************************************
  ���̼߳��볬ʱ���Ѷ���
*********************************************************************************************************/

#define __ADD_TO_WAKEUP_LINE(ptcb)                                  \
        do {                                                        \
            ptcb->TCB_usStatus |= LW_THREAD_STATUS_DELAY;           \
            _WakeupAdd(&_K_wuDelay, &ptcb->TCB_wunDelay, LW_FALSE); \
        } while (0)
        
/*********************************************************************************************************
  ���̴߳ӳ�ʱ���Ѷ����˳�
*********************************************************************************************************/

#define __DEL_FROM_WAKEUP_LINE(ptcb)                                \
        do {                                                        \
            ptcb->TCB_usStatus &= ~LW_THREAD_STATUS_DELAY;          \
            _WakeupDel(&_K_wuDelay, &ptcb->TCB_wunDelay, LW_FALSE); \
        } while (0)
        
/*********************************************************************************************************
  ���̼߳��뿴�Ź�����
*********************************************************************************************************/

#define __ADD_TO_WATCHDOG_LINE(ptcb)        _WakeupAdd(&_K_wuWatchDog, &ptcb->TCB_wunWatchDog, LW_FALSE)

/*********************************************************************************************************
  ���̴߳ӿ��Ź������˳�
*********************************************************************************************************/

#define __DEL_FROM_WATCHDOG_LINE(ptcb)      _WakeupDel(&_K_wuWatchDog, &ptcb->TCB_wunWatchDog, LW_FALSE)

#endif                                                                  /*  __K_SCANLINK_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/