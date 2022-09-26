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
** ��   ��   ��: devtreeProperty.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 07 �� 30 ��
**
** ��        ��: �豸�����Զ�ȡ�ӿ�
*********************************************************************************************************/
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
** ��������: __deviceTreePropertyValueOfSize
** ��������: ���豸�ڵ����������ԣ���ȡ���Ե���Ч��С������ȡ���Ե�ֵ
** �䡡��  : pdtnDev        ���ڲ��ҵ��豸���ڵ�
**           pcPropname     ��������
**           uiMin          ����ֵ������С��Чֵ
**           uiMax          ����ֵ���������Чֵ
**           pstLen         ����ֵʵ����Ч�ĳ���
** �䡡��  : ERROR_CODE �� ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __deviceTreePropertyValueOfSize (const PLW_DEVTREE_NODE  pdtnDev,
                                               CPCHAR                  pcPropname,
                                               UINT32                  uiMin,
                                               UINT32                  uiMax,
                                               size_t                 *pstLen)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropname, LW_NULL);

    if (!pdtprop) {
        return  (PVOID)(-EINVAL);
    }

    if (!pdtprop->DTP_pvValue) {
        return  (PVOID)(-ENODATA);
    }

    if (pdtprop->DTP_iLength < uiMin) {
        return  (PVOID)(-EOVERFLOW);
    }

    if (uiMax && pdtprop->DTP_iLength > uiMax) {
        return  (PVOID)(-EOVERFLOW);
    }

    if (pstLen) {
        *pstLen = pdtprop->DTP_iLength;
    }

    return  (pdtprop->DTP_pvValue);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeNodeIsOkay
** ��������: �鿴ĳ���豸���ڵ��Ƿ�Ϊ okay ״̬
** �䡡��  : pdtnDev          �豸���ڵ�
** �䡡��  : LW_TRUE ��ʾΪ okay��LW_FALSE ��ʾ��Ϊ okay
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
BOOL  API_DeviceTreeNodeIsOkay (PLW_DEVTREE_NODE  pdtnDev)
{
    CPCHAR  pcStatus;
    INT     iStatLen;

    if (!pdtnDev) {
        return  (LW_FALSE);
    }

    pcStatus = API_DeviceTreePropertyGet(pdtnDev, "status", &iStatLen);
    if (!pcStatus) {                                                    /*  û�� status ���ԵĶ�Ϊ okay */
        return  (LW_TRUE);
    }

    if (iStatLen > 0) {
        if (!strcmp(pcStatus, "ok") || !strcmp(pcStatus, "okay")) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeNodeIsOkayByOffset
** ��������: ͨ�� offset ָ���鿴ĳ���豸���ڵ��Ƿ�Ϊ okay ״̬
** �䡡��  : pvDevTree       �豸������ַ
**           iOffset         �ڵ�ƫ��
** �䡡��  : LW_TRUE ��ʾΪ okay��LW_FALSE ��ʾ��Ϊ okay
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
BOOL  API_DeviceTreeNodeIsOkayByOffset (PVOID  pvDevTree, INT  iOffset)
{
    CPCHAR  pcStatus;

    if (!pvDevTree) {
        _ErrorHandle(EINVAL);
        return  (LW_FALSE);
    }

    pcStatus = fdt_getprop(pvDevTree, iOffset, "status", LW_NULL);
    if (!pcStatus) {                                                    /*  û�� status ���ԵĶ�Ϊ okay */
        return  (LW_TRUE);
    }

    if (!strcmp(pcStatus, "ok") || !strcmp(pcStatus, "okay")) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyFind
** ��������: �������Խڵ�
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           piLen          ��ȡ������ֵ����
** �䡡��  : ���Խڵ� �� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_PROPERTY  API_DeviceTreePropertyFind (PLW_DEVTREE_NODE  pdtnDev,
                                                  CPCHAR            pcPropname,
                                                  INT              *piLen)
{
    PLW_DEVTREE_PROPERTY  pdtprop;

    if (!pdtnDev) {
        return  (LW_NULL);
    }

    for (pdtprop = pdtnDev->DTN_pdtpproperties;
         pdtprop;
         pdtprop = pdtprop->DTP_pdtpNext) {                             /*  �����ڵ�����Խṹ          */

        if (lib_strcmp(pdtprop->DTP_pcName, pcPropname) == 0) {
            if (piLen) {
                *piLen = pdtprop->DTP_iLength;
            }
            break;
        }
    }

    return  (pdtprop);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyU32VaraiableArrayRead
** ��������: ��ȡ U32 Array ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           puiOutValue    ����������
**           stMin          ����ֵ������С��Чֵ
**           stMax          ����ֵ���������Чֵ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32VaraiableArrayRead (const PLW_DEVTREE_NODE  pdtnDev,
                                                        CPCHAR            pcPropname,
                                                        UINT32           *puiOutValue,
                                                        size_t            stMin,
                                                        size_t            stMax)
{
    size_t         stSize;
    size_t         stCount;
    const UINT32  *puiVal = __deviceTreePropertyValueOfSize(pdtnDev,
                                                            pcPropname,
                                                            (stMin * sizeof(UINT32)),
                                                            (stMax * sizeof(UINT32)),
                                                            &stSize);

    if ((ULONG)puiVal >= (ULONG)-ERRMAX) {
        return  (LONG)(puiVal);
    }

    if (!stMax) {
        stSize = stMin;
    } else {
        stSize /= sizeof(UINT32);
    }

    stCount = stSize;
    while (stCount--) {
        *puiOutValue++ = BE32_TO_CPU(puiVal++);
    }

    return  (stSize);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyU32ArrayRead
** ��������: ��ȡ U32 Array ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           puiOutValue    ����������
**           stSize         Array �� size
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32ArrayRead (PLW_DEVTREE_NODE  pdtnDev,
                                         CPCHAR            pcPropname,
                                         UINT32           *puiOutValue,
                                         size_t            stSize)
{
    INT  iRet = API_DeviceTreePropertyU32VaraiableArrayRead(pdtnDev,
                                                            pcPropname,
                                                            puiOutValue,
                                                            stSize,
                                                            0);
    if (iRet >= 0) {
        return  (ERROR_NONE);
    } else {
        return  (iRet);
    }
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyU32Read
** ��������: ��ȡ U32 ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           puiOutValue    ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32Read (PLW_DEVTREE_NODE  pdtnDev,
                                    CPCHAR            pcPropname,
                                    UINT32           *puiOutValue)
{
    return  (API_DeviceTreePropertyU32ArrayRead(pdtnDev, pcPropname, puiOutValue, 1));
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyU32IndexRead
** ��������: ����Ŷ�ȡ U32 ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           uiIndex        ָ�������
**           puiOutValue    ����������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyU32IndexRead (PLW_DEVTREE_NODE  pdtnDev,
                                         CPCHAR            pcPropname,
                                         UINT32            uiIndex,
                                         UINT32           *puiOutValue)
{
    const UINT32  *puiVal = __deviceTreePropertyValueOfSize(pdtnDev,
                                                            pcPropname,
                                                            ((uiIndex + 1) * sizeof(UINT32)),
                                                            0,
                                                            LW_NULL);

    if ((ULONG)puiVal >= (ULONG)-ERRMAX) {
        return  (INT)(ULONG)(puiVal);
    }

    *puiOutValue = BE32_TO_CPU(((UINT32 *)puiVal) + uiIndex);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyU32Next
** ��������: ��˳���ȡ U32 ���͵���һ������ֵ
** �䡡��  : pdtprop        ��ǰ�豸�����Խڵ�
**           puiCur         ��ǰ���Ե�ַ
**           puiOut         ����������
** �䡡��  : ���º�����Ե�ַ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
UINT32*  API_DeviceTreePropertyU32Next (PLW_DEVTREE_PROPERTY  pdtprop,
                                        UINT32               *puiCur,
                                        UINT32               *puiOut)
{
    PVOID  pvCur = puiCur;

    if (!pdtprop) {
        return  (LW_NULL);
    }

    if (!puiCur) {
        puiCur = pdtprop->DTP_pvValue;
        goto  __out_val;
    }

    pvCur += sizeof(PVOID);
    if (pvCur >= (pdtprop->DTP_pvValue + pdtprop->DTP_iLength)) {
        return  (LW_NULL);
    }

__out_val:
    *puiOut = BE32_TO_CPU(puiCur);

    return  (pvCur);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyStringHelperRead
** ��������: ���� string ���͵�����ֵ
** �䡡��  : pdtnDev        ���ڻ�ȡ���Ե��豸���ڵ�
**           pcPropname     ���ҵ���������
**           ppcOutStrs     ������ַ���ָ������
**           stSize         ��ȡ������Ԫ������
**           iSkip          ��ͷ�������ַ�������
**           piCount        ��ȡ string �������Գ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��: ��Ҫֱ�ӵ��øýӿڣ�����ͨ�� API_DeviceTreePropertyStringRead* �����Ľӿ�����ӵ��øýӿ�
**
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringHelperRead (PLW_DEVTREE_NODE  pdtnDev,
                                             CPCHAR            pcPropName,
                                             CPCHAR           *ppcOutStrs,
                                             size_t            stSize,
                                             INT               iSkip,
                                             INT              *piCount)
{
    PLW_DEVTREE_PROPERTY  pdtprop;
    INT                   l;
    INT                   i;
    CPCHAR                pcTmp;
    CPCHAR                pcEnd;

    pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);
    if (!pdtprop) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pdtprop->DTP_pvValue) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    pcTmp = pdtprop->DTP_pvValue;
    pcEnd = pcTmp + pdtprop->DTP_iLength;

    for (i = 0;
         (pcTmp < pcEnd) && (!ppcOutStrs || (i < iSkip + stSize));
         i++, pcTmp += l) {

        l = lib_strnlen(pcTmp, pcEnd - pcTmp) + 1;
        if ((pcTmp + l) > pcEnd) {
            _ErrorHandle(EILSEQ);
            return  (PX_ERROR);
        }

        if (ppcOutStrs && (i >= iSkip)) {
            *ppcOutStrs++ = pcTmp;
        }
    }

    i -= iSkip;

    if (i <= 0) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    if (piCount) {
        *piCount = i;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyStringIndexRead
** ��������: ��ȡָ����ŵ� String ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropName     ��������
**           iIndex         ָ�������
**           ppcOutPut      ��ȡ���������ַ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringIndexRead (const PLW_DEVTREE_NODE  pdtnDev,
                                            CPCHAR                  pcPropName,
                                            INT                     iIndex,
                                            CPCHAR                 *ppcOutPut)
{
    return  (API_DeviceTreePropertyStringHelperRead(pdtnDev, pcPropName, ppcOutPut, 1, iIndex, LW_NULL));
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyStringCount
** ��������: ��һ���ɶ��� String ������ɵ�����ֵ�л�ȡ String �ĸ���
** �䡡��  : pdtnDev         �豸���ڵ�
**           pcPropName      ��������
** �䡡��  : ��ȡ�� String ����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringCount (const PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcPropName)
{
    INT  iCount;
    INT  iRet;

    iRet = API_DeviceTreePropertyStringHelperRead(pdtnDev, pcPropName, LW_NULL, 0, 0, &iCount);
    if (iRet) {
        return  (0);
    } else {
        return  (iCount);
    }
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyStringNext
** ��������: ��ȡ��һ������Ϊ  String  ������ֵ
** �䡡��  : pdtprop        ���Խڵ�
**           pcCur          ��ǰ String ��������ָ�룬Ϊ NULL ʱ��õ�ǰ String ����ֵ
** �䡡��  : ��ȡ���� String ����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
CPCHAR  API_DeviceTreePropertyStringNext (PLW_DEVTREE_PROPERTY  pdtprop, CPCHAR  pcCur)
{
    CPVOID  pvCur = pcCur;

    if (!pdtprop) {
        return  (LW_NULL);
    }

    if (!pcCur) {                                                       /*  Ϊ��ʱ�����ص�ǰ����ֵ      */
        return  (pdtprop->DTP_pvValue);
    }

    pvCur += lib_strlen(pcCur) + 1;
    if (pvCur >= (pdtprop->DTP_pvValue + pdtprop->DTP_iLength)) {
        return  (LW_NULL);
    }

    return  (pvCur);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyStringRead
** ��������: ��ȡ String ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           ppcOutString   ��ȡ���� String ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringRead (PLW_DEVTREE_NODE  pdtnDev,
                                       CPCHAR            pcPropName,
                                       CPCHAR           *ppcOutString)
{
    const PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);

    if (!pdtprop) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pdtprop->DTP_pvValue) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    if (lib_strnlen(pdtprop->DTP_pvValue, pdtprop->DTP_iLength) >= pdtprop->DTP_iLength) {
        _ErrorHandle(EILSEQ);
        return  (PX_ERROR);
    }

    *ppcOutString = pdtprop->DTP_pvValue;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyStringMatch
** ��������: ����ֵ��ָ���ַ����ȽϺ���
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropname     ��������
**           pcString       ָ���ַ���
** �䡡��  : PX_ERROR �� �������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePropertyStringMatch (PLW_DEVTREE_NODE  pdtnDev,
                                        CPCHAR            pcPropName,
                                        CPCHAR            pcString)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);
    PCHAR                 pcPropTmp;
    PCHAR                 pcPropEnd;
    size_t                stLen;
    INT                   i;

    if (!pdtprop) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!pdtprop->DTP_pvValue) {
        _ErrorHandle(ENODATA);
        return  (PX_ERROR);
    }

    pcPropTmp = pdtprop->DTP_pvValue;
    pcPropEnd = pcPropTmp + pdtprop->DTP_iLength;

    for (i = 0; pcPropTmp < pcPropEnd; i++, pcPropTmp += stLen) {
        stLen = lib_strnlen(pcPropTmp, pcPropEnd - pcPropTmp) + 1;
        if ((pcPropTmp + stLen) > pcPropEnd) {
            _ErrorHandle(EILSEQ);
            return  (PX_ERROR);
        }

        if (lib_strcmp(pcString, pcPropTmp) == 0) {
            return  (i);
        }
    }

    _ErrorHandle(ENODATA);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyBoolRead
** ��������: ��ȡ BOOL ���͵�����ֵ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPropName     ��������
** �䡡��  : LW_TRUE �� LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
BOOL  API_DeviceTreePropertyBoolRead (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcPropName)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcPropName, LW_NULL);

    if (pdtprop) {
        return  (LW_TRUE);
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePropertyGet
** ��������: ͨ��ָ���ڵ��ָ������������ȡһ������ֵ
** �䡡��  : pdtnDev        ���ڻ�ȡ���Ե��豸���ڵ�
**           pcName         ���ҵ���������
**           piLen          ���Եĳ���
** �䡡��  : ����ֵ �� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PVOID  API_DeviceTreePropertyGet (PLW_DEVTREE_NODE  pdtnDev,
                                  CPCHAR            pcName,
                                  INT              *piLen)
{
    PLW_DEVTREE_PROPERTY  pdtprop = API_DeviceTreePropertyFind(pdtnDev, pcName, piLen);

    if (pdtprop) {
        return  (pdtprop->DTP_pvValue);
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeModaliasGet
** ��������: Ϊ�豸�ڵ��ҵ�һ�����ʵ�����
** �䡡��  : pdtnDev        ���ڻ�ȡ���Ե��豸���ڵ�
**           pcName         ������������ڴ�ָ��
**           iLen           ������������ڴ��С
** �䡡��  : �ҵ����� ERROR_NONE, δ�ҵ���������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeModaliasGet (PLW_DEVTREE_NODE  pdtnDev,
                                PCHAR             pcName,
                                INT               iLen)
{
    CPCHAR  pcCompatible;
    CPCHAR  pcStart;
    INT     iCpLen;
    
    pcCompatible = API_DeviceTreePropertyGet(pdtnDev, "compatible", &iCpLen);
    if (!pcCompatible || (lib_strlen(pcCompatible) > iCpLen)) {
        return  (ENODEV);
    }

    pcStart = lib_strchr(pcCompatible, ',');
    lib_strlcpy(pcName, pcStart ? (pcStart + 1) : pcCompatible, iLen);
    
    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
