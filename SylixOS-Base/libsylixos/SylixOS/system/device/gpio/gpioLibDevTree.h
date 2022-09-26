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
** ��   ��   ��: gpioLibDevTree.h
**
** ��   ��   ��: Zhao.Bing (�Ա�)
**
** �ļ���������: 2021 �� 10 �� 09 ��
**
** ��        ��: GPIO ƽ̨��������ģ��.
*********************************************************************************************************/

#ifndef __GPIOLIB_DEVTREE_H
#define __GPIOLIB_DEVTREE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_GPIO_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  GPIO �������ṹ����
*********************************************************************************************************/

typedef struct lw_dt_gpio_ctrl {
    LW_GPIO_CHIP            DTGPIOCTRL_gpiochip;
    PLW_DEV_INSTANCE        DTGPIOCTRL_pdevinstance;                    /*  ����ģ���е��豸            */
    PVOID                   DTGPIOCTRL_pvPriv;                          /*  ˽������                    */

    CHAR                    DTGPIOCTRL_cName[LW_CFG_OBJECT_NAME_SIZE];  /*  ����������                  */
    UINT                    DTGPIOCTRL_uiBase;                          /*  GPIO �ŵĻ���ƫ��           */
    UINT                    DTGPIOCTRL_uiNGpios;                        /*  GPIO ����                   */
    LW_LIST_LINE_HEADER     DTGPIOCTRL_plineGpioRange;                  /*  GPIO ��Χ                   */

    ULONG                   DTGPIOCTRL_ulPad[16];                       /*  ����δ����չ                */

#define DTGPIOCTRL_pfuncRequest             DTGPIOCTRL_gpiochip.GC_pfuncRequest
#define DTGPIOCTRL_pfuncFree                DTGPIOCTRL_gpiochip.GC_pfuncFree
#define DTGPIOCTRL_pfuncGetDirection        DTGPIOCTRL_gpiochip.GC_pfuncGetDirection
#define DTGPIOCTRL_pfuncDirectionInput      DTGPIOCTRL_gpiochip.GC_pfuncDirectionInput
#define DTGPIOCTRL_pfuncGet                 DTGPIOCTRL_gpiochip.GC_pfuncGet
#define DTGPIOCTRL_pfuncDirectionOutput     DTGPIOCTRL_gpiochip.GC_pfuncDirectionOutput
#define DTGPIOCTRL_pfuncSetDebounce         DTGPIOCTRL_gpiochip.GC_pfuncSetDebounce
#define DTGPIOCTRL_pfuncSetPull             DTGPIOCTRL_gpiochip.GC_pfuncSetPull
#define DTGPIOCTRL_pfuncSet                 DTGPIOCTRL_gpiochip.GC_pfuncSet
#define DTGPIOCTRL_pfuncGetIrq              DTGPIOCTRL_gpiochip.GC_pfuncGetIrq
#define DTGPIOCTRL_pfuncSetupIrq            DTGPIOCTRL_gpiochip.GC_pfuncSetupIrq
#define DTGPIOCTRL_pfuncClearIrq            DTGPIOCTRL_gpiochip.GC_pfuncClearIrq
#define DTGPIOCTRL_pfuncSvrIrq              DTGPIOCTRL_gpiochip.GC_pfuncSvrIrq

} LW_DT_GPIO_CTRL;
typedef LW_DT_GPIO_CTRL    *PLW_DT_GPIO_CTRL;

/*********************************************************************************************************
  GPIO ���ŷ�Χ����ṹ����
*********************************************************************************************************/

typedef struct lw_gpio_pin_range {
    LW_LIST_LINE                GPRANGE_lineManage;                     /*  ���ſ���������              */
    LW_PINCTRL_GPIO_RANGE       GPRANGE_gpioRange;                      /*  GPIO ��Χ                   */
    PLW_PINCTRL_DEV             GPRANGE_ppinctldev;                     /*  PINCTRL ������              */
} LW_GPIO_PIN_RANGE;
typedef LW_GPIO_PIN_RANGE  *PLW_GPIO_PIN_RANGE;

/*********************************************************************************************************
  ����ӿں���
*********************************************************************************************************/

LW_API INT              API_GpioCtrlRegister(PLW_DT_GPIO_CTRL  pdtgpioctrl, CPCHAR  pcName);

LW_API VOID             API_GpioCtrlUnregister(PLW_DT_GPIO_CTRL  pdtgpioctrl);

#endif                                                                  /*  (LW_CFG_GPIO_EN > 0) &&     */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __GPIOLIB_DEVTREE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
