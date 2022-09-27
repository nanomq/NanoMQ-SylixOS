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
** ��   ��   ��: devtreeGpio.c
**
** ��   ��   ��: Zhao.Bing (�Ա�)
**
** �ļ���������: 2021 �� 11 �� 04 ��
**
** ��        ��: �豸���ӿ� GPIO ��ؽӿ�ʵ��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_GPIO_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree.h"
/*********************************************************************************************************
** ��������: __gpioDevtreeNodeMatch
** ��������: �Ƚ� GPIO ���������豸���ڵ��봫��ڵ��Ƿ�һ��
** �䡡��  : pgchip            GPIO ������
**           pvData            ���Ƚϵ��豸���ڵ�
** �䡡��  : LW_TRUE  һ��, LW_FALSE  ��һ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __gpioDevtreeNodeMatch (PLW_GPIO_CHIP  pgchip, PVOID  pvData)
{
    PLW_DT_GPIO_CTRL   pdtgpioctrl = (PLW_DT_GPIO_CTRL)pgchip;
    PLW_DEVTREE_NODE   pdtnDev     = pdtgpioctrl->DTGPIOCTRL_pdevinstance->DEVHD_pdtnDev;
    PLW_DEVTREE_NODE   pdtnCmpNode = (PLW_DEVTREE_NODE)pvData;

    return  (pdtnDev == pdtnCmpNode);
}
/*********************************************************************************************************
** ��������: __gpioNamedGpioDescGet
** ��������: ��ȡ�豸���ڵ��� GPIO �б��ָ��λ�õ� GPIO �����ṹ
** �䡡��  : pdtnDev            �豸���ڵ�
**           pcListName         GPIO �б��������
**           iIndex             �б��е�λ������
** �䡡��  : GPIO �����ṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_GPIO_DESC  __gpioNamedGpioDescGet (PLW_DEVTREE_NODE  pdtnDev,
                                              CPCHAR            pcListName,
                                              INT               iIndex)
{
    INT                         iRet;
    LW_DEVTREE_PHANDLE_ARGS     dtpaGpio;
    PLW_GPIO_CHIP               pgchip;
    PLW_GPIO_DESC               pgdesc;
    UINT                        uiOffset;

    /*
     *  ��ȡָ��λ�õ� Phandle
     */
    iRet = API_DeviceTreePhandleParseWithArgs(pdtnDev, pcListName, "#gpio-cells",
                                              iIndex, &dtpaGpio);
    if (iRet != ERROR_NONE) {
        return  (LW_NULL);
    }

    /*
     *  ���Ҷ�Ӧ�� GPIO ������
     */
    pgchip = API_GpioChipFind(dtpaGpio.DTPH_pdtnDev, __gpioDevtreeNodeMatch);
    if (!pgchip) {
        _ErrorHandle(ENODEV);
        return  (LW_NULL);
    }

    uiOffset = dtpaGpio.DTPH_uiArgs[0];
    if (uiOffset >= pgchip->GC_uiNGpios) {                              /*  GPIO ƫ�Ƴ�����Χ           */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pgdesc = &(pgchip->GC_gdDesc[uiOffset]);

    return  (pgdesc);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeGpioCtrlRegister
** ��������: GPIO ������ע��
** �䡡��  : pdtgpioctrl      GPIO ������ָ��
**           pcName           GPIO ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioCtrlRegister (PLW_DT_GPIO_CTRL  pdtgpioctrl, CPCHAR  pcName)
{
    return  (API_GpioCtrlRegister(pdtgpioctrl, pcName));
}
/*********************************************************************************************************
** ��������: API_DeviceTreeGpioCtrlRemove
** ��������: �Ƴ� GPIO ������
** �䡡��  : pdtgpioctrl      GPIO ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_DeviceTreeGpioCtrlRemove (PLW_DT_GPIO_CTRL  pdtgpioctrl)
{
    API_GpioCtrlUnregister(pdtgpioctrl);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeGpioPinRangeAdd
** ��������: �� PINCTRL ����������� GPIO ���ŷ�Χ
** �䡡��  : pdtgpioctrl      GPIO ������ָ��
**           ppinctldev       PINCTRL ������ָ��
**           uiGpioOffset     GPIO ƫ��
**           uiPinOffset      ����ƫ��
**           uiNPins          ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioPinRangeAdd (PLW_DT_GPIO_CTRL   pdtgpioctrl,
                                    PLW_PINCTRL_DEV    ppinctldev,
                                    UINT               uiGpioOffset,
                                    UINT               uiPinOffset,
                                    UINT               uiNPins)
{
    PLW_GPIO_PIN_RANGE      pgpiopinrange;

    pgpiopinrange = (PLW_GPIO_PIN_RANGE)__SHEAP_ZALLOC(sizeof(LW_GPIO_PIN_RANGE));
    if (!pgpiopinrange) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    pgpiopinrange->GPRANGE_ppinctldev = ppinctldev;

    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_pcName     = pdtgpioctrl->DTGPIOCTRL_cName;
    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_uiGpioBase = pdtgpioctrl->DTGPIOCTRL_uiBase + uiGpioOffset;
    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_uiPinBase  = uiPinOffset;
    pgpiopinrange->GPRANGE_gpioRange.PCTLGR_uiNPins    = uiNPins;

    if (API_PinCtrlGpioRangeAdd(ppinctldev, &pgpiopinrange->GPRANGE_gpioRange) != ERROR_NONE) {
        __SHEAP_FREE(pgpiopinrange);
        return  (PX_ERROR);
    }

    _List_Line_Add_Ahead(&pgpiopinrange->GPRANGE_lineManage, &pdtgpioctrl->DTGPIOCTRL_plineGpioRange);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeGpioPinRangeRemove
** ��������: �� PINCTRL ���������Ƴ� GPIO ��������Ӧ���������ŷ�Χ
** �䡡��  : pdtgpioctrl      GPIO ������ָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_DeviceTreeGpioPinRangeRemove (PLW_DT_GPIO_CTRL  pdtgpioctrl)
{
    PLW_LIST_LINE           plineTemp;
    PLW_GPIO_PIN_RANGE      pgpiopinrange;

    for (plineTemp  = pdtgpioctrl->DTGPIOCTRL_plineGpioRange;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �������ſ��Ƶ�ӳ������      */

        pgpiopinrange = _LIST_ENTRY(plineTemp, LW_GPIO_PIN_RANGE, GPRANGE_lineManage);

        API_PinCtrlGpioRangeRemove(pgpiopinrange->GPRANGE_ppinctldev,
                                   &pgpiopinrange->GPRANGE_gpioRange);
        _List_Line_Del(plineTemp, &pdtgpioctrl->DTGPIOCTRL_plineGpioRange);
        __SHEAP_FREE(pgpiopinrange);
    }
}
/*********************************************************************************************************
** ��������: API_DeviceTreeGpioNamedGpioGet
** ��������: ��ȡ�豸���ڵ��� GPIO �б��ָ��λ�õ� GPIO ��
** �䡡��  : pdtnDev         �豸���ڵ�
**           pcListName      GPIO �б��������
**           iIndex          �б��е�����
** �䡡��  : GPIO ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioNamedGpioGet (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcListName, INT  iIndex)
{
    PLW_GPIO_DESC   pgdesc;

    pgdesc = __gpioNamedGpioDescGet(pdtnDev, pcListName, iIndex);
    if (!pgdesc) {
        return  (PX_ERROR);
    }

    return  (DESC_TO_GPIO(pgdesc));
}
/*********************************************************************************************************
** ��������: API_DeviceTreeGpioNamedCountGet
** ��������: ��ȡ�豸���ڵ��� GPIO �б����Ŀ����
** �䡡��  : pdtnDev         �豸���ڵ�
**           pcListName      GPIO �б��������
** �䡡��  : ��Ŀ����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeGpioNamedCountGet (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcListName)
{
    return  (API_DeviceTreePhandleCountWithArgs(pdtnDev, pcListName, "#gpio-cells"));
}

#endif                                                                  /*  (LW_CFG_GPIO_EN > 0) &&     */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
