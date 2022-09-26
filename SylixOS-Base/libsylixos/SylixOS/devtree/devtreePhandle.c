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
** ��   ��   ��: devtreePhandle.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 07 �� 30 ��
**
** ��        ��: �豸�� phandle ��ؽӿ�
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
** ��������: __deviceTreePhandleIteratorArgs
** ��������: ��ȡ��ǰ������ָʾ�� phandle ����
** �䡡��  : pdtpiItor      ������ָ��
**           puiArgs        ��ȡ�Ĳ���
**           iSize          �����Ĵ�С
** �䡡��  : ����ռ�õĴ�С
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreePhandleIteratorArgs (PLW_DEVTREE_PHANDLE_ITERATOR  pdtpiItor,
                                             UINT32                       *puiArgs,
                                             INT                           iSize)
{
    INT  iCount;
    INT  i;

    iCount = pdtpiItor->DTPHI_uiCurCount;

    if (iSize < iCount) {
        iCount = iSize;
    }

    for (i = 0; i < iCount; i++) {
        puiArgs[i] = BE32_TO_CPU(pdtpiItor->DTPHI_puiCurrent++);
    }

    return  (iCount);
}

/*********************************************************************************************************
** ��������: API_DeviceTreePhandleIteratorInit
** ��������: ��ʼ�� phandle �ĵ�����
** �䡡��  : pdtpiItor     ��Ҫ��ʼ���ĵ�����
**           pdtnDev       �豸���ڵ�
**           pcListName    ��������
**           pcCellsName   cell ����
**           iCellCount    cell ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePhandleIteratorInit (PLW_DEVTREE_PHANDLE_ITERATOR  pdtpiItor,
                                        const PLW_DEVTREE_NODE        pdtnDev,
                                        CPCHAR                        pcListName,
                                        CPCHAR                        pcCellsName,
                                        INT                           iCellCount)
{
    UINT32  *puiList;
    INT      iSize;

    if (!pdtpiItor) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    lib_bzero(pdtpiItor, sizeof(LW_DEVTREE_PHANDLE_ITERATOR));

    puiList = API_DeviceTreePropertyGet(pdtnDev, pcListName, &iSize);
    if (!puiList) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    pdtpiItor->DTPHI_pcCellsName   = pcCellsName;
    pdtpiItor->DTPHI_iCellCount    = iCellCount;
    pdtpiItor->DTPHI_pdtnParent    = pdtnDev;
    pdtpiItor->DTPHI_puiListEnd    = puiList + iSize / sizeof(UINT32);
    pdtpiItor->DTPHI_puiPhandleEnd = puiList;
    pdtpiItor->DTPHI_puiCurrent    = puiList;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePhandleIteratorNext
** ��������: ��ȡ��һ��������
** �䡡��  : pdtpiItor           ��ǰ�ĵ�����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePhandleIteratorNext (PLW_DEVTREE_PHANDLE_ITERATOR  pdtpiItor)
{
    UINT32  uiCount = 0;

    if (!pdtpiItor) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pdtpiItor->DTPHI_pdtnDev) {
        pdtpiItor->DTPHI_pdtnDev = LW_NULL;
    }

    if (!pdtpiItor->DTPHI_puiCurrent ||
        (pdtpiItor->DTPHI_puiPhandleEnd >= pdtpiItor->DTPHI_puiListEnd)) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    pdtpiItor->DTPHI_puiCurrent = pdtpiItor->DTPHI_puiPhandleEnd;
    pdtpiItor->DTPHI_uiPhandle  = BE32_TO_CPU(pdtpiItor->DTPHI_puiCurrent++);

    if (pdtpiItor->DTPHI_uiPhandle) {
        pdtpiItor->DTPHI_pdtnDev = API_DeviceTreeFindNodeByPhandle(pdtpiItor->DTPHI_uiPhandle);

        if (pdtpiItor->DTPHI_pcCellsName) {
            if (!pdtpiItor->DTPHI_pdtnDev) {
                goto  __error_handle;
            }

            if (API_DeviceTreePropertyU32Read(pdtpiItor->DTPHI_pdtnDev,
                                              pdtpiItor->DTPHI_pcCellsName,
                                              &uiCount)) {
                goto  __error_handle;
            }
        } else {
            uiCount = pdtpiItor->DTPHI_iCellCount;
        }

        if ((pdtpiItor->DTPHI_puiCurrent + uiCount) > pdtpiItor->DTPHI_puiListEnd) {
            goto  __error_handle;
        }
    }

    pdtpiItor->DTPHI_puiPhandleEnd = pdtpiItor->DTPHI_puiCurrent + uiCount;
    pdtpiItor->DTPHI_uiCurCount    = uiCount;

    return  (ERROR_NONE);

__error_handle:
    if (pdtpiItor->DTPHI_pdtnDev) {
        pdtpiItor->DTPHI_pdtnDev = LW_NULL;
    }

    _ErrorHandle(EINVAL);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePhandleCountWithArgs
** ��������: ��ȡ�б��� phandle ����
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcListName     �б�����
**           pcCellsName    cells ����
** �䡡��  : phandle ���� or ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePhandleCountWithArgs (const PLW_DEVTREE_NODE     pdtnDev,
                                         CPCHAR                     pcListName,
                                         CPCHAR                     pcCellsName)
{
    LW_DEVTREE_PHANDLE_ITERATOR     dtpiItor;
    PLW_DEVTREE_PROPERTY            pProp;
    INT                             iSize;
    INT                             iRet;
    INT                             iSum = 0;

    if (!pcCellsName) {
        pProp = API_DeviceTreePropertyFind(pdtnDev, pcListName, &iSize);
        if (!pProp) {
            return  (PX_ERROR);
        }

        return  (iSize / sizeof(INT));
    }

    iRet = API_DeviceTreePhandleIteratorInit(&dtpiItor, pdtnDev, pcListName, pcCellsName, -1);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    while (API_DeviceTreePhandleIteratorNext(&dtpiItor) == ERROR_NONE) {
        iSum++;
    }

    if (errno != ENOENT) {
        return  (PX_ERROR);
    }

    return  (iSum);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePhandleParseFixedArgs
** ��������: �������й̶����������� phandle
** �䡡��  : pdtnDev       �豸���ڵ�
**           pcListName    �б�����
**           pcCellsName   cells ����
**           iCellCount    cells ����
**           iIndex        ���
**           pdtpaOutArgs  �������� phandle ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePhandleParseFixedArgs (const PLW_DEVTREE_NODE     pdtnDev,
                                          CPCHAR                     pcListName,
                                          CPCHAR                     pcCellsName,
                                          INT                        iCellCount,
                                          INT                        iIndex,
                                          PLW_DEVTREE_PHANDLE_ARGS   pdtpaOutArgs)
{
    LW_DEVTREE_PHANDLE_ITERATOR   dtpiItor;
    INT                           iRet;
    INT                           iC;
    INT                           iCurIndex = 0;

    _LIST_EACH_PHANDLE(&dtpiItor, iRet, pdtnDev, pcListName, pcCellsName, iCellCount) {
        if (iCurIndex == iIndex) {
            if (!dtpiItor.DTPHI_uiPhandle) {
                _ErrorHandle(EINVAL);
                goto  __error_handle;
            }

            if (pdtpaOutArgs) {
                iC = __deviceTreePhandleIteratorArgs(&dtpiItor,
                                                     pdtpaOutArgs->DTPH_uiArgs,
                                                     MAX_PHANDLE_ARGS);
                pdtpaOutArgs->DTPH_pdtnDev    = dtpiItor.DTPHI_pdtnDev;
                pdtpaOutArgs->DTPH_iArgsCount = iC;
            }

            return  (ERROR_NONE);
        }

        iCurIndex++;
    }

__error_handle:
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_DeviceTreePhandleParseWithArgs
** ��������: ���в����� phandle ����
** �䡡��  : pdtnDev       �豸���ڵ�
**           pcListName    �б�����
**           pcCellsName   �ڵ�����
**           iIndex        ���
**           pdtpaOutArgs  �������� phandle ����
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePhandleParseWithArgs (const PLW_DEVTREE_NODE     pdtnDev,
                                         CPCHAR                     pcListName,
                                         CPCHAR                     pcCellsName,
                                         INT                        iIndex,
                                         PLW_DEVTREE_PHANDLE_ARGS   pdtpaOutArgs)
{
    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (iIndex < 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (API_DeviceTreePhandleParseFixedArgs(pdtnDev, pcListName, pcCellsName,
                                                 0, iIndex, pdtpaOutArgs));
}
/*********************************************************************************************************
** ��������: API_DeviceTreePhandleParse
** ��������: phandle ����
** �䡡��  : pdtnDev        �豸���ڵ�
**           pcPhandleName  ����
**           iIndex         ���
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreePhandleParse (const PLW_DEVTREE_NODE     pdtnDev,
                                                    CPCHAR               pcPhandleName,
                                                    INT                  iIndex)
{
    LW_DEVTREE_PHANDLE_ARGS   dtpaOutArgs;
    INT                       iRet;
    
    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    if (iIndex < 0) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    iRet = API_DeviceTreePhandleParseFixedArgs(pdtnDev, pcPhandleName, LW_NULL, 0,
                                               iIndex, &dtpaOutArgs);
    if (iRet) {
        return  (LW_NULL);
    }

    return  (dtpaOutArgs.DTPH_pdtnDev);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeFindNodeByPhandle
** ��������: ͨ�� phandle �����豸���ڵ�
** �䡡��  : uiPhandle   �豸���ڵ�� phandle
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeFindNodeByPhandle (UINT32  uiPhandle)
{
    PLW_DEVTREE_NODE  pdtnDev = LW_NULL;
    UINT32            uiMaskedHandle;

    if (uiPhandle == 0) {
        return  (LW_NULL);
    }

    uiMaskedHandle = uiPhandle & _G_uiPhandleCacheMask;

    if (_G_ppdtnPhandleCache) {
        if (_G_ppdtnPhandleCache[uiMaskedHandle] &&
            uiPhandle == _G_ppdtnPhandleCache[uiMaskedHandle]->DTN_uiHandle) {
            pdtnDev = _G_ppdtnPhandleCache[uiMaskedHandle];
        }
    }

    if (!pdtnDev) {
        _LIST_EACH_OF_ALLNODES(pdtnDev) {
            if (pdtnDev->DTN_uiHandle == uiPhandle) {
                if (_G_ppdtnPhandleCache) {
                    _G_ppdtnPhandleCache[uiMaskedHandle] = pdtnDev;
                }
                break;
            }
        }
    }

    return  (pdtnDev);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
