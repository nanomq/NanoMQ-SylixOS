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
** ��   ��   ��: pinCtrl.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 02 ��
**
** ��        ��: ���ſ���ʵ��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
#include "pinCtrl.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER      _G_plinePinCtrls;                       /*  GPIO ����������             */
static LW_LIST_LINE_HEADER      _G_plinePinCtrlMaps;                    /*  ȫ����Ч��ӳ���б�          */
/*********************************************************************************************************
  ͬ����
*********************************************************************************************************/
static LW_OBJECT_HANDLE         _G_hPctlLock     = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE         _G_hPctlMapsLock = LW_OBJECT_HANDLE_INVALID;

#define __PCTL_LOCK()           API_SemaphoreMPend(_G_hPctlLock, LW_OPTION_WAIT_INFINITE)
#define __PCTL_UNLOCK()         API_SemaphoreMPost(_G_hPctlLock)
#define __PCTLMAPS_LOCK()       API_SemaphoreMPend(_G_hPctlMapsLock, LW_OPTION_WAIT_INFINITE)
#define __PCTLMAPS_UNLOCK()     API_SemaphoreMPost(_G_hPctlMapsLock)
/*********************************************************************************************************
** ��������: __pinCtrlStateCommit
** ��������: ʹ��ָ��������״̬
** �䡡��  : ppinctl          ���ſ��ƽṹ
**           ppctlstate       ��Ҫ���õ�����״̬
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
static INT  __pinCtrlStateCommit (PLW_PINCTRL  ppinctl, PLW_PINCTRL_STATE  ppctlstate)
{
    PLW_PINCTRL_SETTING  ppctlsetting;
    PLW_PINCTRL_SETTING  ppctlsetting2;
    PLW_PINCTRL_STATE    ppctloldstate;
    PLW_LIST_LINE        plineTemp;
    INT                  iRet;

    ppctloldstate = ppinctl->PCTL_ppctlstate;                           /*  ��¼ԭ��������״̬          */
    if (ppctloldstate) {                                                /*  ���ԭ�����趨������״̬    */
        for (plineTemp  = ppinctl->PCTL_ppctlstate->PCTLS_plineSettings;/*  ����ԭ������״̬�е�����    */
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            ppctlsetting = _LIST_ENTRY(plineTemp, LW_PINCTRL_SETTING, PCTLS_lineManage);
            if (ppctlsetting->PCTLS_pinctlmaptype !=
                PIN_MAP_TYPE_MUX_GROUP) {                               /*  ������ǰ������Ÿ�������    */
                continue;
            }

            API_PinMuxDisable(ppctlsetting);                            /*  ���ø���������              */
         }
    }

    ppinctl->PCTL_ppctlstate = LW_NULL;                                 /*  ������״̬���              */

    for (plineTemp  = ppctlstate->PCTLS_plineSettings;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ��������������״̬�е�����  */

        ppctlsetting = _LIST_ENTRY(plineTemp, LW_PINCTRL_SETTING, PCTLS_lineManage);

        switch (ppctlsetting->PCTLS_pinctlmaptype) {

        case PIN_MAP_TYPE_MUX_GROUP:                                    /*  �������Ÿ���                */
            iRet = API_PinMuxEnable(ppctlsetting);
            break;

        case PIN_MAP_TYPE_CONFIGS_PIN:                                  /*  ������������                */
        case PIN_MAP_TYPE_CONFIGS_GROUP:                                /*  ������������                */
            iRet = API_PinConfigApply(ppctlsetting);
            break;

        default:
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if (iRet < 0) {                                                 /*  ʧ��ʱ�ָ�ԭ��������        */
            goto  __error_handle;
        }
    }

    ppinctl->PCTL_ppctlstate = ppctlstate;

    return  (ERROR_NONE);

__error_handle:
    for (plineTemp  = ppctlstate->PCTLS_plineSettings;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppctlsetting2 = _LIST_ENTRY(plineTemp, LW_PINCTRL_SETTING, PCTLS_lineManage);

        if (&ppctlsetting2->PCTLS_pdtnDev == &ppctlsetting->PCTLS_pdtnDev) {
            break;
        }

        if (ppctlsetting2->PCTLS_pinctlmaptype == PIN_MAP_TYPE_MUX_GROUP) {
            API_PinMuxDisable(ppctlsetting2);
        }
    }

    if (ppctloldstate) {                                                /*  ��������ԭ��������          */
        API_PinCtrlStateSelect(ppinctl, ppctloldstate);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __pinCtrlStateFind
** ��������: �����ſ���״̬�����в��Ҷ�Ӧ���Ƶ�����״̬
** �䡡��  : ppinctl        ���������ſ���
**           pcName         ���ҵ�����״̬����
** �䡡��  : �ҵ�������״̬
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_PINCTRL_STATE  __pinCtrlStateFind (PLW_PINCTRL  ppinctl, CPCHAR  pcName)
{
    PLW_PINCTRL_STATE  ppctlstate;
    PLW_LIST_LINE      plineTemp;

    for (plineTemp  = ppinctl->PCTL_plineStates;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppctlstate = _LIST_ENTRY(plineTemp, LW_PINCTRL_STATE, PCTLS_lineManage);
        if (!lib_strcmp(ppctlstate->PCTLS_pcName, pcName)) {
            return  (ppctlstate);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __pinCtrlStateCreate
** ��������: �����µ�����״̬�����뵽���ſ���״̬������
** �䡡��  : ppinctl        ���������ſ���
**           pcName         ����������״̬����
** �䡡��  : ����������״̬
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_PINCTRL_STATE  __pinCtrlStateCreate (PLW_PINCTRL  ppinctl, CPCHAR  pcName)
{
    PLW_PINCTRL_STATE  ppctlstate;

    ppctlstate = (PLW_PINCTRL_STATE)__SHEAP_ZALLOC(sizeof(LW_PINCTRL_STATE));
    if (ppctlstate == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    ppctlstate->PCTLS_pcName = pcName;

    _List_Line_Add_Ahead(&ppctlstate->PCTLS_lineManage, &ppinctl->PCTL_plineStates);

    return  (ppctlstate);
}
/*********************************************************************************************************
** ��������: __pinCtrlSettingAdd
** ��������: ������ӳ��ṹת��Ϊ�������ã������������ſ���
** �䡡��  : ppinctl          ���������ſ���
**           ppctlmap         ����ת��������ӳ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __pinCtrlSettingAdd (PLW_PINCTRL  ppinctl, PLW_PINCTRL_MAP  ppctlmap)
{
    PLW_PINCTRL_STATE     ppctlstate;
    PLW_PINCTRL_SETTING   ppctlsetting;
    INT                   iRet;

    ppctlstate = __pinCtrlStateFind(ppinctl, ppctlmap->PCTLM_pcName);   /*  ��������״̬                */
    if (!ppctlstate) {
        ppctlstate = __pinCtrlStateCreate(ppinctl,
                                          ppctlmap->PCTLM_pcName);      /*  ���ޣ��򴴽�����״̬        */
        if (!ppctlstate) {
             return  (PX_ERROR);
        }
    }

    if (ppctlmap->PCTLM_pctlmaptype == PIN_MAP_TYPE_DUMMY_STATE) {      /*  DUMMY_STATE ������          */
        return  (ERROR_NONE);
    }

    ppctlsetting = (PLW_PINCTRL_SETTING)__SHEAP_ZALLOC(sizeof(LW_PINCTRL_SETTING));
    if (ppctlsetting == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    ppctlsetting->PCTLS_pinctlmaptype = ppctlmap->PCTLM_pctlmaptype;    /*  �������õ�ӳ������          */
    ppctlsetting->PCTLS_pdtnDev = ppctlmap->PCTLM_pdtnDev;              /*  �������ù������豸���ڵ�    */

    ppctlsetting->PCTLS_ppinctldev = API_PinCtrlDevGetByDevtreeNode(ppctlmap->PCTLM_pdtnCtrlNode);
    if (ppctlsetting->PCTLS_ppinctldev == LW_NULL) {                    /*  �������ù��������ſ����豸  */
        __SHEAP_FREE(ppctlsetting);
        if (ppctlmap->PCTLM_pdtnCtrlNode == ppctlmap->PCTLM_pdtnDev) {
            _ErrorHandle(-ENODEV);
            return  (PX_ERROR);
        }
        return  (-ERROR_DEVTREE_EPROBE_DEFER);
    }

    switch (ppctlmap->PCTLM_pctlmaptype) {                              /*  ������ӳ��ṹת��Ϊ��������*/

    case PIN_MAP_TYPE_MUX_GROUP:                                        /*  �������Ÿ�������            */
        iRet = API_PinMuxMapToSetting(ppctlmap, ppctlsetting);
        break;

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  ���յ������Ź�������        */
    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  �������Ź�������            */
        iRet = API_PinConfigMapToSetting(ppctlmap, ppctlsetting);
        break;

    default:
        iRet = -EINVAL;
        break;
    }

    if (iRet < 0) {
        __SHEAP_FREE(ppctlsetting);
        return  (iRet);
    }

    _List_Line_Add_Ahead(&ppctlsetting->PCTLS_lineManage,
                         &ppctlstate->PCTLS_plineSettings);             /*  ��������״̬����������      */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __pinCtrlSettingFree
** ��������: �ͷ�һ����������
** �䡡��  : bDisableSetting     �ͷ�ʱ�Ƿ���ø�����
**           ppctlsetting        �Ƿ����������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pinCtrlSettingFree (BOOL  bDisableSetting, PLW_PINCTRL_SETTING  ppctlsetting)
{
    switch (ppctlsetting->PCTLS_pinctlmaptype) {

    case PIN_MAP_TYPE_MUX_GROUP:                                        /*  �������Ÿ���                */
        if (bDisableSetting) {
            API_PinMuxDisable(ppctlsetting);
        }
        break;

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  ������������                */
    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  ����������� (��ʱ������)   */
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: __pinCtrlFind
** ��������: ����ĳ��������ص����ſ���
** �䡡��  : pdtnDev      �����豸���ڵ�
** �䡡��  : ���ҵ������ſ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_PINCTRL  __pinCtrlFind (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL      ppinctl;
    PLW_LIST_LINE    plineTemp;

    if (_G_hPctlLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlLock = API_SemaphoreMCreate("ppinctl_lock", LW_PRIO_DEF_CEILING,
                                            LW_OPTION_WAIT_PRIORITY |
                                            LW_OPTION_INHERIT_PRIORITY |
                                            LW_OPTION_DELETE_SAFE |
                                            LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    if (_G_hPctlMapsLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlMapsLock = API_SemaphoreMCreate("ppctlmaps_lock", LW_PRIO_DEF_CEILING,
                                                LW_OPTION_WAIT_PRIORITY |
                                                LW_OPTION_INHERIT_PRIORITY |
                                                LW_OPTION_DELETE_SAFE |
                                                LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    __PCTL_LOCK();
    for (plineTemp  = _G_plinePinCtrls;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �������ſ�������            */

        ppinctl = _LIST_ENTRY(plineTemp, LW_PINCTRL, PCTL_lineManage);
        if (ppinctl->PCTL_pdtnDev == pdtnDev) {                         /*  ƥ�����ſ��ƹ����Ľڵ�      */
            __PCTL_UNLOCK();
            return  (ppinctl);
        }
    }
    __PCTL_UNLOCK();

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __pinctrlFree
** ��������: �ͷ�һ�����ſ���
** �䡡��  : ppinctl     ��Ҫ�ͷŵ����ſ���
**           bInlist     �Ƿ�ͬʱ���������Ƴ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __pinCtrlFree (PLW_PINCTRL  ppinctl, BOOL  bInlist)
{
    PLW_PINCTRL_STATE    ppctlstate;
    PLW_PINCTRL_SETTING  ppctlsetting;
    PLW_LIST_LINE        plinestate;
    PLW_LIST_LINE        plinesetting;
    BOOL                 bDisableSetting;

    __PCTL_LOCK();

    for (plinestate  = ppinctl->PCTL_plineStates;
         plinestate != LW_NULL;
         plinestate  = _list_line_get_next(plinestate)) {               /*  �������ſ��Ƶ�����״̬����  */

        ppctlstate = _LIST_ENTRY(plinestate, LW_PINCTRL_STATE, PCTLS_lineManage);

        for (plinesetting  = ppctlstate->PCTLS_plineSettings;
             plinesetting != LW_NULL;
             plinesetting  = _list_line_get_next(plinesetting)) {       /*  ��������״̬�е���������    */

            ppctlsetting = _LIST_ENTRY(plinesetting, LW_PINCTRL_SETTING, PCTLS_lineManage);

            bDisableSetting = (ppctlstate == ppinctl->PCTL_ppctlstate);
            __pinCtrlSettingFree(bDisableSetting, ppctlsetting);        /*  ע������������              */

            _List_Line_Del(&ppctlsetting->PCTLS_lineManage,
                           &ppctlstate->PCTLS_plineSettings);           /*  �Ƴ����������ýڵ�          */
            __SHEAP_FREE(ppctlsetting);                                 /*  �ͷŸ����������ڴ�          */
        }

        _List_Line_Del(&ppctlstate->PCTLS_lineManage,
                       &ppinctl->PCTL_plineStates);                     /*  �Ƴ�������״̬              */
        __SHEAP_FREE(ppctlstate);                                       /*  �ͷŸ�����״̬�ڴ�          */
    }

    API_DeviceTreePinCtrlMapsFree(ppinctl);                             /*  ע�������ſ���              */

    if (bInlist) {
        _List_Line_Del(&ppinctl->PCTL_lineManage, &_G_plinePinCtrls);
    }

    __SHEAP_FREE(ppinctl);                                              /*  �ͷŸ������ڴ�              */

    __PCTL_UNLOCK();
}
/*********************************************************************************************************
** ��������: __pinCtrlCreate
** ��������: ����ĳ��������ص����ſ���
** �䡡��  : pdtnDev      �����豸���ڵ�
** �䡡��  : ���������ſ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_PINCTRL  __pinCtrlCreate (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL        ppinctl;
    PLW_PINCTRL_MAPS   ppctlmaps;
    PLW_PINCTRL_MAP    ppctlmap;
    PLW_LIST_LINE      plineTemp;
    INT                iRet;
    INT                i;

    ppinctl = (PLW_PINCTRL)__SHEAP_ZALLOC(sizeof(LW_PINCTRL));
    if (ppinctl == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    ppinctl->PCTL_pdtnDev = pdtnDev;                                    /*  ���ſ��ƹ����豸���ڵ�      */

    iRet = API_DeviceTreePinCtrlMapsCreate(ppinctl);                    /*  ���豸������������ӳ��ṹ  */
    if (iRet < 0) {
        __SHEAP_FREE(ppinctl);
        return  (LW_NULL);
    }

    __PCTLMAPS_LOCK();

    for (plineTemp  = _G_plinePinCtrlMaps;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ����������Ч������ӳ��      */

        ppctlmaps = _LIST_ENTRY(plineTemp, LW_PINCTRL_MAPS, PCTLM_lineGlobalManage);

        for (i = 0, ppctlmap = &ppctlmaps->PCTLM_ppinctlmaps[i];
             i < ppctlmaps->PCTLM_uiMapsNum;
             i++, ppctlmap = &ppctlmaps->PCTLM_ppinctlmaps[i]) {

            if (ppctlmap->PCTLM_pdtnDev != pdtnDev) {                   /*  �����ǵ�ǰ�豸              */
                continue;
            }

            iRet = __pinCtrlSettingAdd(ppinctl, ppctlmap);              /*  ������������                */
            if (iRet == -ERROR_DEVTREE_EPROBE_DEFER) {
                __pinCtrlFree(ppinctl, LW_FALSE);
                __PCTLMAPS_UNLOCK();
                return  (LW_NULL);
            }
        }
    }
    __PCTLMAPS_UNLOCK();

    if (iRet < 0) {
        __pinCtrlFree(ppinctl, LW_FALSE);
        return  (LW_NULL);
    }

    __PCTL_LOCK();
    _List_Line_Add_Ahead(&ppinctl->PCTL_lineManage, &_G_plinePinCtrls);
    __PCTL_UNLOCK();

    return  (ppinctl);
}
/*********************************************************************************************************
** ��������: API_PinCtrlPinGetByName
** ��������: �������ƻ�ȡ�������
** �䡡��  : ppctldev        ���ſ������豸
**           pcName          ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinCtrlPinGetByName (PLW_PINCTRL_DEV  ppctldev, CPCHAR  pcName)
{
    PLW_PIN_DESC  pindesc;
    UINT          uiPin;
    UINT          i;

    if (!pcName   ||
        !ppctldev ||
        !ppctldev->PCTLD_ppinctldesc) {                                 /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    for (i = 0;
         i < ppctldev->PCTLD_ppinctldesc->PCTLD_uiPinsNum;
         i++) {                                                         /*  ������������                */

        uiPin   = ppctldev->PCTLD_ppinctldesc->PCTLD_ppinctrlpindesc[i].PCTLPD_uiIndex;
                                                                        /*  ��ȡ�������                */
        pindesc = API_PinCtrlPinDescGet(ppctldev, uiPin);               /*  ��ȡ����ŵ���������        */
        if (pindesc && !lib_strcmp(pcName, pindesc->PIN_pcName)) {      /*  �Ƚ����������е�����        */
            return  (uiPin);
        }
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PinCtrlGroupSelectorGet
** ��������: �������������ƻ�ȡ���������
** �䡡��  : ppctldev       ���ſ�����
**           pcPinGroup     ����������
** �䡡��  : ���������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinCtrlGroupSelectorGet (PLW_PINCTRL_DEV  ppctldev, CPCHAR  pcPinGroup)
{
    PLW_PINCTRL_OPS  ppinctlops;
    CPCHAR           pcGroupName     = LW_NULL;
    UINT             uiGroupsNum     = 0;
    UINT             uiGroupSelector = 0;

    if (!ppctldev || !ppctldev->PCTLD_ppinctldesc) {                    /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctlops = ppctldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
    if (!ppinctlops->pinctrlGroupCountGet ||
        !ppinctlops->pinctrlGroupNameGet) {                             /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    uiGroupsNum = ppinctlops->pinctrlGroupCountGet(ppctldev);           /*  ��ȡ����������              */

    while (uiGroupSelector < uiGroupsNum) {                             /*  ��������������              */
        pcGroupName = ppinctlops->pinctrlGroupNameGet(ppctldev,
                                                      uiGroupSelector); /*  �������������              */
        if (!lib_strcmp(pcGroupName, pcPinGroup)) {                     /*  �ҵ�ƥ���������            */
           return  (uiGroupSelector);
       }
        uiGroupSelector++;
    }

    PCTL_LOG(PCTL_LOG_BUG, "does not have pin group %s\r\n", pcPinGroup);
    _ErrorHandle(EINVAL);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PinCtrlStateFind
** ��������: ����ָ�����Ƶ�����״̬
** �䡡��  : ppinctl           ���ſ���
**           pcName            ���ſ���״̬����
** �䡡��  : �ҵ������ſ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_PINCTRL_STATE  API_PinCtrlStateFind (PLW_PINCTRL  ppinctl, CPCHAR  pcName)
{
    PLW_PINCTRL_STATE  ppctlstate;

    ppctlstate = __pinCtrlStateFind(ppinctl, pcName);                   /*  ��������״̬                */
    if (!ppctlstate) {
        ppctlstate = (LW_NULL);
    }

    return  (ppctlstate);
}
/*********************************************************************************************************
** ��������: API_PinCtrlStateSelect
** ��������: ѡ��ָ��������״̬��Ӧ�ö�Ӧ����״̬������
** �䡡��  : ppinctl            ���ſ���
**           ppctlstate         ���ſ���״̬
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinCtrlStateSelect (PLW_PINCTRL  ppinctl, PLW_PINCTRL_STATE  ppctlstate)
{
    if (!ppinctl) {                                                     /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (ppinctl->PCTL_ppctlstate == ppctlstate) {                       /*  ��Ԥ��ֵ״̬�Ѿ�Ϊ��ǰ״̬  */
        return  (ERROR_NONE);
    }

    return  (__pinCtrlStateCommit(ppinctl, ppctlstate));
}
/*********************************************************************************************************
** ��������: API_PinCtrlMapDel
** ��������: ע��һ�����ſ��Ƶ�����ӳ��ṹ
**           ������ӳ��ṹ��ȫ�ֵ�����ӳ�������Ƴ�
** �䡡��  : ppctlmap      ��Ҫע��������ӳ��ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_PinCtrlMapDel (PLW_PINCTRL_MAP  ppctlmap)
{
    PLW_PINCTRL_MAPS  ppctlmaps;
    PLW_LIST_LINE     plineTemp;

    __PCTLMAPS_LOCK();
    for (plineTemp  = _G_plinePinCtrlMaps;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppctlmaps = _LIST_ENTRY(plineTemp, LW_PINCTRL_MAPS, PCTLM_lineGlobalManage);
        if (ppctlmaps->PCTLM_ppinctlmaps == ppctlmap) {
            _List_Line_Del(&ppctlmaps->PCTLM_lineGlobalManage, &_G_plinePinCtrlMaps);
            break;
        }
    }
    __PCTLMAPS_UNLOCK();
}
/*********************************************************************************************************
** ��������: API_PinCtrlMapAdd
** ��������: ע�����ſ��Ƶ�����ӳ��ṹ
**           ����Ч������ӳ��ṹ����ȫ�ֵ�����ӳ������
** �䡡��  : ppctlmaps      ��Ҫע�������ӳ��ṹ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinCtrlMapAdd (PLW_PINCTRL_MAPS  ppctlmaps)
{
    PLW_PINCTRL_MAP   ppctlmap;
    UINT              uiNumMaps;
    INT               iRet;
    INT               i;

    if (!ppctlmaps) {                                                   /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppctlmap  = ppctlmaps->PCTLM_ppinctlmaps;
    uiNumMaps = ppctlmaps->PCTLM_uiMapsNum;

    for (i = 0; i < uiNumMaps; i++) {                                   /*  ����ӳ����Ч�Լ��          */
        if (!ppctlmap[i].PCTLM_pdtnDev ||
            !ppctlmap[i].PCTLM_pcName) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if ((ppctlmap[i].PCTLM_pctlmaptype != PIN_MAP_TYPE_DUMMY_STATE) &&
            !ppctlmap[i].PCTLM_pdtnCtrlNode) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        switch (ppctlmap[i].PCTLM_pctlmaptype) {

        case PIN_MAP_TYPE_DUMMY_STATE:
            break;

        case PIN_MAP_TYPE_MUX_GROUP:                                    /*  ���Ÿ���������Ч�Լ��      */
            iRet = API_PinMuxMapValidate(&ppctlmap[i], i);
            if (iRet < 0) {
                return  (iRet);
            }
            break;

        case PIN_MAP_TYPE_CONFIGS_PIN:
        case PIN_MAP_TYPE_CONFIGS_GROUP:                                /*  ���Ź���������Ч�Լ��      */
            iRet = API_PinConfigMapValidate(&ppctlmap[i], i);
            if (iRet < 0) {
                return  (iRet);
            }
            break;

        default:
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    __PCTLMAPS_LOCK();
    _List_Line_Add_Ahead(&ppctlmaps->PCTLM_lineGlobalManage, &_G_plinePinCtrlMaps);
    __PCTLMAPS_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PinCtrlGet
** ��������: ��ȡĳ��������ص����ſ���
** �䡡��  : pdtnDev      �����豸���ڵ�
** �䡡��  : ���ҵ������ſ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_PINCTRL  API_PinCtrlGet (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL  ppinctl;

    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    ppinctl = __pinCtrlFind(pdtnDev);                                   /*  ����������ص����ſ���      */
    if (ppinctl) {
        return  (ppinctl);
    }

    return  (__pinCtrlCreate(pdtnDev));                                 /*  ���Ҳ����򴴽����ſ���      */
}
/*********************************************************************************************************
** ��������: API_PinBind
** ��������: �����豸���ڵ��е���Ϣ�����Ž����趨
** �䡡��  : pdevinstance     �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinBind (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DEV_PIN_INFO    pdevpininfo = LW_NULL;
    PLW_PINCTRL         ppinctl     = LW_NULL;
    PLW_PINCTRL_STATE   ppctlstate  = LW_NULL;
    PLW_DEVTREE_NODE    pdtnDev     = LW_NULL;
    INT                 iRet;

    if (!pdevinstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtnDev = pdevinstance->DEVHD_pdtnDev;

    pdevpininfo = __SHEAP_ZALLOC(sizeof(LW_DEV_PIN_INFO));
    if (!pdevpininfo) {
        pdevinstance->DEVHD_pdevpininfo = LW_NULL;
        return  (PX_ERROR);
    }
        
    ppinctl = API_PinCtrlGet(pdtnDev);                                  /*  ���豸�ڵ��л�ȡ������Ϣ    */
    if (!ppinctl) {
        PCTL_LOG(PCTL_LOG_LOG, "DTN %s has no pin info.\r\n",
                 pdtnDev->DTN_pcFullName);
        iRet = ERROR_NONE;
        goto __error_handle;
    }
    pdevpininfo->DEVPIN_ppinctl = ppinctl;

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_DEFAULT);  /*  ����Ĭ�ϵ��豸����״̬      */
    if (!ppctlstate) {
        iRet = PX_ERROR;
        goto __error_handle;
    }
    pdevpininfo->DEVPIN_ppctlstateDefault = ppctlstate;

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_INIT);     /*  ���� init �豸����״̬      */
    if (!ppctlstate) {
        ppctlstate = pdevpininfo->DEVPIN_ppctlstateDefault;             /*  û�� init ����״̬��Ĭ��    */
    } else {        
        pdevpininfo->DEVPIN_ppctlstateInit = ppctlstate;
    }

    iRet = API_PinCtrlStateSelect(ppinctl, ppctlstate);                 /*  ��������״̬                */
    if (iRet) {
        PCTL_LOG(PCTL_LOG_ERR, "DTN %s pin state select failed.\r\n", 
                 pdtnDev->DTN_pcFullName);
        iRet = PX_ERROR;
        goto __error_handle;
    }

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_IDLE);     /*  ���ҿ�������״̬            */
    if (ppctlstate) {
        pdevpininfo->DEVPIN_ppctlstateIdle = ppctlstate;
    }

    ppctlstate = API_PinCtrlStateFind(ppinctl, PINCTRL_STATE_SLEEP);    /*  ����˯������״̬            */
    if (ppctlstate) {
        pdevpininfo->DEVPIN_ppctlstateSleep = ppctlstate;
    }

    pdevinstance->DEVHD_pdevpininfo = pdevpininfo;

    return  (ERROR_NONE);

__error_handle:
    __SHEAP_FREE(pdevpininfo);
    pdevinstance->DEVHD_pdevpininfo = LW_NULL;
    
    return  (iRet);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
