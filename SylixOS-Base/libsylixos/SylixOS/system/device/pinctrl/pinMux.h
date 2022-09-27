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
** ��   ��   ��: pinMux.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 02 ��
**
** ��        ��: ���Ÿ�������
*********************************************************************************************************/

#ifndef __PINMUX_H
#define __PINMUX_H

/*********************************************************************************************************
  �ü���
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
