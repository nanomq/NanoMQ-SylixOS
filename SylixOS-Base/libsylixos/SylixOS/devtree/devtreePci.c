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
** 文   件   名: devtreePci.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2021 年 07 月 30 日
**
** 描        述: 设备树接口 PCI 设备相关接口实现
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "system/device/pci/pciDev.h"
/*********************************************************************************************************
** 函数名称: __deviceTreePciFlagsGet
** 功能描述: 获取 PCI 的资源类型
** 输　入  : puiAddr           解析的资源内容
** 输　出  : 资源类型
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __deviceTreePciFlagsGet (const  UINT32  *puiAddr)
{
    UINT    uiFlags = 0;
    UINT32  uiValue = BE32_TO_CPU(puiAddr);

    switch ((uiValue >> 24) & 0x03) {

    case 0x01:
        uiFlags = PCI_IORESOURCE_IO;
        break;

    case 0x02:                                                          /*  32 bits                     */
        uiFlags = PCI_IORESOURCE_MEM;
        break;

    case 0x03:                                                          /*  64 bits                     */
        uiFlags = PCI_IORESOURCE_MEM_64;
        break;
    }

    if (uiValue & 0x40000000) {
        uiFlags = PCI_IORESOURCE_PREFETCH;
    }

    return  (uiFlags);
}
/*********************************************************************************************************
** 函数名称: __deviceTreePciParserInit
** 功能描述: 初始化 PCI 设备树节点资源解析
** 输　入  : pdtpciparser      资源解析结构
**           pdtnDev           设备树节点
**           pcName            资源名称
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreePciParserInit (PLW_DEVTREE_PCI_RANGE_PARSER  pdtpciparser,
                                       PLW_DEVTREE_NODE              pdtnDev,
                                       CPCHAR                        pcName)
{
    const INT  iNa = 3;
    const INT  iNs = 2;
          INT  iRlen;

    pdtpciparser->DTPRP_pdtnDev  = pdtnDev;
    pdtpciparser->DTPRP_iPna     = API_DeviceTreeNAddrCells(pdtnDev);
    pdtpciparser->DTPRP_iNp      = pdtpciparser->DTPRP_iPna + iNa + iNs;

    pdtpciparser->DTPRP_puiRange = API_DeviceTreePropertyGet(pdtnDev, pcName, &iRlen);
    if (pdtpciparser->DTPRP_puiRange == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    pdtpciparser->DTPRP_puiEnd   = pdtpciparser->DTPRP_puiRange + iRlen / sizeof(UINT32);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreePciRangeParserOne
** 功能描述: 解析一条 PCI 设备树节点资源
** 输　入  : pdtpciparser      资源解析结构
**           pdtpcirange       range 资源结构
** 输　出  : 解析出的 range 资源结构
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
static PLW_DEVTREE_PCI_RANGE __deviceTreePciRangeParserOne (PLW_DEVTREE_PCI_RANGE_PARSER  pdtpciparser,
                                                            PLW_DEVTREE_PCI_RANGE         pdtpcirange)
{
    const INT     iNa = 3;
    const INT     iNs = 2;
          UINT32  uiFlags;
          UINT64  ullPciAddr;
          UINT64  ullCpuAddr;
          UINT64  ullSize;

    if (!pdtpcirange) {
        return  (LW_NULL);
    }

    if (!pdtpciparser->DTPRP_puiRange ||
        ((pdtpciparser->DTPRP_puiRange + pdtpciparser->DTPRP_iNp) > pdtpciparser->DTPRP_puiEnd)) {
        return  (LW_NULL);
    }

    pdtpcirange->DTPR_uiPciSpace = BE32_TO_CPU(pdtpciparser->DTPRP_puiRange);
    pdtpcirange->DTPR_uiFlags    = __deviceTreePciFlagsGet(pdtpciparser->DTPRP_puiRange);
    pdtpcirange->DTPR_ullPciAddr = __deviceTreeNumberRead(pdtpciparser->DTPRP_puiRange + 1, iNs);
    pdtpcirange->DTPR_ullCpuAddr = API_DeviceTreeAddressTranslate(pdtpciparser->DTPRP_pdtnDev,
                                                                  pdtpciparser->DTPRP_puiRange + iNa);
    pdtpcirange->DTPR_ullSize    = __deviceTreeNumberRead(pdtpciparser->DTPRP_puiRange +
                                                            pdtpciparser->DTPRP_iPna + iNa, iNs);

    pdtpciparser->DTPRP_puiRange += pdtpciparser->DTPRP_iNp;

    while ((pdtpciparser->DTPRP_puiRange + pdtpciparser->DTPRP_iNp) <= pdtpciparser->DTPRP_puiEnd) {
        uiFlags    = __deviceTreePciFlagsGet(pdtpciparser->DTPRP_puiRange);
        ullPciAddr = __deviceTreeNumberRead(pdtpciparser->DTPRP_puiRange + 1, iNs);
        ullCpuAddr = API_DeviceTreeAddressTranslate(pdtpciparser->DTPRP_pdtnDev,
                                                    pdtpciparser->DTPRP_puiRange + iNa);
        ullSize    = __deviceTreeNumberRead(pdtpciparser->DTPRP_puiRange +
                                            pdtpciparser->DTPRP_iPna + iNa, iNs);

        if (uiFlags != pdtpcirange->DTPR_uiFlags) {
            break;
        }

        if ((ullPciAddr != pdtpcirange->DTPR_ullPciAddr + pdtpcirange->DTPR_ullSize) ||
            (ullCpuAddr != pdtpcirange->DTPR_ullCpuAddr + pdtpcirange->DTPR_ullSize)) {
            break;
        }

        pdtpcirange->DTPR_ullSize    += ullSize;
        pdtpciparser->DTPRP_puiRange += pdtpciparser->DTPRP_iNp;
    }

    return  (pdtpcirange);
}
/*********************************************************************************************************
** 函数名称: __deviceTreePciRangeToResource
** 功能描述: 将一条 PCI Range 资源翻译为 resource
** 输　入  : pdtpcirange       range 资源结构
**           pdtnDev           设备树节点
**           pdevresource      翻译出的 resource
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreePciRangeToResource (PLW_DEVTREE_PCI_RANGE  pdtpcirange,
                                            PLW_DEVTREE_NODE       pdtnDev,
                                            PLW_DEV_RESOURCE       pdevresource)
{
    pdevresource->DEVRES_pcName        = pdtnDev->DTN_pcFullName;
    pdevresource->DEVRES_ulFlags       = pdtpcirange->DTPR_uiFlags;
    pdevresource->iomem.DEVRES_ulStart = pdtpcirange->DTPR_ullCpuAddr;
    pdevresource->iomem.DEVRES_ulEnd   = pdtpcirange->DTPR_ullCpuAddr + pdtpcirange->DTPR_ullSize - 1;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreePciResourceOffsetAdd
** 功能描述: 将一条 resource 加入资源链表
** 输　入  : pplineheadResource   资源链表头
**           pdevresource         要加入的资源
**           ullOffset            当前的偏移
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreePciResourceOffsetAdd (LW_LIST_LINE_HEADER *pplineheadResource,
                                               PLW_DEV_RESOURCE     pdevresource,
                                               UINT64               ullOffset)
{
    PLW_DEV_RESOURCE_ENTRY   pdevresentry;

    pdevresentry = __SHEAP_ZALLOC(sizeof(LW_DEV_RESOURCE_ENTRY));
    if (!pdevresentry) {
        return;
    }

    pdevresentry->DEVRESE_ullOffset = ullOffset;
    pdevresentry->DEVRESE_pdevres   = pdevresource;

    _List_Line_Add_Tail(&pdevresentry->DEVRESE_plineManage,
                        pplineheadResource);
}
/*********************************************************************************************************
** 函数名称: __deviceTreePciResourceListFree
** 功能描述: 释放一条 resource 资源链表
** 输　入  : plineheadResource    资源链表头
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreePciResourceListFree (LW_LIST_LINE_HEADER  *pplineheadResource)
{
    PLW_DEV_RESOURCE_ENTRY   pdevresentry;
    PLW_LIST_LINE            plineTemp;

    for (plineTemp  = *pplineheadResource;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pdevresentry = _LIST_ENTRY(plineTemp, LW_DEV_RESOURCE_ENTRY, DEVRESE_plineManage);
        if (pdevresentry->DEVRESE_pdevres) {
            __SHEAP_FREE(pdevresentry->DEVRESE_pdevres);
        }

        _List_Line_Del(&pdevresentry->DEVRESE_plineManage,
                       pplineheadResource);

        __SHEAP_FREE(pdevresentry);
    }
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePciBusRangeParse
** 功能描述: PCI 设备树节点获取总线范围
** 输　入  : pdtnDev          指定的设备树节点
**           pdevresource   存储地址范围的资源变量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePciBusRangeParse (PLW_DEVTREE_NODE  pdtnDev, PLW_DEV_RESOURCE  pdevresource)
{
    UINT32  uiBusRange[2];
    INT     iError;

    if (!pdtnDev || !pdevresource) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iError = API_DeviceTreePropertyU32ArrayRead(pdtnDev, "bus-range",
                                                uiBusRange, ARRAY_SIZE(uiBusRange));
    if (iError) {
        return  (iError);
    }

    pdevresource->DEVRES_pcName      = pdtnDev->DTN_pcFullName;
    pdevresource->DEVRES_ulFlags     = PCI_IORESOURCE_BUS;
    pdevresource->bus.DEVRES_ulStart = uiBusRange[0];
    pdevresource->bus.DEVRES_ulEnd   = uiBusRange[1];

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePciHostBridgeResourcesGet
** 功能描述: PCI 设备树节点获取 PCI 桥片上资源
** 输　入  : pdtnDev             指定的设备树节点
**           ucBusNo             起始 BUS 序号
**           ucBusMax            结束 BUS 序号
**           pplineheadResource  存储资源的链表头
**           pullIoBase          IO 基地址
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePciHostBridgeResourcesGet (PLW_DEVTREE_NODE     pdtnDev,
                                              UINT8                ucBusNo,
                                              UINT8                ucBusMax,
                                              LW_LIST_LINE_HEADER *pplineheadResource,
                                              UINT64              *pullIoBase)
{
    LW_DEVTREE_PCI_RANGE_PARSER   dtpciparser;
    LW_DEVTREE_PCI_RANGE          dtpcirange;
    LW_DEV_RESOURCE               devresTemp;
    PLW_DEV_RESOURCE              pdevresource;
    PLW_DEV_RESOURCE              pdevresBusRange;
    INT                           iError;

    if (!pdtnDev || !pplineheadResource) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pullIoBase) {
        *pullIoBase = (UINT64)OF_BAD_ADDR;
    }

    pdevresBusRange = __SHEAP_ZALLOC(sizeof(LW_DEV_RESOURCE));
    if (!pdevresBusRange) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    /*
     *  解析 PCI BUS 资源
     */
    iError = API_DeviceTreePciBusRangeParse(pdtnDev, pdevresBusRange);
    if (iError) {
        pdevresBusRange->bus.DEVRES_ulStart = ucBusNo;
        pdevresBusRange->bus.DEVRES_ulEnd   = ucBusMax;
        pdevresBusRange->DEVRES_ulFlags     = PCI_IORESOURCE_BUS;
    } else {
        if (pdevresBusRange->bus.DEVRES_ulEnd > (pdevresBusRange->bus.DEVRES_ulStart + ucBusMax)) {
            pdevresBusRange->bus.DEVRES_ulEnd = pdevresBusRange->bus.DEVRES_ulStart + ucBusMax;
        }
    }

    __deviceTreePciResourceOffsetAdd(pplineheadResource,
                                     pdevresBusRange, 0);               /*  将 BUS 资源加入链表         */

    /*
     *  解析 PCI IO、MEM 资源
     */
    iError = __deviceTreePciParserInit(&dtpciparser, pdtnDev, "ranges");
    if (iError) {
        goto  failed;
    }

    while (__deviceTreePciRangeParserOne(&dtpciparser, &dtpcirange)) {
        if ((dtpcirange.DTPR_ullCpuAddr == OF_BAD_ADDR) ||
            (dtpcirange.DTPR_ullSize    == 0)) {
            continue;
        }

        iError = __deviceTreePciRangeToResource(&dtpcirange,
                                                pdtnDev,
                                                &devresTemp);           /*  Range 资源转换为 resource   */
        if (iError) {
            continue;
        }

        pdevresource = __SHEAP_ALLOC(sizeof(LW_DEV_RESOURCE));
        if (!pdevresource) {
            _ErrorHandle(ENOMEM);
            goto  failed;
        }

        lib_memcpy(pdevresource, &devresTemp, sizeof(LW_DEV_RESOURCE));

        if (pdevresource->DEVRES_ulFlags == PCI_IORESOURCE_IO) {
            if (!pullIoBase) {
                _ErrorHandle(EINVAL);
                goto  failed;
            }

            *pullIoBase = dtpcirange.DTPR_ullCpuAddr;
        }

        __deviceTreePciResourceOffsetAdd(pplineheadResource,
                                         pdevresource,
                                         pdevresource->iomem.DEVRES_ulStart - dtpcirange.DTPR_ullPciAddr);
    }

    return  (ERROR_NONE);

failed:
    __deviceTreePciResourceListFree(pplineheadResource);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreePciRangesParse
** 功能描述: PCI 设备树节点获取 PCI IO、MEM、BUS 范围
** 输　入  : pdtnDev               指定的设备树节点
**           pplineheadResource    存储地址范围的链表头
**           pdevresBusRange       BUS 的范围
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreePciRangesParse (PLW_DEVTREE_NODE     pdtnDev,
                                   LW_LIST_LINE_HEADER *pplineheadResource,
                                   PLW_DEV_RESOURCE    *pdevresBusRange)
{
    PLW_DEV_RESOURCE_ENTRY   pdevreseTmp;
    PLW_DEV_RESOURCE         pdevres;
    PLW_LIST_LINE            plineTemp;
    UINT64                   ullIoBase;
    INT                      iError;

    if (!pdtnDev || !pplineheadResource) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    *pplineheadResource = LW_NULL;

    iError = API_DeviceTreePciHostBridgeResourcesGet(pdtnDev,
                                                     0,
                                                     0xff,
                                                     pplineheadResource,
                                                     &ullIoBase);
    if (iError) {
        return  (iError);
    }

    for (plineTemp  = *pplineheadResource;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  对资源进行一次分析          */

        pdevreseTmp = _LIST_ENTRY(plineTemp, LW_DEV_RESOURCE_ENTRY, DEVRESE_plineManage);
        pdevres     = pdevreseTmp->DEVRESE_pdevres;

        switch (pdevres->DEVRES_ulFlags) {

        case PCI_IORESOURCE_IO:
        case PCI_IORESOURCE_MEM:
        case PCI_IORESOURCE_MEM_64:
            break;

        case PCI_IORESOURCE_BUS:
            if (pdevresBusRange) {
                *pdevresBusRange = pdevres;
            }
            break;
        }
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
