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
** 文   件   名: gpioLibDevTree.c
**
** 创   建   人: Zhao.Bing (赵冰)
**
** 文件创建日期: 2021 年 10 月 09 日
**
** 描        述: GPIO 平台总线驱动模型.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_GPIO_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
** 函数名称: API_GpioCtrlRegister
** 功能描述: GPIO 控制器注册
** 输　入  : pdtgpioctrl      GPIO 控制器指针
**           pcName           GPIO 控制器名称
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_GpioCtrlRegister (PLW_DT_GPIO_CTRL  pdtgpioctrl, CPCHAR  pcName)
{
    PLW_GPIO_CHIP   pgchip;

    if (!pdtgpioctrl || !pcName) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_strlcpy(pdtgpioctrl->DTGPIOCTRL_cName, pcName, LW_CFG_OBJECT_NAME_SIZE);

    pgchip = &pdtgpioctrl->DTGPIOCTRL_gpiochip;

    pgchip->GC_pcLabel    = pdtgpioctrl->DTGPIOCTRL_cName;
    pgchip->GC_ulVerMagic = LW_GPIO_VER_MAGIC;
    pgchip->GC_uiBase     = pdtgpioctrl->DTGPIOCTRL_uiBase;
    pgchip->GC_uiNGpios   = pdtgpioctrl->DTGPIOCTRL_uiNGpios;

    if (API_GpioChipAdd(pgchip) != ERROR_NONE) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GpioCtrlUnregister
** 功能描述: 卸载 GPIO 控制器
** 输　入  : pdtgpioctrl      GPIO 控制器指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_GpioCtrlUnregister (PLW_DT_GPIO_CTRL  pdtgpioctrl)
{
    if (!pdtgpioctrl) {
        _ErrorHandle(EINVAL);
        return;
    }

    API_DeviceTreeGpioPinRangeRemove(pdtgpioctrl);
    API_GpioChipDelete(&pdtgpioctrl->DTGPIOCTRL_gpiochip);
}

#endif                                                                  /*  (LW_CFG_GPIO_EN > 0) &&     */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
