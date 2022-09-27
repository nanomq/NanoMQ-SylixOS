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
** 文   件   名: pinConfig.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 12 日
**
** 描        述: pinctrl 子系统中的引脚配置实现
*********************************************************************************************************/

#ifndef __PINCONFIG_H
#define __PINCONFIG_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  引脚配置参数
*********************************************************************************************************/

typedef enum pin_config_param {
    PIN_CONFIG_BIAS_BUS_HOLD,
    PIN_CONFIG_BIAS_DISABLE,
    PIN_CONFIG_BIAS_HIGH_IMPEDANCE,
    PIN_CONFIG_BIAS_PULL_DOWN,
    PIN_CONFIG_BIAS_PULL_PIN_DEFAULT,
    PIN_CONFIG_BIAS_PULL_UP,
    PIN_CONFIG_DRIVE_OPEN_DRAIN,
    PIN_CONFIG_DRIVE_OPEN_SOURCE,
    PIN_CONFIG_DRIVE_PUSH_PULL,
    PIN_CONFIG_DRIVE_STRENGTH,
    PIN_CONFIG_INPUT_DEBOUNCE,
    PIN_CONFIG_INPUT_ENABLE,
    PIN_CONFIG_INPUT_SCHMITT,
    PIN_CONFIG_INPUT_SCHMITT_ENABLE,
    PIN_CONFIG_LOW_POWER_MODE,
    PIN_CONFIG_OUTPUT_ENABLE,
    PIN_CONFIG_OUTPUT,
    PIN_CONFIG_POWER_SOURCE,
    PIN_CONFIG_SLEEP_HARDWARE_STATE,
    PIN_CONFIG_SLEW_RATE,
    PIN_CONFIG_SKEW_DELAY,
    PIN_CONFIG_PERSIST_STATE,
    PIN_CONFIG_END = 0x7F,
    PIN_CONFIG_MAX = 0xFF,
} PIN_CONFIG_PARAM;

/*********************************************************************************************************
  引脚配置与 CONFIG 变量转换

  +--------------------------+--------+
  | 31                     8 | 7    0 |
  +--------+--------+--------+--------+
  |         Argument         |  Param |
  +--------+--------+--------+--------+

*********************************************************************************************************/

#define PINCONF_TO_CONFIG_PARAM(ulConfig)      (PIN_CONFIG_PARAM)(ulConfig & 0xfful)
#define PINCONF_TO_CONFIG_ARGUMENT(ulConfig)   (UINT32)((ulConfig >> 8) & 0xfffffful)
#define PINCONF_PACKED(pin, conf)              (((conf) << 8) | ((ULONG)pin & 0xfful))

/*********************************************************************************************************
  引脚配置接口实现
*********************************************************************************************************/

LW_API INT   API_PinConfigApply(PLW_PINCTRL_SETTING  ppinctrlsetting);

LW_API INT   API_PinConfigMapToSetting(PLW_PINCTRL_MAP      ppinctrlmap,
                                       PLW_PINCTRL_SETTING  ppinctrlsetting);

LW_API INT   API_PinConfigMapValidate(PLW_PINCTRL_MAP  ppinctrlmap, INT  i);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PINCONFIG_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
