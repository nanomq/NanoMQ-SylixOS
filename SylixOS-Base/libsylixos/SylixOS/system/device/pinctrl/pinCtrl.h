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
** ��   ��   ��: pinCtrl.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 12 ��
**
** ��        ��: pinctrl ��ϵͳ
*********************************************************************************************************/

#ifndef __PINCTRL_H
#define __PINCTRL_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  ���Ժ궨��
*********************************************************************************************************/

#define PCTL_LOG_LOG    __LOGMESSAGE_LEVEL
#define PCTL_LOG_BUG    __BUGMESSAGE_LEVEL
#define PCTL_LOG_ERR    __ERRORMESSAGE_LEVEL
#define PCTL_LOG_ALL    __PRINTMESSAGE_LEVEL

#define PCTL_LOG        _DebugFormat

/*********************************************************************************************************
  ����ͷ�ļ�
*********************************************************************************************************/

#include "pinCtrlClass.h"
#include "pinCtrlDev.h"
#include "pinConfig.h"
#include "pinMux.h"

/*********************************************************************************************************
  �ṹ������
*********************************************************************************************************/

struct lw_pinctrl;
struct lw_pinctrl_state;
struct lw_device_pin_info;

/*********************************************************************************************************
  ����״̬�궨��
*********************************************************************************************************/

#define PINCTRL_STATE_DEFAULT "default"
#define PINCTRL_STATE_INIT    "init"
#define PINCTRL_STATE_IDLE    "idle"
#define PINCTRL_STATE_SLEEP   "sleep"

/*********************************************************************************************************
  ����״̬ lw_pinctrl_state
*********************************************************************************************************/

typedef struct lw_pinctrl_state {
    LW_LIST_LINE               PCTLS_lineManage;                        /*  ����״̬����                */
    LW_LIST_LINE_HEADER        PCTLS_plineSettings;                     /*  ����״̬����������          */
    CPCHAR                     PCTLS_pcName;                            /*  ����״̬����                */
} LW_PINCTRL_STATE;
typedef LW_PINCTRL_STATE      *PLW_PINCTRL_STATE;

/*********************************************************************************************************
  ���ſ��� lw_pinctrl
*********************************************************************************************************/

typedef struct lw_pinctrl {
    LW_LIST_LINE               PCTL_lineManage;                         /*  ���ſ��ƹ���                */
    LW_LIST_LINE_HEADER        PCTL_plineStates;                        /*  ����״̬����                */
    LW_LIST_LINE_HEADER        PCTL_plinemaps;                          /*  ���豸����������ӳ���      */
    struct lw_pinctrl_state   *PCTL_ppctlstate;                         /*  ��ǰ������״̬              */
    PLW_DEVTREE_NODE           PCTL_pdtnDev;                            /*  ���ſ��ƹ������豸���ڵ�    */
} LW_PINCTRL;
typedef LW_PINCTRL            *PLW_PINCTRL;

/*********************************************************************************************************
  �豸������Ϣ lw_device_pin_info
*********************************************************************************************************/

typedef struct lw_device_pin_info {
    PLW_PINCTRL                DEVPIN_ppinctl;
    PLW_PINCTRL_STATE          DEVPIN_ppctlstateDefault;
    PLW_PINCTRL_STATE          DEVPIN_ppctlstateInit;
    PLW_PINCTRL_STATE          DEVPIN_ppctlstateSleep;
    PLW_PINCTRL_STATE          DEVPIN_ppctlstateIdle;
} LW_DEV_PIN_INFO;
typedef LW_DEV_PIN_INFO       *PLW_DEV_PIN_INFO;

/*********************************************************************************************************
  ���ſ��ƽӿ�
*********************************************************************************************************/

LW_API INT                API_PinCtrlPinGetByName(PLW_PINCTRL_DEV  ppinctldev, CPCHAR  pcName);

LW_API INT                API_PinCtrlGroupSelectorGet(PLW_PINCTRL_DEV  ppinctldev, CPCHAR  pcPinGroup);

LW_API PLW_PINCTRL_STATE  API_PinCtrlStateFind(PLW_PINCTRL  ppinctl, CPCHAR  pcName);

LW_API INT                API_PinCtrlStateSelect(PLW_PINCTRL  ppinctl, PLW_PINCTRL_STATE  ppinctlstate);

LW_API VOID               API_PinCtrlMapDel(PLW_PINCTRL_MAP  ppinctlmap);

LW_API INT                API_PinCtrlMapAdd(PLW_PINCTRL_MAPS  ppinctlmaps);

LW_API PLW_PINCTRL        API_PinCtrlGet(PLW_DEVTREE_NODE  pdtnDev);

LW_API INT                API_PinBind(PLW_DEV_INSTANCE  pdevinstance);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PINCTRL_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
