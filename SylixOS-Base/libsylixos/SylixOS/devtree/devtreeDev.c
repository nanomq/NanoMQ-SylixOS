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
** ��   ��   ��: devtreeDev.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 06 �� 21 ��
**
** ��        ��: �豸���ӿ��豸��ؽӿ�ʵ��
**
** �޸ģ�
** 2019.10.22 ����ģ�͸��ģ��޸Ĵ��ļ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
#include "linux/bitops.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
  �豸����ƥ���
*********************************************************************************************************/
static LW_DEVTREE_TABLE      _G_dttDefaultDevBus[] = {
    { .DTITEM_cCompatible = "simple-bus", },
    { .DTITEM_cCompatible = "simple-mfd", },
    { .DTITEM_cCompatible = "isa",        },
    LW_DEVTREE_TABLE_END
};
/*********************************************************************************************************
  �豸���߲�ƥ���  
*********************************************************************************************************/
static LW_DEVTREE_TABLE      _G_dttSkipDevBus[] = {
    { .DTITEM_cCompatible = "operating-points-v2", },
    LW_DEVTREE_TABLE_END
};
/*********************************************************************************************************
** ��������: __deviceTreeIsCompatible
** ��������: ���ڵ���ƥ������Ƿ�ƥ��
** �䡡��  : pdtnDev         ָ���Ľڵ�
**           pcCompatible    ƥ�����ļ�������
**           pcType          ƥ��������������
**           pcName          ƥ��������������
** �䡡��  : ƥ���Ȩ��ֵ��Խ���ʾԽƥ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeIsCompatible (PLW_DEVTREE_NODE  pdtnDev,
                                      PCHAR             pcCompatible,
                                      PCHAR             pcType,
                                      PCHAR             pcName)
{
    PLW_DEVTREE_PROPERTY  pdtproperty;
    CPCHAR                pcTemp;
    INT                   iIndex = 0;
    INT                   iScore = 0;

    if (pcCompatible && pcCompatible[0]) {                              /*  ����ƥ�� "compatible"       */
        pdtproperty = API_DeviceTreePropertyFind(pdtnDev, "compatible", LW_NULL);
        for (pcTemp  = API_DeviceTreePropertyStringNext(pdtproperty, LW_NULL);
             pcTemp != LW_NULL;
             pcTemp  = API_DeviceTreePropertyStringNext(pdtproperty, pcTemp), iIndex++) {

            if (lib_strcasecmp(pcTemp, pcCompatible) == 0) {            /*  ���Դ�Сдʱ��ȫƥ��        */
                iScore = INT_MAX / 2 - (iIndex << 2);                   /*  ����ϴ��Ȩ��              */
                break;
            }
        }

        if (!iScore) {                                                  /*  ��� "compatible" ����ƥ��  */
            return  (0);                                                /*  ��ô����ֱ����Ϊ����ƥ��    */
        }
    }

    if (pcType && pcType[0]) {                                          /*  ���Ҫ��ƥ�� type ����      */
        if (!pdtnDev->DTN_pcType ||
            lib_strcasecmp(pcType, pdtnDev->DTN_pcType)) {              /*  type ���Ͳ���ƥ��           */
            return  (0);                                                /*  ������Ϊ����ƥ��            */
        }

        iScore += 2;                                                    /*  type ����ƥ�䣬Ȩ������     */
    }

    if (pcName && pcName[0]) {                                          /*  ���Ҫ��ƥ�� name           */
        if (!pdtnDev->DTN_pcName ||
            lib_strcasecmp(pcName, pdtnDev->DTN_pcName)) {              /*  name ����ƥ��               */
            return  (0);                                                /*  ������Ϊ����ƥ��            */
        }

        iScore++;                                                       /*  name ƥ�䣬Ȩ������         */
    }

    return  (iScore);
}
/*********************************************************************************************************
** ��������: __deviceTreeNodeMatch
** ��������: ���ڵ���ƥ����Ƿ�ƥ��
** �䡡��  : pdtnDev         ָ���Ľڵ�
**           pdttMatch       ƥ��ı��
** �䡡��  : ����ƥ������� ����ʵı����û��ƥ������� LW_NULL��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static const PLW_DEVTREE_TABLE  __deviceTreeNodeMatch (PLW_DEVTREE_NODE   pdtnDev,
                                                       PLW_DEVTREE_TABLE  pdttMatch)
{
    PLW_DEVTREE_TABLE  pdttBestMatch = LW_NULL;
    INT                iScore;
    INT                iBestScore      = 0;

    if (!pdttMatch) {
        return  (LW_NULL);
    }

    for (;
         pdttMatch->DTITEM_cName[0] ||
         pdttMatch->DTITEM_cType[0] ||
         pdttMatch->DTITEM_cCompatible[0];
         pdttMatch++) {                                                 /*  ����Ա�ƥ����еı���      */

        iScore = __deviceTreeIsCompatible(pdtnDev,
                                          pdttMatch->DTITEM_cCompatible,
                                          pdttMatch->DTITEM_cType,
                                          pdttMatch->DTITEM_cName);
        if (iScore > iBestScore) {                                      /*  �ҵ�����ʵı���            */
            pdttBestMatch   = pdttMatch;
            iBestScore      = iScore;
        }
    }

    return  (pdttBestMatch);
}
/*********************************************************************************************************
** ��������: __deviceTreeDevCreate
** ��������: �����豸
** �䡡��  : pdtnDev             ָ���Ľڵ�
** �䡡��  : �����ɹ��������豸ʵ����ʧ�ܣ����� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_DEV_INSTANCE  __deviceTreeDevCreate (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEV_INSTANCE   pdevInstance;

    pdevInstance = __SHEAP_ZALLOC(sizeof(LW_DEV_INSTANCE));
    if (LW_NULL == pdevInstance) {
        return  (LW_NULL);
    }
    
    pdevInstance->DEVHD_pdtnDev = pdtnDev;
    pdevInstance->DEVHD_pcName  = pdtnDev->DTN_pcFullName;

    API_PlatformDeviceRegister(pdevInstance);
    __deviceTreeNodeFlagSet(pdtnDev, OF_POPULATED);                     /*  ��ǽڵ㱻����              */
    
    return  (pdevInstance);
} 
/*********************************************************************************************************
** ��������: __deviceTreeDevPopulate
** ��������: �豸���豸�ڵ����豸ʵ������ƥ��
** �䡡��  : pdtnDev             ָ���Ľڵ�
**           pdttMatch          ָ����ƥ���
** �䡡��  : ƥ��ɹ�������ƥ����豸ʵ����ƥ��ʧ�ܣ����� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __deviceTreeDevPopulate (PLW_DEVTREE_NODE        pdtnDev,
                                    PLW_DEVTREE_TABLE       pdttMatch)
{
    INT                iRet = ERROR_NONE;
    PLW_DEVTREE_NODE   pdtnChild;
    PLW_DEV_INSTANCE   pdevInstance;
    PCHAR              pcCompat;

    pcCompat = API_DeviceTreePropertyGet(pdtnDev, "compatible", LW_NULL);
    if (pcCompat) {
        DEVTREE_MSG("%s() - node %s compatible : %s\r\n",
                    __func__, pdtnDev->DTN_pcFullName, pcCompat);
    } else {
        return  (ERROR_NONE);
    }

    if (__deviceTreeNodeFlagCheck(pdtnDev, OF_POPULATED_BUS)) {         /*  �Ѿ�ƥ����Ĳ���ƥ��        */
        DEVTREE_MSG("%s() - skipping %s, already populated\r\n",
                    __func__, pdtnDev->DTN_pcFullName);

        return  (ERROR_NONE);
    }

    if (__deviceTreeNodeMatch(pdtnDev, _G_dttSkipDevBus)) {             /*  ��������Ҫƥ��Ľڵ�        */
        return  (ERROR_NONE);
    }
    
    pdevInstance = __deviceTreeDevCreate(pdtnDev);                      /*  ����ƽ̨�豸                */
    if (LW_NULL == pdevInstance) {
        return  (ERROR_NONE);
    }

    if (!__deviceTreeNodeMatch(pdtnDev, pdttMatch)) {                   /*  �������߼�����ƥ��          */
        return  (ERROR_NONE);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  ���������ýڵ���ӽڵ�      */
        iRet = __deviceTreeDevPopulate(pdtnChild, pdttMatch);
        if (iRet) {
            break;
        }
    }

    return  (iRet);
}  
/*********************************************************************************************************
** ��������: API_DeviceTreeDrvMatchDev
** ��������: �豸����������ƥ��ӿ�
** �䡡��  : pdevInstance    �豸ʵ��ָ��
**           pdrvInstance    ����ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDrvMatchDev (PLW_DEV_INSTANCE   pdevInstance,
                                PLW_DRV_INSTANCE   pdrvInstance)
{
    if (!pdevInstance || !pdrvInstance) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__deviceTreeNodeMatch(pdevInstance->DEVHD_pdtnDev,
                              pdrvInstance->DRVHD_pMatchTable)) {       /*  ƥ���豸ʵ����ƥ���        */
        return  (ERROR_NONE);
    } 

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeDevPopulate
** ��������: �豸���豸�ڵ�ƥ�����
** �䡡��  : pdtnDev          ָ���Ľڵ�
**           pdttMatch        ָ����ƥ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDevPopulate (PLW_DEVTREE_NODE       pdtnDev,
                                PLW_DEVTREE_TABLE      pdttMatch)
{
    PLW_DEVTREE_NODE   pdtnChild;

    pdtnDev = pdtnDev ? pdtnDev : __deviceTreeFindNodeByPath("/");
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  ���������ýڵ���ӽڵ�      */
        if (__deviceTreeDevPopulate(pdtnChild, pdttMatch)) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeDevEarlyInit
** ��������: �豸��ǰ��ʼ���ӿ�
** �䡡��  : pdttMatch    ƥ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDevEarlyInit (PLW_DEVTREE_TABLE  pdttMatch)
{
    PLW_DEVTREE_NODE   pdtnDev;
    DEVTREE_INIT_FUNC  pfuncInitCb;
    PLW_DEVTREE_TABLE  pdttMatchTmp;

    _LIST_EACH_OF_ALLNODES(pdtnDev) {                                   /*  ������㣬�����жϿ�����    */
        if (!API_DeviceTreePropertyGet(pdtnDev, "compatible", LW_NULL)) {
            DEVTREE_MSG("%s() - skipping %s, no compatible prop\r\n",
                        __func__, pdtnDev->DTN_pcName);
            continue;
        }

        if (__deviceTreeNodeFlagCheck(pdtnDev, OF_POPULATED)) {         /*  �Ѿ�ƥ����Ĳ���ƥ��        */
            DEVTREE_MSG("%s() - skipping %s, already populated\r\n",
                        __func__, pdtnDev->DTN_pcName);
            continue;
        }

        pdttMatchTmp = __deviceTreeNodeMatch(pdtnDev, pdttMatch);
        if (LW_NULL != pdttMatchTmp) {
            __deviceTreeNodeFlagSet(pdtnDev, OF_POPULATED);             /*  ��ǽڵ㱻����              */
            pfuncInitCb = pdttMatchTmp->DTITEM_pvData;
            if (pfuncInitCb) {                                          /*  ����ҵ�ƥ����              */
                pfuncInitCb(pdtnDev);                                   /*  ִ���豸ʵ���� Probe ����   */
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeDefaultPopulate
** ��������: �豸������ڵ���س�ʼ�����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeDefaultPopulate (VOID)
{
    DEVTREE_MSG("Device populate from root.\r\n");
    
    return  (API_DeviceTreeDevPopulate(NULL, _G_dttDefaultDevBus));
}
/*********************************************************************************************************
** ��������: API_DeviceTreeIsCompatible
** ��������: ���ڵ���ƥ������Ƿ�ƥ��
** �䡡��  : pdtnDev      ָ���Ľڵ�
**           pcCompat     ƥ�����ļ�������
** �䡡��  : ƥ���Ȩ��ֵ��Խ���ʾԽƥ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIsCompatible (PLW_DEVTREE_NODE  pdtnDev,
                                 PCHAR             pcCompat)
{
    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (0);
    }

    return  (__deviceTreeIsCompatible(pdtnDev, pcCompat, LW_NULL, LW_NULL));
}
/*********************************************************************************************************
** ��������: API_DeviceTreeDevGetMatchData
** ��������: �豸���豸�ڵ��ȡƥ�������
** �䡡��  : pdevInstance     �豸�ڵ�
** �䡡��  : ƥ�������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PVOID  API_DeviceTreeDevGetMatchData (PLW_DEV_INSTANCE  pdevInstance)
{
    PLW_DEVTREE_TABLE  pdttMatch;

    if (!pdevInstance) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pdttMatch = __deviceTreeNodeMatch(pdevInstance->DEVHD_pdtnDev,
                                      pdevInstance->DEVHD_pdrvinstance->DRVHD_pMatchTable);
    if (!pdttMatch) {
        return  (LW_NULL);
    }

    return  ((PVOID)pdttMatch->DTITEM_pvData);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeDevGetMatchTable
** ��������: �ж��豸���ڵ���ָ����ƥ���������Ƿ���ƥ����
** �䡡��  : pdttMatches     ƥ������
**           pdtnDev         ָ���Ľڵ�
** �䡡��  : ����ƥ�����������ʵı����û��ƥ������� LW_NULL��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_TABLE  API_DeviceTreeDevGetMatchTable (PLW_DEVTREE_TABLE  pdttMatches,
                                                   PLW_DEVTREE_NODE   pdtnDev)
{
    if (!pdtnDev || !pdttMatches) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    return  (__deviceTreeNodeMatch(pdtnDev, pdttMatches));
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
