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
** ��   ��   ��: devtreeLowLevel.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 07 �� 30 ��
**
** ��        ��: �豸���ӿڵײ�ӿ�ϵͳ�ں�ʵ��
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
  ��������
*********************************************************************************************************/
#define MAX_REGION            3
/*********************************************************************************************************
  �ڵ�����
*********************************************************************************************************/
#define PHYSICAL_DMA          "physical_dma"
#define PHYSICAL_APP          "physical_app"
#define VIRTUAL_APP           "virtual_app"
#define VIRTUAL_IOMAP         "virtual_iomap"
/*********************************************************************************************************
  ϵͳȫ�ֱ�����
*********************************************************************************************************/
static addr_t                 _G_ulKernelParam;                         /*  ����������λ��              */
static PLW_MMU_PHYSICAL_DESC  _G_pphysicalDesc;                         /*  �����ڴ�����                */
static PLW_MMU_VIRTUAL_DESC   _G_pvirtualDesc;                          /*  �����ڴ�����                */
/*********************************************************************************************************
  �ļ���ȫ�ֱ���
*********************************************************************************************************/
static INT                    _G_iPhyDescNum;
static INT                    _G_iVirDescNum;
static INT                    _G_iRootAddrCells;                        /*  ���ڵ� #address-cells ��ֵ  */
static INT                    _G_iRootSizeCells;                        /*  ���ڵ� #size-cells ��ֵ     */
static size_t                 _G_stPhyOriginDesc;
/*********************************************************************************************************
** ��������: __deviceTreeChosenNodeScan
** ��������: �����豸���� chosen �ڵ�
** �䡡��  : pvDevtreeMem   �豸����ַ
**           iOffset        �ڵ��ַƫ��
**           pcName         �ڵ�����
**           iDepth         ���ҵ����
**           pvData         ��ȡ�� chosen �ڵ����ݵ�ַ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT   __deviceTreeChosenNodeScan (PVOID       pvDevtreeMem,
                                         INT         iOffset,
                                         CPCHAR      pcName,
                                         INT         iDepth,
                                         PVOID       pvData)
{
    CPCHAR  pcNodeProp;
    INT     iNodePropLen;

    if ((iDepth != 1)       ||
        (pvData == LW_NULL) ||
        ((lib_strcmp(pcName, "chosen")   != 0) &&
         (lib_strcmp(pcName, "chosen@0") != 0))) {                      /*  ���û���ҵ� chosen �ڵ�    */
        return  (ERROR_NONE);                                           /*  ������ȷ�����������¸��ڵ�  */
    }

    pcNodeProp = fdt_getprop(pvDevtreeMem, iOffset,
                             "bootargs", &iNodePropLen);                /*  ��ȡ "bootargs"             */
    if ((pcNodeProp != LW_NULL) && (iNodePropLen > 0)) {
        *(addr_t *)pvData = (addr_t)pcNodeProp;
        DEVTREE_MSG("Command line is: %s\r\n", pcNodeProp);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __deviceTreeRootCellScan
** ��������: �豸�����Ҹ��ڵ�� cells ����
** �䡡��  : pvDevtreeMem   �豸����ַ
**           iOffset        �ڵ��ַƫ��
**           pcName         �ڵ�����
**           iDepth         ���ҵ����
**           pvData         ��ȡ�Ľڵ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeRootCellScan (PVOID       pvDevtreeMem,
                                      INT         iOffset,
                                      CPCHAR      pcName,
                                      INT         iDepth,
                                      PVOID       pvData)
{
    CPVOID  pvSizeProp;
    CPVOID  pvAddrProp;

    if (iDepth != 0) {                                                  /*  ���ڵ�Ĳ��һ��Ϊ 0        */
        return  (PX_ERROR);
    }

    pvAddrProp = fdt_getprop(pvDevtreeMem, iOffset,
                             "#address-cells", LW_NULL);                /*  ��ȡ���ڵ�� address cells  */
    pvSizeProp = fdt_getprop(pvDevtreeMem, iOffset,
                             "#size-cells", LW_NULL);                   /*  ��ȡ���ڵ�� size cells     */

    _G_iRootAddrCells = pvAddrProp ? BE32_TO_CPU(pvAddrProp) : ROOT_NODE_ADDR_CELLS_DEFAULT;
    _G_iRootSizeCells = pvSizeProp ? BE32_TO_CPU(pvSizeProp) : ROOT_NODE_SIZE_CELLS_DEFAULT;

    DEVTREE_MSG("Root Node #address-cells: %d, #size-cells: %d\r\n",
                _G_iRootAddrCells, _G_iRootSizeCells);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __deviceTreeMemNextCell
** ��������: �豸����ȡ��һ�� memory cell ����Ϣ
** �䡡��  : iSize       ��ȡ�����ݳ���
**           ppvCell     Cell ����ַ�������ڲ������
** �䡡��  : ��ȡ����Ϣ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT64  __deviceTreeMemoryNextCell (INT  iSize, PVOID  *ppvCell)
{
    UINT32  *puiTemp = *ppvCell;

    *(UINT32 **)ppvCell = puiTemp + iSize;

    return  (__deviceTreeNumberRead(puiTemp, iSize));
}
/*********************************************************************************************************
** ��������: __deviceTreeMemoryNodeSave
** ��������: ��¼�豸���� memory cell ����Ϣ
** �䡡��  : pcMemName   Cell ����
**           ullBase     Cell ����ַ
**           ullSize     Cell ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeMemoryNodeSave (CPCHAR     pcMemName,
                                        UINT64     ullBase,
                                        UINT64     ullSize)
{
    static INT  iMaxPhyRegion = MAX_REGION;
    static INT  iMaxVirRegion = MAX_REGION;

    if (_G_iPhyDescNum == (iMaxPhyRegion - 1)) {
        iMaxPhyRegion    = iMaxPhyRegion << 1;
        _G_pphysicalDesc = __SHEAP_REALLOC(_G_pphysicalDesc,
                                           sizeof(LW_MMU_PHYSICAL_DESC) * iMaxPhyRegion +
                                           _G_stPhyOriginDesc);
        if (!_G_pphysicalDesc) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }

    if (_G_iVirDescNum == (iMaxVirRegion - 1)) {
        iMaxVirRegion   = iMaxVirRegion << 1;
        _G_pvirtualDesc = __SHEAP_REALLOC(_G_pvirtualDesc,
                                          sizeof(LW_MMU_VIRTUAL_DESC) * iMaxVirRegion);
        if (!_G_pvirtualDesc) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }

    if (!lib_strcmp(pcMemName, PHYSICAL_DMA)) {                          /*  DMA �����ڴ�               */
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_ulPhyAddr = (addr_t)ullBase;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_ulVirMap  = (addr_t)ullBase;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_stSize    = (size_t)ullSize;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_uiType    = LW_PHYSICAL_MEM_DMA;
        _G_iPhyDescNum++;
        return  (ERROR_NONE);
    }

    if (!lib_strcmp(pcMemName, PHYSICAL_APP)) {                         /*  APP �����ڴ�                */
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_ulPhyAddr = (addr_t)ullBase;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_stSize    = (size_t)ullSize;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_uiType    = LW_PHYSICAL_MEM_APP;
        _G_iPhyDescNum++;
        return  (ERROR_NONE);
    }

    if (!lib_strcmp(pcMemName, VIRTUAL_APP)) {                           /*  APP �����ڴ�               */
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_ulVirAddr  = (addr_t)ullBase;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_stSize     = (size_t)ullSize;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_uiType     = LW_VIRTUAL_MEM_APP;
        _G_iVirDescNum++;
        return  (ERROR_NONE);
    }

    if (!lib_strcmp(pcMemName, VIRTUAL_IOMAP)) {                         /*  IOMAP �����ڴ�             */
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_ulVirAddr  = (addr_t)ullBase;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_stSize     = (size_t)ullSize;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_uiType     = LW_VIRTUAL_MEM_DEV;
        _G_iVirDescNum++;
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __deviceTreeMemoryParse
** ��������: �豸���� memory cell ����Ϣ����
** �䡡��  : pvDevtreeMem   �豸����ַ
**           iOffset        �ڵ��ַƫ��
**           iTotalLen      �����ܳ�
**           pcCellName     �ڵ�����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeMemoryParse (PVOID    pvDevtreeMem,
                                     INT      iOffset,
                                     INT      iTotalLen,
                                     CPCHAR   pcCellName)
{
    UINT64  ullBase;
    UINT64  ullSize;
    CPVOID  pvProp;
    INT     iLen;

    pvProp = fdt_getprop(pvDevtreeMem, iOffset, pcCellName, &iLen);
    if (!pvProp) {
         _ErrorHandle(ENOENT);
         return  (PX_ERROR);
    }

    if (iLen && (iLen % iTotalLen != 0)) {                              /*  ��Դ���Ȳ���ȷ              */
         _DebugFormat(__ERRORMESSAGE_LEVEL, "Reserved memory: invalid reg property in '%s', "
                      "skipping node.\r\n", pcCellName);
         _ErrorHandle(EINVAL);
         return  (PX_ERROR);
    }

    while (iLen >= iTotalLen) {                                         /*  ����������Դ                */
        ullBase = __deviceTreeMemoryNextCell(_G_iRootAddrCells, (PVOID *)&pvProp);
        ullSize = __deviceTreeMemoryNextCell(_G_iRootSizeCells, (PVOID *)&pvProp);

        iLen   -= iTotalLen;

        __deviceTreeMemoryNodeSave(pcCellName,
                                   ullBase, ullSize);                   /*  ��¼��Ϣ                    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __deviceTreeMemoryScan
** ��������: �豸�����Ҹ��ڵ�� memory ����
** �䡡��  : pvDevtreeMem   �豸����ַ
**           iOffset        �ڵ��ַƫ��
**           pcName         �ڵ�����
**           iDepth         ���ҵ����
**           pvData         ԭʼ���������б�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeMemoryScan (PVOID       pvDevtreeMem,
                                    INT         iOffset,
                                    CPCHAR      pcName,
                                    INT         iDepth,
                                    PVOID       pvData)
{
    INT     iTotalLen;

    if ((iDepth != 1) ||
        (lib_strcmp(pcName, "memory") != 0)) {                          /*  ���û���ҵ� memory �ڵ�    */
        return  (ERROR_NONE);                                           /*  ������ȷ�����������¸��ڵ�  */
    }

    iTotalLen          = (_G_iRootAddrCells + _G_iRootSizeCells) * sizeof(UINT32);
    _G_stPhyOriginDesc = (size_t)pvData;

    if (!_G_pphysicalDesc) {                                            /*  �������������ṹ����        */
        _G_pphysicalDesc = __SHEAP_ZALLOC(sizeof(LW_MMU_PHYSICAL_DESC) * MAX_REGION + _G_stPhyOriginDesc);
        if (!_G_pphysicalDesc) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }

    if (!_G_pvirtualDesc) {                                             /*  �������������ṹ����        */
        _G_pvirtualDesc  = __SHEAP_ZALLOC(sizeof(LW_MMU_VIRTUAL_DESC) * MAX_REGION);
        if (!_G_pvirtualDesc) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }

    __deviceTreeMemoryParse(pvDevtreeMem, iOffset, iTotalLen, PHYSICAL_DMA);
    __deviceTreeMemoryParse(pvDevtreeMem, iOffset, iTotalLen, PHYSICAL_APP);
    __deviceTreeMemoryParse(pvDevtreeMem, iOffset, iTotalLen, VIRTUAL_APP);
    __deviceTreeMemoryParse(pvDevtreeMem, iOffset, iTotalLen, VIRTUAL_IOMAP);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __deviceTreeScan
** ��������: �豸���ڵ�ɨ�裬��ִ��ָ����ɨ��ص�����
** �䡡��  : pvDevtreeMem         �豸����ַ
**           pfuncScanCallBack    ɨ��ڵ�ʱ�Ļص�����
**           pvData               �ص���������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeScan (PVOID  pvDevtreeMem, FUNC_SCAN_CALLBACK  pfuncScanCallBack, PVOID  pvData)
{
    CPCHAR  pcNodeName;
    INT     iOffset;
    INT     iRet   = 0;
    INT     iDepth = -1;

    for (iOffset  = fdt_next_node(pvDevtreeMem, -1, &iDepth);
         (iOffset >= 0) &&                                              /*  �ڵ�ƫ����Ч                */
         (iDepth  >= 0) &&                                              /*  �ڵ������Ч                */
         (iRet    == ERROR_NONE);                                       /*  �ص�ִ����Ч                */
         iOffset  = fdt_next_node(pvDevtreeMem, iOffset, &iDepth)) {    /*  ����ÿһ���ڵ�              */

        pcNodeName = fdt_get_name(pvDevtreeMem, iOffset, LW_NULL);
        if (*pcNodeName == '/') {                                       /*  ����ڵ���ȫ·����          */
            pcNodeName = __deviceTreeBaseNameGet(pcNodeName);           /*  ��ȡ�ڵ������              */
        }

        iRet = pfuncScanCallBack(pvDevtreeMem, iOffset, pcNodeName, iDepth, pvData);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __deviceTreeRootNodesScan
** ��������: ɨ���豸�����ڵ���Ϣ
** �䡡��  : pvDevtreeMem       �豸����ַ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeRootNodesScan (PVOID  pvDevtreeMem)
{
    if (__deviceTreeScan(pvDevtreeMem,
                         __deviceTreeChosenNodeScan,
                         (PVOID)&_G_ulKernelParam)) {                   /*  ��ȡ chosen �ڵ�            */
        return  (PX_ERROR);
    }

    if (__deviceTreeScan(pvDevtreeMem,
                         __deviceTreeRootCellScan,
                         LW_NULL)) {                                    /*  ��ȡ���ڵ� cells ����       */
        return  (ERROR_NONE);                                           /*  ��������ֵΪ ERROR_NONE     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeKernelVmmParamGet
** ��������: ����豸�������ϵͳ�ڴ����
** �䡡��  : pvDevtreeMem        �豸������ַ
**           pphyOriginDesc      ԭʼ���豸�������ڴ�����
**           stPhyOriginDesc     ԭʼ���豸�������ڴ��ֽ�
**           ppphydesc           ���������ṹ
**           ppvirdes            ���������ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
                                       API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeKernelVmmParamGet (PVOID                   pvDevtreeMem,
                                      PLW_MMU_PHYSICAL_DESC   pphyOriginDesc,
                                      size_t                  stPhyOriginDesc,
                                      PLW_MMU_PHYSICAL_DESC  *ppphydesc,
                                      PLW_MMU_VIRTUAL_DESC   *ppvirdes)
{
    if (!pphyOriginDesc || !ppphydesc || !ppvirdes) {
        _ErrorHandle(ERROR_KERNEL_BUFFER_NULL);
        return  (PX_ERROR);
    }

    if (__deviceTreeScan(pvDevtreeMem,
                         __deviceTreeMemoryScan,
                         (PVOID)stPhyOriginDesc)) {                     /*  ����ڴ�ڵ���Ϣ            */
        return  (PX_ERROR);
    }

    lib_memcpy(&_G_pphysicalDesc[_G_iPhyDescNum], pphyOriginDesc, stPhyOriginDesc);
    *ppphydesc = _G_pphysicalDesc;
    *ppvirdes  = _G_pvirtualDesc;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeKernelStartParamGet
** ��������: ����豸�������ϵͳ�ں���������
** �䡡��  : pcParam       ��������
**           stLen         ����������
** �䡡��  : ʵ�ʿ�����������������
** ȫ�ֱ���:
** ����ģ��:
                                       API ����
*********************************************************************************************************/
LW_API
ssize_t  API_DeviceTreeKernelStartParamGet (PCHAR  pcParam, size_t  stLen)
{
    if (!pcParam || !stLen) {
        _ErrorHandle(ERROR_KERNEL_BUFFER_NULL);
        return  (0);
    }

    return  ((ssize_t)lib_strlcpy(pcParam, (PVOID)_G_ulKernelParam, stLen));
}
/*********************************************************************************************************
** ��������: API_DeviceTreeLowLevelInit
** ��������: �豸��������ʼ����֮����Ի�ȡ���ڵ���Ϣ
** �䡡��  : pvDevtreeMem        �豸������ַ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeLowLevelInit (PVOID  pvDevtreeMem)
{
    if (!pvDevtreeMem) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (PX_ERROR);
    }

    if (fdt_check_header(pvDevtreeMem)) {                               /*  �豸��ͷ��У��              */
        _ErrorHandle(ERROR_DEVTREE_MAGIC_ERROR);
        return  (PX_ERROR);
    }

    if (__deviceTreeRootNodesScan(pvDevtreeMem)) {                      /*  ɨ����ڵ���Ϣ              */
        _ErrorHandle(ERROR_DEVTREE_SCAN_ERROR);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
