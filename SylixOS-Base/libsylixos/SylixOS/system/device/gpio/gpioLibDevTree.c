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
** ��   ��   ��: gpioLibDevTree.c
**
** ��   ��   ��: Zhao.Bing (�Ա�)
**
** �ļ���������: 2021 �� 10 �� 09 ��
**
** ��        ��: GPIO ƽ̨��������ģ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_GPIO_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
** ��������: API_GpioCtrlRegister
** ��������: GPIO ������ע��
** �䡡��  : pdtgpioctrl      GPIO ������ָ��
**           pcName           GPIO ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_GpioCtrlUnregister
** ��������: ж�� GPIO ������
** �䡡��  : pdtgpioctrl      GPIO ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
