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
** ��   ��   ��: devtreeLib.c
**
** ��   ��   ��: Wang.Xuan(���Q)
**
** �ļ���������: 2019 �� 07 �� 31 ��
**
** ��        ��: �豸��ͨ�ýӿ�
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
  ȫ�ֿɼ�����
*********************************************************************************************************/
PLW_DEVTREE_NODE        _G_pdtnRoot;                                    /*  root   �ڵ�                 */
PLW_DEVTREE_NODE        _G_pdtnAliases;                                 /*  aliases �ڵ�                */
PLW_DEVTREE_NODE       *_G_ppdtnPhandleCache;                           /*  phandle cache               */
UINT32                  _G_uiPhandleCacheMask;
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define DEVTREE_MAX_ADDR_CELLS          4
#define DEVTREE_CHECK_ADDR_COUNT(na)    ((na) > 0 && (na) <= DEVTREE_MAX_ADDR_CELLS)
#define DEVTREE_CHECK_COUNTS(na, ns)    ((na) > 0 && (na) <= DEVTREE_MAX_ADDR_CELLS && (ns) > 0)
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID  __deviceTreeDefaultCountCells(PLW_DEVTREE_NODE  pdtnDev,
                                           INT              *piCellAddr,
                                           INT              *piCellSize);
static UINT64  __deviceTreeDefaultMap(UINT32  *puiAddr,
                                      UINT32  *puiRange,
                                      INT      iNumAddr,
                                      INT      iNumSize,
                                      INT      iParentNumAddr);
static INT  __deviceTreeDefaultTranslate(UINT32  *puiAddr,
                                         UINT64   ullOffset,
                                         INT      iNumAddr);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_DEVTREE_BUS  _G_dtbBusses[] = {
    {                                                                   /*  Default                     */
        .DTBUS_pcAddresses = "reg",
        .DTBUS_match       = LW_NULL,
        .DTBUS_cellsCount  = __deviceTreeDefaultCountCells,
        .DTBUS_map         = __deviceTreeDefaultMap,
        .DTBUS_translate   = __deviceTreeDefaultTranslate,
    },
};
/*********************************************************************************************************
** ��������: __deviceTreeAddrDump
** ��������: ��ӡ addr cells �����Ϣ
** �䡡��  : pcStr     ǰ׺����ַ���
**           puiAddr   addr cells ��ַ
**           iNumAddr  addr cells �ĸ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __deviceTreeAddrDump (CPCHAR  pcStr, const UINT32  *puiAddr, INT  iNumAddr)
{
#if DEVTREE_DEBUG_EN > 0
    DEVTREE_MSG("%s", pcStr);
    while (iNumAddr--) {
        DEVTREE_MSG(" %08x", be32toh(*(puiAddr++)));
    }
    DEVTREE_MSG("\r\n");
#endif
}
/*********************************************************************************************************
** ��������: __deviceTreeDefaultCountCells
** ��������: Ĭ���������ͼ��� cells ����
** �䡡��  : pdtnDev        �豸���ڵ�
**           piCellAddr     �洢 addr cells �ĸ���
**           piCellSize     �洢 size cells �ĸ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __deviceTreeDefaultCountCells (PLW_DEVTREE_NODE  pdtnDev,
                                            INT              *piCellAddr,
                                            INT              *piCellSize)
{
    if (piCellAddr) {
        *piCellAddr = API_DeviceTreeNAddrCells(pdtnDev);
    }

    if (piCellSize) {
        *piCellSize = API_DeviceTreeNSizeCells(pdtnDev);
    }
}
/*********************************************************************************************************
** ��������: __deviceTreeDefaultMap
** ��������: Ĭ�������豸��ӳ�䷽��
** �䡡��  : puiAddr         ��ǰ��ַ
**           puiRange        range ����ָ��
**           iNumAddr        address-cells
**           iNumSize        address-size
**           iParentNumAddr  ���ڵ�� address-cells
** �䡡��  : ��ǰƫ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  __deviceTreeDefaultMap (UINT32   *puiAddr,
                                       UINT32   *puiRange,
                                       INT       iNumAddr,
                                       INT       iNumSize,
                                       INT       iParentNumAddr)
{
    UINT64  ullCp;
    UINT64  ullS;
    UINT64  ullDa;

    ullCp = __deviceTreeNumberRead(puiRange, iNumAddr);
    ullS  = __deviceTreeNumberRead(puiRange + iNumAddr + iParentNumAddr, iNumSize);
    ullDa = __deviceTreeNumberRead(puiAddr,  iNumAddr);

    if ((ullDa < ullCp) || (ullDa >= (ullCp + ullS))) {
        return  (PX_ERROR);
    }

    return  (ullDa - ullCp);
}
/*********************************************************************************************************
** ��������: __deviceTreeDefaultTranslate
** ��������: Ĭ�ϵ�ַ�������͵ĵ�ַת������
** �䡡��  : puiAddr      �洢ת�����ַ�ı���ָ��
**           ullOffset    ת����ƫ�Ƶ�ַ
**           iNumAddr     ��ַ cells �ĸ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeDefaultTranslate (UINT32  *puiAddr,
                                          UINT64   ullOffset,
                                          INT      iNumAddr)
{
    UINT64  ullAddr = __deviceTreeNumberRead(puiAddr, iNumAddr);

    lib_bzero(puiAddr, iNumAddr * 4);
    ullAddr += ullOffset;

    if (iNumAddr > 1) {
        puiAddr[iNumAddr - 2] = htobe32(ullAddr >> 32);
    }

    puiAddr[iNumAddr - 1] = htobe32(ullAddr & 0xfffffffful);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __deviceTreeBusMatch
** ��������: ƥ����ʵ� bus
** �䡡��  : pdtnDev        ����ƥ����豸���ڵ�
** �䡡��  : ƥ�䵽�� bus �� LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_DEVTREE_BUS  __deviceTreeBusMatch (PLW_DEVTREE_NODE  pdtnDev)
{
    INT  i;

    for (i = 0; i < ARRAY_SIZE(_G_dtbBusses); i++) {
        if (!_G_dtbBusses[i].DTBUS_match ||                             /*  Ĭ�ϰ��� default bus ƥ��   */
             _G_dtbBusses[i].DTBUS_match(pdtnDev)) {
            return  (&_G_dtbBusses[i]);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __deviceTreePropertyStringIndexRead
** ��������: ������Ŷ�ȡ String ��������
** �䡡��  : pdtnDev           �豸���ڵ�
**           pcPropName        ��������
**           iIndex            �������
**           ppcOutput         ��ȡ�� String ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __deviceTreePropertyStringIndexRead (PLW_DEVTREE_NODE  pdtnDev,
                                                CPCHAR            pcPropName,
                                                INT               iIndex,
                                                CPCHAR           *ppcOutput)
{
    INT  iRc = API_DeviceTreePropertyStringHelperRead(pdtnDev, pcPropName,
                                                      ppcOutput, 1, iIndex, LW_NULL);

    return  (iRc < 0 ? iRc : 0);
}
/*********************************************************************************************************
** ��������: __deviceTreeTranslateOne
** ��������: ����ַ���Ի�ַת��Ϊʵ�� 64 λ��ַ
**           ע��ranges ����ֵΪ��ʱ����ʾ 1:1 ӳ��
** �䡡��  : pdtnParent        ���豸���ڵ�
**           pdtbBus           ��Ӧ����������
**           pdtbParentBus     ���豸��Ӧ����������
**           puiAddr           ��ǰ��ַ
**           iNumAddr          address-cells
**           iNumSize          address-size
**           iParentNumAddr    ���ڵ�� address-cells
**           pcResourceProp    ��ָ������������ת��
** �䡡��  : ʵ�� 64 λ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeTranslateOne (PLW_DEVTREE_NODE  pdtnParent,
                                      PLW_DEVTREE_BUS   pdtbBus,
                                      PLW_DEVTREE_BUS   pdtbParentBus,
                                      UINT32           *puiAddr,
                                      INT               iNumAddr,
                                      INT               iNumSize,
                                      INT               iParentNumAddr,
                                      CPCHAR            pcResourceProp)
{
    UINT32  *puiRanges;
    UINT     uiResourceLen;
    INT      iResourceOne;
    UINT64   ullOffset = PX_ERROR;

    puiRanges = API_DeviceTreePropertyGet(pdtnParent,
                                          pcResourceProp,
                                          (INT *)&uiResourceLen);       /*  ��ȡ���ڵ�� ranges ����    */
    if ((puiRanges     == LW_NULL) ||                                   /*  ��� ranges ����Ϊ��        */
        (uiResourceLen == 0)) {                                         /*  ���� ranges ���Գ���Ϊ 0    */
        ullOffset = __deviceTreeNumberRead(puiAddr, iNumAddr);          /*  ��ȡ��ַ����ֵ              */
        lib_bzero(puiAddr, iParentNumAddr * 4);
        DEVTREE_MSG("empty ranges; 1:1 translation\r\n");
        goto  __finish;
    }

    DEVTREE_MSG("walking ranges...\r\n");

    /*
     *  ���¶� ranges ���Խ��д���
     */
    uiResourceLen /= 4;
    iResourceOne   = iNumAddr + iParentNumAddr + iNumSize;
    for (;
         uiResourceLen >= iResourceOne;
         uiResourceLen -= iResourceOne, puiRanges += iResourceOne) {
        ullOffset = pdtbBus->DTBUS_map(puiAddr, puiRanges, iNumAddr,
                                       iNumSize, iParentNumAddr);
        if (ullOffset != PX_ERROR) {
            break;
        }
    }

    if (ullOffset == PX_ERROR) {
        DEVTREE_MSG("not found !\r\n");
        return  (PX_ERROR);
    }

    lib_memcpy(puiAddr, puiRanges + iNumAddr, 4 * iParentNumAddr);

__finish:
    __deviceTreeAddrDump("parent translation for:", puiAddr, iParentNumAddr);

    DEVTREE_MSG("with offset: %llx\r\n", ullOffset);

    return  (pdtbParentBus->DTBUS_translate(puiAddr, ullOffset, iParentNumAddr));
}
/*********************************************************************************************************
** ��������: __deviceTreeAddressTranslate
** ��������: ����ַ���Ի�ַת��Ϊʵ�� 64 λ��ַ
** �䡡��  : pdtnDev           �豸���ڵ�
**           puiInAddr         ��ַ���Ի�ַ
**           pcResourceProp    ��ָ������������ת��
** �䡡��  : ʵ�� 64 λ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  __deviceTreeAddressTranslate (PLW_DEVTREE_NODE  pdtnDev,
                                             const UINT32     *puiInAddr,
                                             CPCHAR            pcResourceProp)
{
    PLW_DEVTREE_NODE  pdtnParent = LW_NULL;
    PLW_DEVTREE_BUS   pdtbBus;
    PLW_DEVTREE_BUS   pdtbParentBus;
    UINT32            uiAddr[DEVTREE_MAX_ADDR_CELLS];
    INT               iNumAddr;
    INT               iNumSize;
    INT               iParentNumAddr;
    INT               iParentNumSize;
    UINT64            ullResult = PX_ERROR;

    DEVTREE_MSG("OF: ** translation for device %s **\r\n",
                pdtnDev->DTN_pcFullName);

    pdtnParent = pdtnDev->DTN_pdtnparent;                               /*  ��ȡ���ڵ�                  */
    if (pdtnParent == LW_NULL) {
        goto  __end;                                                    /*  ������ڵ�Ϊ�գ�ֱ�ӷ���    */
    }

    pdtbBus = __deviceTreeBusMatch(pdtnParent);                         /*  �ҵ���Ӧ����������          */
    pdtbBus->DTBUS_cellsCount(pdtnDev, &iNumAddr, &iNumSize);           /*  ��ȡ #size-cells            */
                                                                        /*  �� #address-cells           */

    if (!DEVTREE_CHECK_COUNTS(iNumAddr, iNumSize)) {                    /*  �ж� cells �Ƿ���Ч         */
        DEVTREE_MSG("Bad cell count for %s\r\n",
                    pdtnDev->DTN_pcFullName);
        goto  __end;
    }

    lib_memcpy(uiAddr, puiInAddr, iNumAddr * 4);                        /*  �� #address-cells ������ֵ  */

    __deviceTreeAddrDump("OF: translating address:", uiAddr, iNumAddr);

    for (;;) {                                                          /*  ��ʼ�����ַ                */
        pdtnDev    = pdtnParent;                                        /*  ��ȡ���ڵ�                  */
        pdtnParent = pdtnDev->DTN_pdtnparent;                           /*  ��ȡүү�ڵ�                */

        if (pdtnParent == LW_NULL) {                                    /*  ���үү�ڵ�Ϊ "/"          */
            DEVTREE_MSG("OF: reached root node\r\n");
            ullResult = __deviceTreeNumberRead(uiAddr, iNumAddr);       /*  ��ǰ��ַ��ַΪ�������Ի�ַ  */
            break;
        }

        pdtbParentBus = __deviceTreeBusMatch(pdtnParent);               /*  ��ȡүү�ڵ����������      */
        pdtbParentBus->DTBUS_cellsCount(pdtnDev,
                                   &iParentNumAddr,                     /*  ��ȡ #address-cells         */
                                   &iParentNumSize);                    /*  �� #size-cells              */

        if (!DEVTREE_CHECK_COUNTS(iParentNumAddr, iParentNumSize)) {    /*  �ж� cells �Ƿ���Ч         */
            DEVTREE_MSG("Bad cell count for %s\r\n",
                        pdtnDev->DTN_pcFullName);
            break;
        }

        if (__deviceTreeTranslateOne(pdtnDev,  pdtbBus,
                                     pdtbParentBus, uiAddr,
                                     iNumAddr, iNumSize,
                                     iParentNumAddr, pcResourceProp)) { /*  Ӧ�õ�ַת��                */
            break;
        }

        iNumAddr = iParentNumAddr;                                      /*  ����үү�ڵ���Ϣ����        */
        iNumSize = iParentNumSize;
        pdtbBus  = pdtbParentBus;

        __deviceTreeAddrDump("OF: one level translation:", uiAddr, iNumAddr);
    }

__end:
    return  (ullResult);                                                /*  ��������ת������ 64 λ��ַ  */
}
/*********************************************************************************************************
** ��������: __deviceTreeFindNodeByParentPath
** ��������: ͨ���豸���ڵ�ĸ��ڵ�ȫ·�����ҽڵ�
** �䡡��  : pdtnParent  �豸���ڵ�ĸ��ڵ�
**           pcPath      �ڵ�·��
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_DEVTREE_NODE  __deviceTreeFindNodeByParentPath (PLW_DEVTREE_NODE  pdtnParent,
                                                           CPCHAR            pcPath)
{
    PLW_DEVTREE_NODE  pdtnChild;
    CPCHAR            pcName;
    INT               iLen;

    iLen = lib_strcspn(pcPath, "/:");
    if (!iLen) {
        return  (LW_NULL);
    }

    _LIST_EACH_CHILD_OF_NODE(pdtnParent, pdtnChild) {
        pcName = __deviceTreeBaseNameGet(pdtnChild->DTN_pcFullName);
        if ((lib_strncmp(pcPath, pcName, iLen) == 0) &&
            (lib_strlen(pcName) == iLen)) {
            return  (pdtnChild);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __deviceTreeFindNodeByFullPath
** ��������: ͨ���豸���ڵ�ȫ·�������豸���ڵ�
** �䡡��  : pdtnDev     ��ʼ���豸���ڵ�
**           pcPath      �ڵ�ȫ·��
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_DEVTREE_NODE  __deviceTreeFindNodeByFullPath (PLW_DEVTREE_NODE  pdtnDev,
                                                         CPCHAR            pcPath)
{
    CPCHAR   pcSeparator = lib_strchr(pcPath, ':');

    while (pdtnDev && (*pcPath == '/')) {
        pcPath++;                                                       /* �� '/' ֮���ַ���ʼ����      */
        pdtnDev = __deviceTreeFindNodeByParentPath(pdtnDev, pcPath);
        pcPath  = strchrnul(pcPath, '/');
        if (pcSeparator && (pcSeparator < pcPath)) {
            break;
        }
    }

    return  (pdtnDev);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeNAddrCells
** ��������: ��ȡ addr cells �ĸ���
** �䡡��  : pdtnDev        �豸���ڵ�
** �䡡��  : addr cells �ĸ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeNAddrCells (PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiCells;

    if (!pdtnDev) {
        return  (0);
    }

    do {
        if (pdtnDev->DTN_pdtnparent) {                                  /*  ����и��ڵ㣬��ȡ�丸�ڵ�  */
            pdtnDev = pdtnDev->DTN_pdtnparent;
        }

        if (!API_DeviceTreePropertyU32Read(pdtnDev,
                                           "#address-cells",
                                           &uiCells)) {                 /*  ��ȡ "#address-cells" ����  */
            return  (uiCells);
        }
    } while (pdtnDev->DTN_pdtnparent);                                  /*  ��δ���������ϼ����ڵ����  */

    return  (ROOT_NODE_ADDR_CELLS_DEFAULT);                             /*  ��������ȡ����������Ĭ��ֵ  */
}
/*********************************************************************************************************
** ��������: API_DeviceTreeNSizeCells
** ��������: ��ȡ size cells �ĸ���
** �䡡��  : pdtnDev        �豸���ڵ�
** �䡡��  : size cells �ĸ���
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeNSizeCells (PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiCells;

    if (!pdtnDev) {
        return  (0);
    }

    do {
        if (pdtnDev->DTN_pdtnparent) {                                  /*  ����и��ڵ㣬��ȡ�丸�ڵ�  */
            pdtnDev = pdtnDev->DTN_pdtnparent;
        }

        if (!API_DeviceTreePropertyU32Read(pdtnDev,
                                           "#size-cells",
                                           &uiCells)) {                 /*  ��ȡ "#size-cells" ����     */
            return  (uiCells);
        }
    } while (pdtnDev->DTN_pdtnparent);                                  /*  ��δ���������ϼ����ڵ����  */

    return  (ROOT_NODE_SIZE_CELLS_DEFAULT);                             /*  ��������ȡ����������Ĭ��ֵ  */
}
/*********************************************************************************************************
** ��������: API_DeviceTreeAddressGet
** ��������: ��ȡ��ַ����
**           ����� reg = <0x00 0x5000800 0x00 0x400>;
**           �ú���ͬʱ���Ի�ȡ addr ��ַ �� size ֵ��
** �䡡��  : pdtnDev        ���ڻ�ȡ���Ե��豸���ڵ�
**           iIndex         ָ���� cells ���
**           pullSize       �������Դ��С
** �䡡��  : ��ַ���Զ�Ӧ�ĵ�ַ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
const UINT32*  API_DeviceTreeAddressGet (PLW_DEVTREE_NODE  pdtnDev,
                                         INT               iIndex,
                                         UINT64           *pullSize)
{
    const UINT32            *puiProp;
          UINT               uiPSize;
          PLW_DEVTREE_NODE   pdtnParent;
          PLW_DEVTREE_BUS    pdtbBus;
          INT                iOneSize;
          INT                i;
          INT                iNumAddr;
          INT                iNumSize;

    if (!pdtnDev) {
        return  (LW_NULL);
    }

    pdtnParent = pdtnDev->DTN_pdtnparent;
    if (pdtnParent == LW_NULL) {                                        /*  ��ǰ�ڵ�ĸ��ڵ㲻��Ϊ��    */
        return  (LW_NULL);
    }

    pdtbBus = __deviceTreeBusMatch(pdtnParent);                         /*  �ҵ���Ӧ����������          */
    pdtbBus->DTBUS_cellsCount(pdtnDev, &iNumAddr, &iNumSize);
    if (!DEVTREE_CHECK_ADDR_COUNT(iNumAddr)) {                          /*  ��� addr cells ����ȷ      */
        return  (LW_NULL);
    }

    puiProp = API_DeviceTreePropertyGet(pdtnDev,
                                        pdtbBus->DTBUS_pcAddresses,
                                        (INT *)&uiPSize);               /*  ��ȡ��Ӧ������������ֵ��ַ  */
    if (puiProp == LW_NULL) {
        return  (LW_NULL);
    }

    uiPSize /= 4;                                                       /*  ����Ϊ N �� INT ����        */
    iOneSize = iNumAddr + iNumSize;                                     /*  һ�� cells �ĳ���           */

    for (i = 0;
         uiPSize >= iOneSize;
         uiPSize -= iOneSize, puiProp += iOneSize, i++) {
        if (i == iIndex) {                                              /*  ��ȡָ����ŵ� cells ����ֵ */
            if (pullSize) {                                             /*  ��ȡ��Դ size ��С          */
                *pullSize = __deviceTreeNumberRead(puiProp + iNumAddr,
                                                   iNumSize);
            }
            return  (puiProp);                                          /*  ������Դ addr ��ַ          */
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeAddressTranslate
** ��������: ����ַ���Ի�ַת��Ϊʵ�� 64 λ��ַ
** �䡡��  : pdtnDev        �豸���ڵ�
**           puiInAddr      ��ַ���Ի�ַ
** �䡡��  : ʵ�� 64 λ��ַ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
UINT64  API_DeviceTreeAddressTranslate (PLW_DEVTREE_NODE  pdtnDev, const UINT32  *puiInAddr)
{
    UINT64  ullRet;

    if (!pdtnDev) {
        return  (0);
    }

    ullRet = __deviceTreeAddressTranslate(pdtnDev, puiInAddr, "ranges");

    return  (ullRet);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeResourceGet
** ��������: ����ַת��Ϊ��Դ����
** �䡡��  : pdtnDev        �豸���ڵ�
**           iIndex         ָ���� cells ���
**           pdevresource   �洢��ַ��Χ����Դ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeResourceGet (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex, PLW_DEV_RESOURCE  pdevresource)
{
    const UINT32  *puiAddr;
          ULONG    ulTmpAddr;
          UINT64   ullSize;
          CPCHAR   pcName = LW_NULL;

    if (!pdevresource) {
        return  (-EINVAL);
    }

    puiAddr = API_DeviceTreeAddressGet(pdtnDev, iIndex, &ullSize);      /*  ��ȡ��ַ��ַ�͵�ַ��Χ      */
    if (puiAddr == LW_NULL) {
        return  (-EINVAL);
    }

    __deviceTreePropertyStringIndexRead(pdtnDev,
                                        "reg-names",
                                        iIndex,
                                        &pcName);                       /*  ���Զ�ȡ "reg-names" ����   */

    ulTmpAddr = API_DeviceTreeAddressTranslate(pdtnDev, puiAddr);       /*  �Ե�ַ����ת��              */
    if (ulTmpAddr == PX_ERROR) {
        return  (-EINVAL);
    }

    lib_bzero(pdevresource, sizeof(LW_DEV_RESOURCE));

    pdevresource->iomem.DEVRES_ulStart = ulTmpAddr;                     /*  �����ʼ��ַ                */
    pdevresource->iomem.DEVRES_ulEnd   = ulTmpAddr + ullSize - 1;       /*  ��������ַ                */
    pdevresource->DEVRES_pcName        = pcName ?
                                         pcName :
                                         pdtnDev->DTN_pcFullName;       /*  �����Դ����                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeRegAddressGet
** ��������: ��ȡ�豸�Ĵ�����ַ��Χ
** �䡡��  : pdtnDev        �豸���ڵ�
**           pdevresource   �洢��ַ��Χ����Դ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeRegAddressGet (PLW_DEVTREE_NODE  pdtnDev, PLW_DEV_RESOURCE  pdevresource)
{
    LW_DEV_RESOURCE   resTemp;
    PLW_DEV_RESOURCE  presTemp;
    INT               iNumReg = 0;
    INT               i;

    if (!pdevresource) {                                                /*  �洢��ַ����Դ��������Ϊ��  */
        return  (PX_ERROR);
    }

    while (!API_DeviceTreeResourceGet(pdtnDev, iNumReg, &resTemp)) {    /*  ��ȡ�洢��ַ�� cells ����   */
        iNumReg++;
    }

    presTemp = pdevresource;

    if (iNumReg) {
        for (i = 0; i < iNumReg; i++, presTemp++) {                     /*  ��ȡ��ַ��Դ                */
            API_DeviceTreeResourceGet(pdtnDev, i, presTemp);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeAddressIoremap
** ��������: ��ȡ�豸���е���Դ��ַ��������ӳ��
** �䡡��  : pdtnDev     �豸���ڵ�
**           iIndex      �豸�����
** �䡡��  : ӳ����������ַ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PVOID  API_DeviceTreeAddressIoremap (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex)
{
    LW_DEV_RESOURCE  devresource;

    if (API_DeviceTreeResourceGet(pdtnDev, iIndex, &devresource)) {
        return  (LW_NULL);
    }

    return  (API_VmmIoRemapNocache((PVOID)devresource.iomem.DEVRES_ulStart,
                                   RESOURCE_SIZE(devresource)));
}
/*********************************************************************************************************
** ��������: API_DeviceTreeFindNodeOptsByPath
** ��������: ͨ���ڵ�·�������豸���ڵ㣬����ȡð�ŷָ���֮����ַ�
** �䡡��  : pcPath     �ڵ�·��
**                      ��Ч�Ľڵ�·��ֻ�����¼��֣�
**                      ��1������·���������磺/soc@03000000/spi@05010000
**                      ��2�����������磺spi0
**                      ��3������ + ���·����
**           ppcOpts    ��ȡ�ķָ���֮����ַ�
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeFindNodeOptsByPath (CPCHAR  pcPath, CPCHAR  *ppcOpts)
{
    PLW_DEVTREE_NODE      pdtnDev     = LW_NULL;
    PLW_DEVTREE_PROPERTY  pdtprop;
    CPCHAR                pcSeparator;                                  /*  ��ȡð�ŷָ�����λ��        */
    INT                   iLen;
    CPCHAR                pcTmp;

    if (!pcPath) {
        return  (LW_NULL);
    }

    pcSeparator = lib_strchr(pcPath, ':');                              /*  ��ȡð�ŷָ�����λ��        */

    if (ppcOpts) {
        *ppcOpts = pcSeparator ? (pcSeparator + 1) : LW_NULL;           /*  ȡð��֮����ַ���          */
    }

    if (lib_strcmp(pcPath, "/") == 0) {                                 /*  ����Ǹ��ڵ�                */
        return  (_G_pdtnRoot);
    }

    if (*pcPath != '/') {                                               /*  �����ǰ·�����ַ����� '/'  */
        if (!_G_pdtnAliases) {                                          /*  aliases �ڵ㲻��Ϊ��        */
            return  (LW_NULL);
        }

        pcTmp = pcSeparator;
        if (!pcTmp) {
            pcTmp = lib_strchrnul(pcPath, '/');                         /*  �������ַ���ĩβ���ַ�      */
        }
        iLen = pcTmp - pcPath;                                          /*  �õ�·���ַ�������          */

        for (pdtprop  = _G_pdtnAliases->DTN_pdtpproperties;
             pdtprop != LW_NULL;
             pdtprop  = pdtprop->DTP_pdtpNext) {                        /*  ���� alises �ڵ�ÿ������    */

            if ((lib_strlen(pdtprop->DTP_pcName) == iLen) &&
                !lib_strncmp(pdtprop->DTP_pcName, pcPath, iLen)) {
                pdtnDev = __deviceTreeFindNodeByPath(pdtprop->DTP_pvValue);
                break;
            }
        }

        if (!pdtnDev) {
            return  (LW_NULL);
        }

        pcPath = pcTmp;
    }

    if (!pdtnDev) {                                                     /*  ��δ�ҵ�����Ӹ��ڵ㿪ʼ����*/
        pdtnDev = _G_pdtnRoot;
    }
    pdtnDev = __deviceTreeFindNodeByFullPath(pdtnDev, pcPath);

    return  (pdtnDev);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeNextChildGet
** ��������: ��ȡ�豸���ڵ����һ�����ӽڵ�
** �䡡��  : pdtnDev     �豸���ڵ�
**           pdtnPrev    ǰһ���ڵ�
** �䡡��  : ��һ�����ӽڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeNextChildGet (const PLW_DEVTREE_NODE  pdtnDev,
                                                    PLW_DEVTREE_NODE  pdtnPrev)
{
    PLW_DEVTREE_NODE  pdtnNext;

    if (!pdtnDev) {
        return  (LW_NULL);
    }

    pdtnNext = pdtnPrev ? pdtnPrev->DTN_pdtnsibling : pdtnDev->DTN_pdtnchild;

    return  (pdtnNext);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeFindAllNodes
** ��������: ������һ���ڵ�
** �䡡��  : pdtnPrev     ��ʼ�ڵ�
** �䡡��  : �豸���ڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeFindAllNodes (PLW_DEVTREE_NODE  pdtnPrev)
{
    PLW_DEVTREE_NODE  pdtnode;

    if (!pdtnPrev) {
        pdtnode = _G_pdtnRoot;

    } else if (pdtnPrev->DTN_pdtnchild) {
        pdtnode = pdtnPrev->DTN_pdtnchild;

    } else {
        pdtnode = pdtnPrev;

        while (pdtnode->DTN_pdtnparent && !pdtnode->DTN_pdtnsibling) {
            pdtnode = pdtnode->DTN_pdtnparent;
        }
        pdtnode = pdtnode->DTN_pdtnsibling;
    }

    return  (pdtnode);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
