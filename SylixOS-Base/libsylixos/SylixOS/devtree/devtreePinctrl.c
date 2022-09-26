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
** ��   ��   ��: devtreePinctrl.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 02 ��
**
** ��        ��: �豸���ӿ�������ؽӿ�ʵ��
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
** ��������: __deviceTreeMapFree
** ��������: �ͷ��豸��ʹ�õ�����ӳ��ṹ
** �䡡��  : ppinctrldev      ���ſ�����
**           ppinctrlmap      �ͷŵ�����ӳ��ṹ
**           uiNumMaps        ����ӳ��ṹ��Ԫ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __deviceTreeMapFree (PLW_PINCTRL_DEV       ppinctrldev,
                                  PLW_PINCTRL_MAP       ppinctrlmap,
                                  UINT                  uiNumMaps)
{
    PLW_PINCTRL_OPS  ppinctrlops;

    if (ppinctrldev) {
        ppinctrlops = ppinctrldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
        ppinctrlops->pinctrlMapFree(ppinctrldev, ppinctrlmap, uiNumMaps);

    } else {
        __SHEAP_FREE(ppinctrlmap);
    }
}
/*********************************************************************************************************
** ��������: __deviceTreeRememberOrFreeMap
** ��������: ���ӷ� DUMMY �����Ÿ���ӳ��ṹ
** �䡡��  : ppinctrl        ���ſ���
**           pcStateName     ������״̬
**           ppinctrldev     ���������ſ�����
**           ppinctrlmap     ����ӳ��ṹ
**           uiNumMaps       ����ӳ��ṹԪ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeRememberOrFreeMap (PLW_PINCTRL           ppinctrl,
                                           CPCHAR                pcStateName,
                                           PLW_PINCTRL_DEV       ppinctrldev,
                                           PLW_PINCTRL_MAP       ppinctrlmap,
                                           UINT                  uiNumMaps)
{
    PLW_PINCTRL_MAPS    ppinctlmaps;
    INT                 i;

    for (i = 0; i < uiNumMaps; i++) {                                   /*  �������ӳ��ṹ��ʣ�ಿ��  */
        ppinctrlmap[i].PCTLM_pdtnDev = ppinctrl->PCTL_pdtnDev;          /*  ��¼���������õ��豸���ڵ�  */
        ppinctrlmap[i].PCTLM_pcName  = pcStateName;                     /*  ��¼���������õ�״̬����    */
        if (ppinctrldev) {
            ppinctrlmap[i].PCTLM_pdtnCtrlNode = ppinctrldev->PCTLD_pdtnDev;
                                                                        /*  ��¼���ſ��������豸���ڵ�  */
        }
    }

    ppinctlmaps = (PLW_PINCTRL_MAPS)__SHEAP_ALLOC(sizeof(LW_PINCTRL_MAPS));
    if (!ppinctlmaps) {
        __deviceTreeMapFree(ppinctrldev, ppinctrlmap, uiNumMaps);
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    ppinctlmaps->PCTLM_ppinctldev   = ppinctrldev;                      /*  ��¼���ſ�����              */
    ppinctlmaps->PCTLM_ppinctlmaps  = ppinctrlmap;                      /*  ��¼����ӳ������            */
    ppinctlmaps->PCTLM_uiMapsNum    = uiNumMaps;                        /*  ��¼����ӳ������Ԫ�ظ���    */
    _List_Line_Add_Ahead(&ppinctlmaps->PCTLM_lineManage,
                         &ppinctrl->PCTL_plinemaps);                    /*  �������ӳ��ṹ�ڵ�        */

    return  (API_PinCtrlMapAdd(ppinctlmaps));                           /*  ע������ӳ��ṹ����        */
}
/*********************************************************************************************************
** ��������: __deviceTreeRememberDummyState
** ��������: �������������ӳ��ṹ
** �䡡��  : ppinctrl        ���ſ���
**           pcStateName     ������״̬
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeRememberDummyState (PLW_PINCTRL  ppinctrl, CPCHAR  pcStateName)
{
    PLW_PINCTRL_MAP       ppinctrlmap;

    ppinctrlmap = (PLW_PINCTRL_MAP)__SHEAP_ALLOC(sizeof(LW_PINCTRL_MAP));
    if (!ppinctrlmap) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    ppinctrlmap->PCTLM_pctlmaptype = PIN_MAP_TYPE_DUMMY_STATE;

    return  (__deviceTreeRememberOrFreeMap(ppinctrl, pcStateName, LW_NULL, ppinctrlmap, 1));
}
/*********************************************************************************************************
** ��������: __deviceTreeOneConfigMap
** ��������: ����һ���豸����������ת��Ϊ����ӳ��ṹ
** �䡡��  : ppinctrl        ���ſ���
**           pcStateName     ��Ӧ��״̬
**           pdtnConfig      �������õ��豸���ڵ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeOneConfigMap (PLW_PINCTRL           ppinctrl,
                                      CPCHAR                pcStateName,
                                      PLW_DEVTREE_NODE      pdtnConfig)
{
    PLW_DEVTREE_NODE      pdtnPinctrlDev;
    PLW_PINCTRL_DEV       ppinctrldev;
    PLW_PINCTRL_OPS       ppinctrlops;
    PLW_PINCTRL_MAP       ppinctrlmap = LW_NULL;
    UINT                  uiNunMaps;
    INT                   iRet;

    pdtnPinctrlDev = pdtnConfig;
    while (1) {
        pdtnPinctrlDev = pdtnPinctrlDev->DTN_pdtnparent;                /*  �������ſ������ڵ�          */
        if (!pdtnPinctrlDev ||                                          /*  ���û�и��ڵ�              */
            !pdtnPinctrlDev->DTN_pdtnparent) {                          /*  ���߸��ڵ�Ϊ���ڵ�          */
            _ErrorHandle(ERROR_DEVTREE_EPROBE_DEFER);
            return  (PX_ERROR);
        }

        ppinctrldev = API_PinCtrlDevGetByDevtreeNode(pdtnPinctrlDev);   /*  �ҵ����ſ�����              */
        if (ppinctrldev) {
            break;
        }

        if (pdtnPinctrlDev == ppinctrl->PCTL_pdtnDev) {                 /*  �����ſ��ƹ���������������  */
            _ErrorHandle(ENODEV);                                       /*  ������                      */
            return  (PX_ERROR);
        }
    }

    ppinctrlops = ppinctrldev->PCTLD_ppinctldesc->PCTLD_ppinctlops;
    if (!ppinctrlops->pinctrlMapCreate) {                               /*  ��û�ж����豸��ת������    */
        _ErrorHandle(ENODEV);
        return  (PX_ERROR);
    }

    iRet = ppinctrlops->pinctrlMapCreate(ppinctrldev,
                                         pdtnConfig,
                                         &ppinctrlmap,
                                         &uiNunMaps);                   /*  �������ſ����������ӿ�      */
    if (iRet < 0) {
        return  (iRet);
    }

    if (!ppinctrlmap) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (__deviceTreeRememberOrFreeMap(ppinctrl, pcStateName, ppinctrldev, ppinctrlmap, uiNunMaps));
}
/*********************************************************************************************************
** ��������: API_DeviceTreePinCtrlMapsFree
** ��������: �ͷ��豸��������ӳ��ṹ
** �䡡��  : ppinctrl         ���ſ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_DeviceTreePinCtrlMapsFree (PLW_PINCTRL  ppinctrl)
{
    PLW_PINCTRL_MAPS    pinctrlmaps;
    PLW_LIST_LINE       plineTemp;

    if (!ppinctrl) {
        return;
    }

    for (plineTemp  = ppinctrl->PCTL_plinemaps;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  �������ſ��Ƶ�ӳ������      */

        pinctrlmaps = _LIST_ENTRY(plineTemp, LW_PINCTRL_MAPS, PCTLM_lineManage);

        API_PinCtrlMapDel(pinctrlmaps->PCTLM_ppinctlmaps);              /*  ��ȫ���������Ƴ�            */

        _List_Line_Del(&pinctrlmaps->PCTLM_lineManage,
                       &ppinctrl->PCTL_plinemaps);                      /*  �����ſ����������Ƴ�        */

        __deviceTreeMapFree(pinctrlmaps->PCTLM_ppinctldev,
                            pinctrlmaps->PCTLM_ppinctlmaps,
                            pinctrlmaps->PCTLM_uiMapsNum);              /*  �ͷ�����ӳ���ڴ�            */
        __SHEAP_FREE(pinctrlmaps);
    }
}
/*********************************************************************************************************
** ��������: API_DeviceTreePinCtrlMapsCreate
** ��������: �����ſ��ƶ�Ӧ���豸������������ӳ��ṹ
** �䡡��  : ppinctrl        ���ſ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePinCtrlMapsCreate (PLW_PINCTRL   ppinctrl)
{
    PLW_DEVTREE_NODE      pdtnDev;
    PLW_DEVTREE_NODE      pdtnConfig;
    PLW_DEVTREE_PROPERTY  pdtproperty;
    CPCHAR                pcStateName;
    CHAR                  cPropName[30];
    UINT32               *puiList;
    UINT32                uiPhandle;
    INT                   iState = 0;
    INT                   iConfig;
    INT                   iSize;
    INT                   iRet;

    if (!ppinctrl) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtnDev = ppinctrl->PCTL_pdtnDev;

    while (1) {                                                         /*  ����Ų������ſ�������      */
        snprintf(cPropName, 30, "pinctrl-%d", iState);
        pdtproperty = API_DeviceTreePropertyFind(pdtnDev, cPropName, &iSize);
        if (!pdtproperty) {                                             /*  ����Ѿ��Ҳ�����Ӧ���      */
            if (0 == iState) {
                return  (PX_ERROR);
            }
            break;
        }

        puiList = pdtproperty->DTP_pvValue;                             /*  ��ȡ phandle                */
        iSize  /= sizeof(UINT32);

        iRet = API_DeviceTreePropertyStringIndexRead(pdtnDev,
                                                     "pinctrl-names",
                                                     iState,
                                                     &pcStateName);     /*  ���Ҷ�Ӧ��������            */
        if (iRet < 0) {                                                 /*  δ�ҵ���������ʱ�����Ʒ�ʽ  */
            pcStateName = pdtproperty->DTP_pcName + 8;
        }

        for (iConfig = 0; iConfig < iSize; iConfig++) {
            uiPhandle   = BE32_TO_CPU(puiList++);                       /*  �������ſ��ƻ�ȡ phandle    */
            pdtnConfig = API_DeviceTreeFindNodeByPhandle(uiPhandle);    /*  �� phandle �Ҷ�Ӧ�豸���ڵ� */
            if (!pdtnConfig) {
                DEVTREE_ERR("prop %s index %i invalid phandle\r\n",
                            pdtproperty->DTP_pcName, iConfig);
                iRet = -EINVAL;
                goto  __error_handle;
            }

            iRet = __deviceTreeOneConfigMap(ppinctrl,
                                            pcStateName,
                                            pdtnConfig);                /*  ���������ſ�������          */
            if (iRet < 0) {
                goto  __error_handle;
            }
        }

        if (!iSize) {                                                   /*  ������ſ���û������ֵ      */
            iRet = __deviceTreeRememberDummyState(ppinctrl, pcStateName);
            if (iRet < 0) {
                goto  __error_handle;
            }
        }

        iState++;
    }

    return  (ERROR_NONE);

__error_handle:
    API_DeviceTreePinCtrlMapsFree(ppinctrl);
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePinCtrlDevGet
** ��������: �����豸���ڵ��ȡ���ſ�����
** �䡡��  : pdtnDev      �豸���ڵ�
** �䡡��  : ���ſ�����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DEV  API_DeviceTreePinCtrlDevGet (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL_DEV  ppinctrldev;

    ppinctrldev = API_PinCtrlDevGetByDevtreeNode(pdtnDev);

    return  (ppinctrldev);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
