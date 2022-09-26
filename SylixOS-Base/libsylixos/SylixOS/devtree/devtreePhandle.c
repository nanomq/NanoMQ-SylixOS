/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: devtreePhandle.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 07 月 30 日
**
** 描        述: 设备树 phandle 相关接口
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
** 函数名称: __deviceTreePhandleIteratorArgs
** 功能描述: 获取当前迭代器指示的 phandle 参数
** 输　入  : pdtpiItor      迭代器指针
**           puiArgs        获取的参数
**           iSize          参数的大小
** 输　出  : 参数占用的大小
** 全局变量:
** 调用模块:
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
** 函数名称: API_DeviceTreePhandleIteratorInit
** 功能描述: 初始化 phandle 的迭代器
** 输　入  : pdtpiItor     需要初始化的迭代器
**           pdtnDev       设备树节点
**           pcListName    链表名称
**           pcCellsName   cell 名称
**           iCellCount    cell 的数量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreePhandleIteratorNext
** 功能描述: 获取下一个迭代器
** 输　入  : pdtpiItor           当前的迭代器指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreePhandleCountWithArgs
** 功能描述: 获取列表中 phandle 数量
** 输　入  : pdtnDev        设备树节点
**           pcListName     列表名称
**           pcCellsName    cells 名称
** 输　出  : phandle 数量 or ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreePhandleParseFixedArgs
** 功能描述: 解析带有固定数量参数的 phandle
** 输　入  : pdtnDev       设备树节点
**           pcListName    列表名称
**           pcCellsName   cells 名称
**           iCellCount    cells 数量
**           iIndex        序号
**           pdtpaOutArgs  解析出的 phandle 参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreePhandleParseWithArgs
** 功能描述: 带有参数的 phandle 解析
** 输　入  : pdtnDev       设备树节点
**           pcListName    列表名称
**           pcCellsName   节点名称
**           iIndex        序号
**           pdtpaOutArgs  解析出的 phandle 参数
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreePhandleParse
** 功能描述: phandle 解析
** 输　入  : pdtnDev        设备树节点
**           pcPhandleName  名称
**           iIndex         序号
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreeFindNodeByPhandle
** 功能描述: 通过 phandle 查找设备树节点
** 输　入  : uiPhandle   设备树节点的 phandle
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
**                                            API 函数
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
