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
** ��   ��   ��: irqCtrlDev.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 06 �� 21 ��
**
** ��        ��: �жϿ������豸
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
#include "irqCtrlDev.h"
/*********************************************************************************************************
  ��������ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER  _G_plineIrqctrlDev;
/*********************************************************************************************************
** ��������: API_IrqCtrlDevCreate
** ��������: ����һ���жϿ�����
** �䡡��  : pcName           �жϿ�����������
**           pirqctrlfuncs    �жϿ������Ĳ�����
**           ulDirectMapMax   �жϿ�����֧��ֱ��ӳ�������жϺ�
**           ulIrqMax         �жϿ�������Ҫӳ������Ӳ���жϺ�
** �䡡��  : �жϿ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PLW_IRQCTRL_DEV  API_IrqCtrlDevCreate (CPCHAR             pcName,
                                       PLW_IRQCTRL_FUNCS  pirqctrlfuncs,
                                       ULONG              ulDirectMapMax,
                                       ULONG              ulIrqMax)
{
    PLW_IRQCTRL_DEV  pirqctrldev;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (ulDirectMapMax > ulIrqMax) {                                    /*  ���������Ϲ���              */
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pirqctrldev = (PLW_IRQCTRL_DEV)__SHEAP_ZALLOC(sizeof(LW_IRQCTRL_DEV) + lib_strlen(pcName));
    if (pirqctrldev == LW_NULL) {
        _ErrorHandle(ERROR_POWERM_FULL);
        return  (LW_NULL);
    }

    pirqctrldev->IRQCDEV_pirqctrlfuncs     = pirqctrlfuncs;             /*  �������ָ�뼯Ϊ��          */
    pirqctrldev->IRQCDEV_ulDirectMapIrqMax = ulDirectMapMax;
    pirqctrldev->IRQCDEV_ulIrqMax          = ulIrqMax;
    lib_strcpy(pirqctrldev->IRQCDEV_cName, pcName);

    if (ulDirectMapMax < ulIrqMax) {                                    /*  ��ʾ���ڲ�����Ҫ����ӳ��    */
        pirqctrldev->IRQCDEV_pulLinearMap = (ULONG *)__SHEAP_ZALLOC(ulIrqMax * sizeof(ULONG));
        if (pirqctrldev->IRQCDEV_pulLinearMap == LW_NULL) {
            _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
            goto  __error_handle;
        }
    }

    _List_Line_Add_Ahead(&pirqctrldev->IRQCDEV_lineManage, &_G_plineIrqctrlDev);

    return  (pirqctrldev);

__error_handle:
    __SHEAP_FREE(pirqctrldev);

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_IrqCtrlDevDelete
** ��������: ɾ��һ���жϿ����� (���Ƽ�ʹ�ô˺���)
** �䡡��  : pirqctrldev     �жϿ�����
** �䡡��  : ERROR or OK
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlDevDelete (PLW_IRQCTRL_DEV  pirqctrldev)
{
    if (!pirqctrldev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pirqctrldev->IRQCDEV_pulLinearMap) {
        __SHEAP_FREE(pirqctrldev->IRQCDEV_pulLinearMap);
    }

    _List_Line_Del(&pirqctrldev->IRQCDEV_lineManage, &_G_plineIrqctrlDev);
    __SHEAP_FREE(pirqctrldev);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_IrqCtrlDevFind
** ��������: ��ѯһ���жϿ�����
** �䡡��  : pcName        �жϿ�����������
** �䡡��  : �жϿ�����
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PLW_IRQCTRL_DEV  API_IrqCtrlDevFind (CPCHAR  pcName)
{
    PLW_LIST_LINE    plineTemp;
    PLW_IRQCTRL_DEV  pirqctrldev;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    for (plineTemp  = _G_plineIrqctrlDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pirqctrldev = _LIST_ENTRY(plineTemp, LW_IRQCTRL_DEV, IRQCDEV_lineManage);
        if (lib_strcmp(pirqctrldev->IRQCDEV_cName, pcName) == 0) {
            return  (pirqctrldev);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_IrqCtrlHwIrqMapToVector
** ��������: ��Ӳ���жϺ�ӳ�䵽����жϺ�
** �䡡��  : pirqctrldev     �жϿ�����
**           ulHwIrq         ��Ҫӳ���Ӳ���жϺ�
**           pulVector       ӳ���������жϺ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlHwIrqMapToVector (PLW_IRQCTRL_DEV      pirqctrldev,
                                  ULONG                ulHwIrq,
                                  ULONG               *pulVector)
{
    INT  iRet;

    if (pirqctrldev &&
        pirqctrldev->IRQCDEV_pirqctrlfuncs &&
        pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMap) {
        iRet = pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMap(pirqctrldev,
                                                                         ulHwIrq,
                                                                         pulVector);
    } else {
        iRet = PX_ERROR;
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_IrqCtrlFindVectorByHwIrq
** ��������: ����Ӳ���жϺŲ����Ѿ�ӳ�������жϺ�
** �䡡��  : pirqctrldev     �жϿ�����
**           ulHwIrq         Ӳ���жϺ�
**           pulVector       ����жϺ�
** �䡡��  : ����жϺţ�δ�ҵ�ʱ������ LW_CFG_INTER_INVALID
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlFindVectorByHwIrq (PLW_IRQCTRL_DEV      pirqctrldev,
                                   ULONG                ulHwIrq,
                                   ULONG               *pulVector)
{
    if (!pirqctrldev || !pulVector) {
        return  (PX_ERROR);
    }

    if (ulHwIrq < pirqctrldev->IRQCDEV_ulDirectMapIrqMax) {             /*  ֧��ֱ��ӳ��                */
        *pulVector = ulHwIrq;
        return  (ERROR_NONE);
    }

    if (ulHwIrq < pirqctrldev->IRQCDEV_ulIrqMax) {                      /*  ��������ӳ��                */
        *pulVector = pirqctrldev->IRQCDEV_pulLinearMap[ulHwIrq];
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_IrqCtrlDevtreeNodeMatch
** ��������: �����豸���ڵ���Ҷ�Ӧ���жϿ�����
** �䡡��  : pdtnDev         �豸���ڵ�
** �䡡��  : �жϿ����� �� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
PLW_IRQCTRL_DEV  API_IrqCtrlDevtreeNodeMatch (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_LIST_LINE    plineTemp;
    PLW_IRQCTRL_DEV  pirqctrldev = LW_NULL;
    BOOL             bMatch;

    for (plineTemp  = _G_plineIrqctrlDev;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pirqctrldev = _LIST_ENTRY(plineTemp, LW_IRQCTRL_DEV, IRQCDEV_lineManage);
        if (pirqctrldev->IRQCDEV_pirqctrlfuncs &&
            pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMatch) {

            bMatch = pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlMatch(pirqctrldev, pdtnDev);
            if (bMatch) {
                return  (pirqctrldev);
            }
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_IrqCtrlDevtreeTrans
** ��������: �����豸������ת��ΪӲ���жϺ�
** �䡡��  : pirqctrldev     �жϿ�����
**           pdtpaArgs       �豸������
**           pulHwIrq        ת����Ӳ���жϺ�
**           uiType          ת�����ж�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_IrqCtrlDevtreeTrans (PLW_IRQCTRL_DEV           pirqctrldev,
                              PLW_DEVTREE_PHANDLE_ARGS  pdtpaArgs,
                              ULONG                    *pulHwIrq,
                              UINT                     *uiType)
{
    INT  iRet;

    if (!pirqctrldev || !pdtpaArgs || !pulHwIrq || !uiType) {
        return  (PX_ERROR);
    }

    if (pirqctrldev->IRQCDEV_pirqctrlfuncs &&
        pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlTrans) {
        iRet = pirqctrldev->IRQCDEV_pirqctrlfuncs->IRQCF_pfuncIrqCtrlTrans(pirqctrldev,
                                                                           pdtpaArgs,
                                                                           pulHwIrq,
                                                                           uiType);
    } else {
        *pulHwIrq = pdtpaArgs->DTPH_uiArgs[0];
        iRet      = ERROR_NONE;
    }

    return  (iRet);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
