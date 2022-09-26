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
** 文   件   名: pinMux.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 02 日
**
** 描        述: 引脚复用配置
*********************************************************************************************************/

#ifndef __PINMUX_H
#define __PINMUX_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

LW_API INT   API_PinMuxEnable(PLW_PINCTRL_SETTING  ppinctlsetting);

LW_API VOID  API_PinMuxDisable(PLW_PINCTRL_SETTING  ppinctlsetting);

LW_API INT   API_PinMuxMapToSetting(PLW_PINCTRL_MAP  ppinctlmap, PLW_PINCTRL_SETTING  ppinctlsetting);

LW_API INT   API_PinMuxMapValidate(PLW_PINCTRL_MAP  ppinctlmap, INT  i);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PINMUX_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
