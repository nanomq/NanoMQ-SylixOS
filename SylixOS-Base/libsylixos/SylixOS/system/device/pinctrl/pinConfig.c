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
** ��   ��   ��: pinConfig.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 12 ��
**
** ��        ��: pinctrl ��ϵͳ�е���������ʵ��
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
** ��������: API_PinConfigApply
** ��������: Ӧ��ָ������������
** �䡡��  : ppinctlsetting        ָ������������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinConfigApply (PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV  ppinctldev;
    PLW_PINCONF_OPS  ppinconfops;
    INT              iRet;

    if (!ppinctlsetting ||
        !ppinctlsetting->PCTLS_ppinctldev ||
        !ppinctlsetting->PCTLS_ppinctldev->PCTLD_ppinctldesc) {         /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev  = ppinctlsetting->PCTLS_ppinctldev;
    ppinconfops = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinconfops;
    if (!ppinconfops) {                                                 /*  ���ú�����Ϊ��              */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    switch (ppinctlsetting->PCTLS_pinctlmaptype) {

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  ���õ�������                */
        if (!ppinconfops->pinConfigSet) {
            _ErrorHandle(ENOSYS);
            return  (PX_ERROR);
        }

        iRet = ppinconfops->pinConfigSet(ppinctldev,
                                         ppinctlsetting->set_uiGroupOrPin,
                                         ppinctlsetting->set_pulConfigs,
                                         ppinctlsetting->set_uiNumConfigs);
        if (iRet < 0) {
            return  (iRet);
        }
        break;

    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  ���������������            */
        if (!ppinconfops->pinConfigGroupSet) {
            _ErrorHandle(ENOSYS);
            return  (PX_ERROR);
        }

        iRet = ppinconfops->pinConfigGroupSet(ppinctldev,
                                              ppinctlsetting->set_uiGroupOrPin,
                                              ppinctlsetting->set_pulConfigs,
                                              ppinctlsetting->set_uiNumConfigs);
        if (iRet < 0) {
            return  (iRet);
        }
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PinConfigMapToSetting
** ��������: ������ӳ��ṹת��Ϊ��������
** �䡡��  : ppinctrlmap           ����ת��������ӳ��ṹ
**           ppinctlsetting        ת��������������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinConfigMapToSetting (PLW_PINCTRL_MAP  ppinctrlmap, PLW_PINCTRL_SETTING  ppinctlsetting)
{
    PLW_PINCTRL_DEV  ppinctldev;
    INT              iPin;

    if (!ppinctrlmap || !ppinctlsetting) {                              /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctldev = ppinctlsetting->PCTLS_ppinctldev;

    switch (ppinctlsetting->PCTLS_pinctlmaptype) {

    case PIN_MAP_TYPE_CONFIGS_PIN:                                      /*  ת���������Ź�������        */
        iPin = API_PinCtrlPinGetByName(ppinctldev,
                                       ppinctrlmap->map_pcGroupOrPin);  /*  �������ƻ�ȡ�������        */
        if (iPin < 0) {
            PCTL_LOG(PCTL_LOG_BUG, "map pin config \"%s\" error\r\n",
                     ppinctrlmap->map_pcGroupOrPin);
            return  (iPin);
        }
        ppinctlsetting->set_uiGroupOrPin = iPin;
        break;

    case PIN_MAP_TYPE_CONFIGS_GROUP:                                    /*  ת���������Ź�������        */
        iPin = API_PinCtrlGroupSelectorGet(ppinctldev,
                                           ppinctrlmap->map_pcGroupOrPin);
                                                                        /*  �������ƻ�ȡ���������      */
        if (iPin < 0) {
            PCTL_LOG(PCTL_LOG_BUG, "map group config \"%s\" error\r\n",
                     ppinctrlmap->map_pcGroupOrPin);
            return  (iPin);
        }
        ppinctlsetting->set_uiGroupOrPin = iPin;
        break;

    default:
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinctlsetting->set_uiNumConfigs = ppinctrlmap->map_uiNumConfigs;   /*  �������Ź�����������        */
    ppinctlsetting->set_pulConfigs   = ppinctrlmap->map_pulConfigs;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_PinConfigMapValidate
** ��������: ����ӳ��ṹ��Ч�Լ��
** �䡡��  : ppinctrlmap   ���ڼ�������ӳ��ṹ
**           i             ����ӳ��ṹԪ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_PinConfigMapValidate (PLW_PINCTRL_MAP  ppinctrlmap, INT  i)
{
    if (!ppinctrlmap) {                                                 /*  �������                    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!ppinctrlmap->map_pcGroupOrPin) {
        PCTL_LOG(PCTL_LOG_BUG, "register map %s (%d) failed: no group/pin given\r\n",
                 ppinctrlmap->PCTLM_pcName, i);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!ppinctrlmap->map_uiNumConfigs ||
        !ppinctrlmap->map_pulConfigs) {
        PCTL_LOG(PCTL_LOG_BUG, "register map %s (%d) failed: no configs given\r\n",
                 ppinctrlmap->PCTLM_pcName, i);
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
