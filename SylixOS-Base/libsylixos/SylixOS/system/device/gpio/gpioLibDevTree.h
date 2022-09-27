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
** 文   件   名: gpioLibDevTree.h
**
** 创   建   人: Zhao.Bing (赵冰)
**
** 文件创建日期: 2021 年 10 月 09 日
**
** 描        述: GPIO 平台总线驱动模型.
*********************************************************************************************************/

#ifndef __GPIOLIB_DEVTREE_H
#define __GPIOLIB_DEVTREE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_GPIO_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  GPIO 控制器结构定义
*********************************************************************************************************/

typedef struct lw_dt_gpio_ctrl {
    LW_GPIO_CHIP            DTGPIOCTRL_gpiochip;
    PLW_DEV_INSTANCE        DTGPIOCTRL_pdevinstance;                    /*  驱动模型中的设备            */
    PVOID                   DTGPIOCTRL_pvPriv;                          /*  私有数据                    */

    CHAR                    DTGPIOCTRL_cName[LW_CFG_OBJECT_NAME_SIZE];  /*  控制器名字                  */
    UINT                    DTGPIOCTRL_uiBase;                          /*  GPIO 号的基础偏移           */
    UINT                    DTGPIOCTRL_uiNGpios;                        /*  GPIO 数量                   */
    LW_LIST_LINE_HEADER     DTGPIOCTRL_plineGpioRange;                  /*  GPIO 范围                   */

    ULONG                   DTGPIOCTRL_ulPad[16];                       /*  保留未来扩展                */

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
  GPIO 引脚范围管理结构定义
*********************************************************************************************************/

typedef struct lw_gpio_pin_range {
    LW_LIST_LINE                GPRANGE_lineManage;                     /*  引脚控制器链表              */
    LW_PINCTRL_GPIO_RANGE       GPRANGE_gpioRange;                      /*  GPIO 范围                   */
    PLW_PINCTRL_DEV             GPRANGE_ppinctldev;                     /*  PINCTRL 控制器              */
} LW_GPIO_PIN_RANGE;
typedef LW_GPIO_PIN_RANGE  *PLW_GPIO_PIN_RANGE;

/*********************************************************************************************************
  对外接口函数
*********************************************************************************************************/

LW_API INT              API_GpioCtrlRegister(PLW_DT_GPIO_CTRL  pdtgpioctrl, CPCHAR  pcName);

LW_API VOID             API_GpioCtrlUnregister(PLW_DT_GPIO_CTRL  pdtgpioctrl);

#endif                                                                  /*  (LW_CFG_GPIO_EN > 0) &&     */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __GPIOLIB_DEVTREE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
