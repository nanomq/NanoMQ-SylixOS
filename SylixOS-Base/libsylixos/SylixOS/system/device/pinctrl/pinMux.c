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
** ��   ��   ��: pinMux.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 02 ��
**
** ��        ��: ���Ÿ�������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "pinCtrl.h"
/*********************************************************************************************************
** ��������: __pinMuxFuncNameToSelector
** ��������: �����Ÿ����������ת��Ϊ���
** �䡡��  : ppinctldev        ���ſ�����
**           pcFunction     ���Ÿ����������
** �䡡��  : ת���������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pinMuxFuncNameToSelector (PLW_PINCTRL_DEV  ppinctldev, CPCHAR  pcFunction)
{
    PLW_PINMUX_OPS  ppinmuxops;
    CPCHAR          pcFuncName;
    UINT            uiNFuncs;
    UINT            uiSelector = 0;

    ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    uiNFuncs   = ppinmuxops->pinmuxFuncCountGet(ppinctldev);            /*  ��ȡ���Ÿ������������    */

    while (uiSelector < uiNFuncs) {
        pcFuncName = ppinmuxops->pinmuxFuncNameGet(ppinctldev,
                                                   uiSelector);         /*  ��ȡ���Ÿ����������        */
        if (!lib_strcmp(pcFunction, pcFuncName)) {
            return  (uiSelector);
        }
        uiSelector++;
    }

    PCTL_LOG(PCTL_LOG_ERR, "does not support function %s\r\n", pcFunction);

    return  (-EINVAL);
}
/*********************************************************************************************************
** ��������: __pinmuxPinRequest
** ��������: ���Ÿ��õ���������
** �䡡��  : ppinctldev     ���ſ�����
**           iPin           �������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pinMuxRequest (PLW_PINCTRL_DEV  ppinctldev,
                             INT              iPin)
{
    PLW_PIN_DESC     pindesc;
    PLW_PINMUX_OPS   ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    INT              iRet;

    pindesc = API_PinCtrlPinDescGet(ppinctldev, iPin);
    if (pindesc == LW_NULL) {                                           /*  ������Ų�����              */
        return  (PX_ERROR);
    }

    if (ppinmuxops->pinmuxRequest) {
        iRet = ppinmuxops->pinmuxRequest(ppinctldev, iPin);
    } else {
        iRet = ERROR_NONE;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __pinMuxFree
** ��������: ���Ÿ��õ������ͷ�
** �䡡��  : ppinctldev     ���ſ�����
**           iPin           �������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static  VOID  __pinMuxFree (PLW_PINCTRL_DEV  ppinctldev,
                            INT              iPin)
{
    PLW_PINMUX_OPS   ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    PLW_PIN_DESC     pindesc;

    pindesc = API_PinCtrlPinDescGet(ppinctldev, iPin);
    if (pindesc == LW_NULL) {                                           /*  ������Ų�����              */
        return;
    }

    if (ppinmuxops->pinmuxFree) {
        ppinmuxops->pinmuxFree(ppinctldev, iPin);
    }
}
/*********************************************************************************************************
** ��������: API_PinmuxSettingEnable
** ��������: ���Ÿ�������ʹ��
** �䡡��  : ppinctlsetting      ʹ�ܵ����Ÿ�������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinMuxEnable (PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV         ppinctldev;
    PLW_PINCTRL_OPS         ppinctlops;
    PLW_PINMUX_OPS          ppinmuxops;
    PLW_PIN_DESC            pindesc;
    CPCHAR                  pcPinName   = LW_NULL;
    CPCHAR                  pcGroupName = LW_NULL;
    UINT                   *puiPins     = LW_NULL;
    UINT                    uiNumPins   = 0;
    INT                     iRet        = 0;
    INT                     i;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev) {                            /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;

    if (!ppinctldev->PCTLD_ppinctldesc ||
        !ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops ||
        !ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops) {             /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctlops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
    ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;

    if (ppinctlops->pinctrlGroupPinsGet) {
        iRet = ppinctlops->pinctrlGroupPinsGet(ppinctldev,
                                               ppinctlsetting->set_uiGroup,
                                               &puiPins,
                                               &uiNumPins);             /*  ������ż���                */
    }

    if (iRet) {
        if (ppinctlops->pinctrlGroupNameGet) {
            pcGroupName = ppinctlops->pinctrlGroupNameGet(ppinctldev,
                                                          ppinctlsetting->set_uiGroup);
        }
        PCTL_LOG(PCTL_LOG_LOG, "could not get pins for group %s\r\n", pcGroupName);
        uiNumPins = 0;
    }

    for (i = 0; i < uiNumPins; i++) {                                   /*  ��������ÿ������            */
        iRet = __pinMuxRequest(ppinctldev, puiPins[i]);
        if (iRet) {                                                     /*  ����ʧ��ʱ����Ϣ��ӡ        */
            pindesc   = API_PinCtrlPinDescGet(ppinctldev, puiPins[i]);
            pcPinName = pindesc ? pindesc->PIN_pcName : "non-existing";
            if (ppinctlops->pinctrlGroupNameGet) {
                pcGroupName = ppinctlops->pinctrlGroupNameGet(ppinctldev,
                                                              ppinctlsetting->set_uiGroup);
            }
            PCTL_LOG(PCTL_LOG_LOG, "could not request pin %d (%s) from group %s\r\n",
                     puiPins[i], pcPinName, pcGroupName);
            goto  __error_handle;
        }
    }

    if (ppinmuxops->pinmuxMuxSet) {                                     /*  �������Ÿ��ù���            */
        iRet = ppinmuxops->pinmuxMuxSet(ppinctldev,
                                        ppinctlsetting->set_uiFunc,
                                        ppinctlsetting->set_uiGroup);
        if (iRet) {
            goto  __error_handle;
        }
    } else {
        goto  __error_handle;
    }

    return  (ERROR_NONE);

__error_handle:
    while (--i >= 0) {
        __pinMuxFree(ppinctldev, puiPins[i]);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_PinMuxDisable
** ��������: ���Ÿ������ý���
** �䡡��  : ppinctlsetting      ���ܵ����Ÿ�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_PinMuxDisable (PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV   ppinctldev;
    PLW_PINCTRL_OPS   ppinctlops;
    UINT             *puiPins;
    UINT              uiNumPins;
    INT               iRet = PX_ERROR;
    INT               i;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev) {                            /*  �������                    */
        return;
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;

    if (!ppinctldev->PCTLD_ppinctldesc ||
        !ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops) {             /*  �������                    */
        return;
    }

    ppinctlops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;

    if (ppinctlops->pinctrlGroupPinsGet) {                              /*  ������ż���                */
        iRet = ppinctlops->pinctrlGroupPinsGet(ppinctldev,
                                               ppinctlsetting->set_uiGroup,
                                               &puiPins,
                                               &uiNumPins);
    }

    if (iRet) {
        PCTL_LOG(PCTL_LOG_LOG, "could not get pins for group selector %d\r\n",
                 ppinctlsetting->set_uiGroup);
        uiNumPins = 0;
    }

    for (i = 0; i < uiNumPins; i++) {
        __pinMuxFree(ppinctldev, puiPins[i]);
    }
}
/*********************************************************************************************************
** ��������: API_PinMuxMapToSetting
** ��������: ������ӳ��ṹת��Ϊ���Ÿ�������
** �䡡��  : ppinctlmap       ����ת��������ӳ��ṹ
**           ppinctlsetting   ת��������������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinMuxMapToSetting (PLW_PINCTRL_MAP  ppinctlmap, PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV   ppinctldev;
    PLW_PINMUX_OPS    ppinmuxops;
    CHAR            **ppGroups;
    UINT              uiNumGroups;
    CPCHAR            pGroup;
    BOOL              bFound = LW_FALSE;
    INT               iRet   = PX_ERROR;
    INT               i;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev ||
        !ppinctlsetting->PCTLS_ppinctldev->PCTLD_ppinctldesc) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;
    ppinmuxops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    if (!ppinmuxops) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iRet = __pinMuxFuncNameToSelector(ppinctldev,
                                      ppinctlmap->map_pcFunction);      /*  �����Ÿ��������ת��Ϊ���  */
    if (iRet < 0) {
        return  (iRet);
    }
    ppinctlsetting->set_uiFunc = iRet;                                  /*  �������Ÿ������õ����      */

    if (ppinmuxops->pinmuxFuncGroupsGet) {                              /*  ������Ÿ����鼯��          */
       iRet = ppinmuxops->pinmuxFuncGroupsGet(ppinctldev,
                                              ppinctlsetting->set_uiFunc,
                                              &ppGroups,
                                              &uiNumGroups);            /*  ��ȡ���Ÿ����鼯��          */
       if (iRet < 0) {
           return  (iRet);
       }

       if (!uiNumGroups) {                                              /*  ��Ӧ������������Ϊ 0        */
           _ErrorHandle(EINVAL);
           return  (PX_ERROR);
       }
    } else {
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    if (ppinctlmap->map_pcGroup) {                                      /*  �������ӳ�����������Ϣ    */
        pGroup = ppinctlmap->map_pcGroup;
        for (i = 0; i < uiNumGroups; i++) {
            if (!lib_strcmp(pGroup, ppGroups[i])) {                     /*  �ҵ���Ӧ��������            */
                bFound = LW_TRUE;
                break;
            }
        }

        if (!bFound) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    } else {
        pGroup = ppGroups[0];
    }

    iRet = API_PinCtrlGroupSelectorGet(ppinctldev, pGroup);             /*  ��������ת��Ϊ���          */
    if (iRet < 0) {
        return  (iRet);
    }

    ppinctlsetting->set_uiGroup = iRet;                                 /*  �����������õ����������    */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PinMuxMapValidate
** ��������: ���Ÿ�����Ч�Լ��
** �䡡��  : ppinctlmap    ���ڼ�������ӳ��ṹ
**           i             ����ӳ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinMuxMapValidate (PLW_PINCTRL_MAP  ppinctlmap, INT  i)
{
    if (!ppinctlmap || !ppinctlmap->map_pcFunction) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
