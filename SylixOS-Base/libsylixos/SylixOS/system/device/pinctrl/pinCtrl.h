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
** 文   件   名: pinCtrl.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 12 日
**
** 描        述: pinctrl 子系统
*********************************************************************************************************/

#ifndef __PINCTRL_H
#define __PINCTRL_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  调试宏定义
*********************************************************************************************************/

#define PCTL_LOG_LOG    __LOGMESSAGE_LEVEL
#define PCTL_LOG_BUG    __BUGMESSAGE_LEVEL
#define PCTL_LOG_ERR    __ERRORMESSAGE_LEVEL
#define PCTL_LOG_ALL    __PRINTMESSAGE_LEVEL

#define PCTL_LOG        _DebugFormat

/*********************************************************************************************************
  引用头文件
*********************************************************************************************************/

#include "pinCtrlClass.h"
#include "pinCtrlDev.h"
#include "pinConfig.h"
#include "pinMux.h"

/*********************************************************************************************************
  结构体声明
*********************************************************************************************************/

struct lw_pinctrl;
struct lw_pinctrl_state;
struct lw_device_pin_info;

/*********************************************************************************************************
  引脚状态宏定义
*********************************************************************************************************/

#define PINCTRL_STATE_DEFAULT "default"
#define PINCTRL_STATE_INIT    "init"
#define PINCTRL_STATE_IDLE    "idle"
#define PINCTRL_STATE_SLEEP   "sleep"

/*********************************************************************************************************
  引脚状态 lw_pinctrl_state
*********************************************************************************************************/

typedef struct lw_pinctrl_state {
    LW_LIST_LINE               PCTLS_lineManage;                        /*  引脚状态链表                */
    LW_LIST_LINE_HEADER        PCTLS_plineSettings;                     /*  引脚状态的配置链表          */
    CPCHAR                     PCTLS_pcName;                            /*  引脚状态名称                */
} LW_PINCTRL_STATE;
typedef LW_PINCTRL_STATE      *PLW_PINCTRL_STATE;

/*********************************************************************************************************
  引脚控制 lw_pinctrl
*********************************************************************************************************/

typedef struct lw_pinctrl {
    LW_LIST_LINE               PCTL_lineManage;                         /*  引脚控制管理                */
    LW_LIST_LINE_HEADER        PCTL_plineStates;                        /*  引脚状态链表                */
    LW_LIST_LINE_HEADER        PCTL_plinemaps;                          /*  从设备树解析出的映射表      */
    struct lw_pinctrl_state   *PCTL_ppctlstate;                         /*  当前的引脚状态              */
    PLW_DEVTREE_NODE           PCTL_pdtnDev;                            /*  引脚控制关联的设备树节点    */
} LW_PINCTRL;
typedef LW_PINCTRL            *PLW_PINCTRL;

/*********************************************************************************************************
  设备引脚信息 lw_device_pin_info
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
  引脚控制接口
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
