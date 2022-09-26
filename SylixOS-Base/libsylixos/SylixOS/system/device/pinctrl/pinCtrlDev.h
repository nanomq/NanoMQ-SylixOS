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
** 文   件   名: pinCtrlDev.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 02 日
**
** 描        述: pinctrl 控制器接口实现
*********************************************************************************************************/

#ifndef __PINCTRLDEV_H
#define __PINCTRLDEV_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

LW_API PLW_PIN_DESC      API_PinCtrlPinDescGet(PLW_PINCTRL_DEV  ppinctldev, UINT  uiPin);

LW_API VOID              API_PinCtrlPinDescDel(PLW_PINCTRL_DEV  ppinctldev, PLW_PIN_DESC  pindesc);

LW_API PLW_PINCTRL_DEV   API_PinCtrlDevGetByDevtreeNode(PLW_DEVTREE_NODE  pdtnDev);

LW_API PLW_PINCTRL_DESC  API_PinCtrlDescBuild(PCHAR                 pcName,
                                              PLW_PINCTRL_OPS       ppinctlops,
                                              PLW_PINMUX_OPS        ppinmuxops,
                                              PLW_PINCONF_OPS       ppinconfops,
                                              PLW_PINCTRL_PIN_DESC  pctlpindescs,
                                              INT                   uiPinsNum);

LW_API PLW_PINCTRL_DEV   API_PinCtrlDevCreate(PLW_PINCTRL_DESC  ppinctldesc,
                                              PVOID             pvData,
                                              PLW_DEVTREE_NODE  pdtnDev);

LW_API INT               API_PinCtrlGpioRangeAdd(PLW_PINCTRL_DEV            ppinctrldev,
                                                 PLW_PINCTRL_GPIO_RANGE     ppgpiorange);

LW_API INT               API_PinCtrlGpioRangeRemove(PLW_PINCTRL_DEV         ppinctrldev,
                                                    PLW_PINCTRL_GPIO_RANGE  ppgpiorange);

LW_API INT               API_PinCtrlGpioRequest(UINT  uiGpio);

LW_API VOID              API_PinCtrlGpioFree(UINT  uiGpio);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PINCTRLDEV_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
