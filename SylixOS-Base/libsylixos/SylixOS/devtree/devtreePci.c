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
** ��   ��   ��: devtreePci.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2021 �� 07 �� 30 ��
**
** ��        ��: �豸���ӿ� PCI �豸��ؽӿ�ʵ��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "system/device/pci/pciDev.h"
/*********************************************************************************************************
** ��������: __deviceTreePciFlagsGet
** ��������: ��ȡ PCI ����Դ����
** �䡡��  : puiAddr           ��������Դ����
** �䡡��  : ��Դ����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __deviceTreePciParserInit
** ��������: ��ʼ�� PCI �豸���ڵ���Դ����
** �䡡��  : pdtpciparser      ��Դ�����ṹ
**           pdtnDev           �豸���ڵ�
**           pcName            ��Դ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __deviceTreePciRangeParserOne
** ��������: ����һ�� PCI �豸���ڵ���Դ
** �䡡��  : pdtpciparser      ��Դ�����ṹ
**           pdtpcirange       range ��Դ�ṹ
** �䡡��  : �������� range ��Դ�ṹ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: __deviceTreePciRangeToResource
** ��������: ��һ�� PCI Range ��Դ����Ϊ resource
** �䡡��  : pdtpcirange       range ��Դ�ṹ
**           pdtnDev           �豸���ڵ�
**           pdevresource      ������� resource
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __deviceTreePciResourceOffsetAdd
** ��������: ��һ�� resource ������Դ����
** �䡡��  : pplineheadResource   ��Դ����ͷ
**           pdevresource         Ҫ�������Դ
**           ullOffset            ��ǰ��ƫ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __deviceTreePciResourceListFree
** ��������: �ͷ�һ�� resource ��Դ����
** �䡡��  : plineheadResource    ��Դ����ͷ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: API_DeviceTreePciBusRangeParse
** ��������: PCI �豸���ڵ��ȡ���߷�Χ
** �䡡��  : pdtnDev          ָ�����豸���ڵ�
**           pdevresource   �洢��ַ��Χ����Դ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_DeviceTreePciHostBridgeResourcesGet
** ��������: PCI �豸���ڵ��ȡ PCI ��Ƭ����Դ
** �䡡��  : pdtnDev             ָ�����豸���ڵ�
**           ucBusNo             ��ʼ BUS ���
**           ucBusMax            ���� BUS ���
**           pplineheadResource  �洢��Դ������ͷ
**           pullIoBase          IO ����ַ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
     *  ���� PCI BUS ��Դ
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
                                     pdevresBusRange, 0);               /*  �� BUS ��Դ��������         */

    /*
     *  ���� PCI IO��MEM ��Դ
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
                                                &devresTemp);           /*  Range ��Դת��Ϊ resource   */
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
** ��������: API_DeviceTreePciRangesParse
** ��������: PCI �豸���ڵ��ȡ PCI IO��MEM��BUS ��Χ
** �䡡��  : pdtnDev               ָ�����豸���ڵ�
**           pplineheadResource    �洢��ַ��Χ������ͷ
**           pdevresBusRange       BUS �ķ�Χ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ����Դ����һ�η���          */

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
