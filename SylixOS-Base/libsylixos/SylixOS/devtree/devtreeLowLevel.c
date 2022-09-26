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
** 文   件   名: devtreeLowLevel.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 07 月 30 日
**
** 描        述: 设备树接口底层接口系统内核实现
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
  区域数量
*********************************************************************************************************/
#define MAX_REGION            3
/*********************************************************************************************************
  节点名称
*********************************************************************************************************/
#define PHYSICAL_DMA          "physical_dma"
#define PHYSICAL_APP          "physical_app"
#define VIRTUAL_APP           "virtual_app"
#define VIRTUAL_IOMAP         "virtual_iomap"
/*********************************************************************************************************
  系统全局变量的
*********************************************************************************************************/
static addr_t                 _G_ulKernelParam;                         /*  启动参数的位置              */
static PLW_MMU_PHYSICAL_DESC  _G_pphysicalDesc;                         /*  物理内存描述                */
static PLW_MMU_VIRTUAL_DESC   _G_pvirtualDesc;                          /*  虚拟内存描述                */
/*********************************************************************************************************
  文件内全局变量
*********************************************************************************************************/
static INT                    _G_iPhyDescNum;
static INT                    _G_iVirDescNum;
static INT                    _G_iRootAddrCells;                        /*  根节点 #address-cells 的值  */
static INT                    _G_iRootSizeCells;                        /*  根节点 #size-cells 的值     */
static size_t                 _G_stPhyOriginDesc;
/*********************************************************************************************************
** 函数名称: __deviceTreeChosenNodeScan
** 功能描述: 查找设备树的 chosen 节点
** 输　入  : pvDevtreeMem   设备树地址
**           iOffset        节点地址偏移
**           pcName         节点名称
**           iDepth         查找的深度
**           pvData         获取的 chosen 节点数据地址
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
         (lib_strcmp(pcName, "chosen@0") != 0))) {                      /*  如果没有找到 chosen 节点    */
        return  (ERROR_NONE);                                           /*  返回正确，继续查找下个节点  */
    }

    pcNodeProp = fdt_getprop(pvDevtreeMem, iOffset,
                             "bootargs", &iNodePropLen);                /*  获取 "bootargs"             */
    if ((pcNodeProp != LW_NULL) && (iNodePropLen > 0)) {
        *(addr_t *)pvData = (addr_t)pcNodeProp;
        DEVTREE_MSG("Command line is: %s\r\n", pcNodeProp);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeRootCellScan
** 功能描述: 设备树查找根节点的 cells 定义
** 输　入  : pvDevtreeMem   设备树地址
**           iOffset        节点地址偏移
**           pcName         节点名称
**           iDepth         查找的深度
**           pvData         获取的节点数据
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeRootCellScan (PVOID       pvDevtreeMem,
                                      INT         iOffset,
                                      CPCHAR      pcName,
                                      INT         iDepth,
                                      PVOID       pvData)
{
    CPVOID  pvSizeProp;
    CPVOID  pvAddrProp;

    if (iDepth != 0) {                                                  /*  根节点的层次一定为 0        */
        return  (PX_ERROR);
    }

    pvAddrProp = fdt_getprop(pvDevtreeMem, iOffset,
                             "#address-cells", LW_NULL);                /*  获取根节点的 address cells  */
    pvSizeProp = fdt_getprop(pvDevtreeMem, iOffset,
                             "#size-cells", LW_NULL);                   /*  获取根节点的 size cells     */

    _G_iRootAddrCells = pvAddrProp ? BE32_TO_CPU(pvAddrProp) : ROOT_NODE_ADDR_CELLS_DEFAULT;
    _G_iRootSizeCells = pvSizeProp ? BE32_TO_CPU(pvSizeProp) : ROOT_NODE_SIZE_CELLS_DEFAULT;

    DEVTREE_MSG("Root Node #address-cells: %d, #size-cells: %d\r\n",
                _G_iRootAddrCells, _G_iRootSizeCells);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemNextCell
** 功能描述: 设备树读取下一个 memory cell 的信息
** 输　入  : iSize       读取的数据长度
**           ppvCell     Cell 基地址，函数内部会更新
** 输　出  : 读取的信息内容
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT64  __deviceTreeMemoryNextCell (INT  iSize, PVOID  *ppvCell)
{
    UINT32  *puiTemp = *ppvCell;

    *(UINT32 **)ppvCell = puiTemp + iSize;

    return  (__deviceTreeNumberRead(puiTemp, iSize));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemoryNodeSave
** 功能描述: 记录设备树的 memory cell 的信息
** 输　入  : pcMemName   Cell 名称
**           ullBase     Cell 基地址
**           ullSize     Cell 大小
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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

    if (!lib_strcmp(pcMemName, PHYSICAL_DMA)) {                          /*  DMA 物理内存               */
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_ulPhyAddr = (addr_t)ullBase;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_ulVirMap  = (addr_t)ullBase;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_stSize    = (size_t)ullSize;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_uiType    = LW_PHYSICAL_MEM_DMA;
        _G_iPhyDescNum++;
        return  (ERROR_NONE);
    }

    if (!lib_strcmp(pcMemName, PHYSICAL_APP)) {                         /*  APP 物理内存                */
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_ulPhyAddr = (addr_t)ullBase;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_stSize    = (size_t)ullSize;
        _G_pphysicalDesc[_G_iPhyDescNum].PHYD_uiType    = LW_PHYSICAL_MEM_APP;
        _G_iPhyDescNum++;
        return  (ERROR_NONE);
    }

    if (!lib_strcmp(pcMemName, VIRTUAL_APP)) {                           /*  APP 虚拟内存               */
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_ulVirAddr  = (addr_t)ullBase;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_stSize     = (size_t)ullSize;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_uiType     = LW_VIRTUAL_MEM_APP;
        _G_iVirDescNum++;
        return  (ERROR_NONE);
    }

    if (!lib_strcmp(pcMemName, VIRTUAL_IOMAP)) {                         /*  IOMAP 虚拟内存             */
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_ulVirAddr  = (addr_t)ullBase;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_stSize     = (size_t)ullSize;
        _G_pvirtualDesc[_G_iVirDescNum].VIRD_uiType     = LW_VIRTUAL_MEM_DEV;
        _G_iVirDescNum++;
        return  (ERROR_NONE);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemoryParse
** 功能描述: 设备树的 memory cell 的信息解析
** 输　入  : pvDevtreeMem   设备树地址
**           iOffset        节点地址偏移
**           iTotalLen      数据总长
**           pcCellName     节点名称
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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

    if (iLen && (iLen % iTotalLen != 0)) {                              /*  资源长度不正确              */
         _DebugFormat(__ERRORMESSAGE_LEVEL, "Reserved memory: invalid reg property in '%s', "
                      "skipping node.\r\n", pcCellName);
         _ErrorHandle(EINVAL);
         return  (PX_ERROR);
    }

    while (iLen >= iTotalLen) {                                         /*  逐条解析资源                */
        ullBase = __deviceTreeMemoryNextCell(_G_iRootAddrCells, (PVOID *)&pvProp);
        ullSize = __deviceTreeMemoryNextCell(_G_iRootSizeCells, (PVOID *)&pvProp);

        iLen   -= iTotalLen;

        __deviceTreeMemoryNodeSave(pcCellName,
                                   ullBase, ullSize);                   /*  记录信息                    */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemoryScan
** 功能描述: 设备树查找根节点的 memory 定义
** 输　入  : pvDevtreeMem   设备树地址
**           iOffset        节点地址偏移
**           pcName         节点名称
**           iDepth         查找的深度
**           pvData         原始物理描述列表
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeMemoryScan (PVOID       pvDevtreeMem,
                                    INT         iOffset,
                                    CPCHAR      pcName,
                                    INT         iDepth,
                                    PVOID       pvData)
{
    INT     iTotalLen;

    if ((iDepth != 1) ||
        (lib_strcmp(pcName, "memory") != 0)) {                          /*  如果没有找到 memory 节点    */
        return  (ERROR_NONE);                                           /*  返回正确，继续查找下个节点  */
    }

    iTotalLen          = (_G_iRootAddrCells + _G_iRootSizeCells) * sizeof(UINT32);
    _G_stPhyOriginDesc = (size_t)pvData;

    if (!_G_pphysicalDesc) {                                            /*  申请物理描述结构区域        */
        _G_pphysicalDesc = __SHEAP_ZALLOC(sizeof(LW_MMU_PHYSICAL_DESC) * MAX_REGION + _G_stPhyOriginDesc);
        if (!_G_pphysicalDesc) {
            _ErrorHandle(ENOMEM);
            return  (PX_ERROR);
        }
    }

    if (!_G_pvirtualDesc) {                                             /*  申请虚拟描述结构区域        */
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
** 函数名称: __deviceTreeScan
** 功能描述: 设备树节点扫描，并执行指定的扫描回调函数
** 输　入  : pvDevtreeMem         设备树地址
**           pfuncScanCallBack    扫描节点时的回调函数
**           pvData               回调函数参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeScan (PVOID  pvDevtreeMem, FUNC_SCAN_CALLBACK  pfuncScanCallBack, PVOID  pvData)
{
    CPCHAR  pcNodeName;
    INT     iOffset;
    INT     iRet   = 0;
    INT     iDepth = -1;

    for (iOffset  = fdt_next_node(pvDevtreeMem, -1, &iDepth);
         (iOffset >= 0) &&                                              /*  节点偏移有效                */
         (iDepth  >= 0) &&                                              /*  节点深度有效                */
         (iRet    == ERROR_NONE);                                       /*  回调执行有效                */
         iOffset  = fdt_next_node(pvDevtreeMem, iOffset, &iDepth)) {    /*  遍历每一个节点              */

        pcNodeName = fdt_get_name(pvDevtreeMem, iOffset, LW_NULL);
        if (*pcNodeName == '/') {                                       /*  如果节点是全路径名          */
            pcNodeName = __deviceTreeBaseNameGet(pcNodeName);           /*  获取节点的名称              */
        }

        iRet = pfuncScanCallBack(pvDevtreeMem, iOffset, pcNodeName, iDepth, pvData);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeRootNodesScan
** 功能描述: 扫描设备树根节点信息
** 输　入  : pvDevtreeMem       设备树地址
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeRootNodesScan (PVOID  pvDevtreeMem)
{
    if (__deviceTreeScan(pvDevtreeMem,
                         __deviceTreeChosenNodeScan,
                         (PVOID)&_G_ulKernelParam)) {                   /*  获取 chosen 节点            */
        return  (PX_ERROR);
    }

    if (__deviceTreeScan(pvDevtreeMem,
                         __deviceTreeRootCellScan,
                         LW_NULL)) {                                    /*  获取根节点 cells 定义       */
        return  (ERROR_NONE);                                           /*  修正返回值为 ERROR_NONE     */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeKernelVmmParamGet
** 功能描述: 获得设备树定义的系统内存参数
** 输　入  : pvDevtreeMem        设备树基地址
**           pphyOriginDesc      原始的设备树物理内存描述
**           stPhyOriginDesc     原始的设备树物理内存字节
**           ppphydesc           物理描述结构
**           ppvirdes            虚拟描述结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
                                       API 函数
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
                         (PVOID)stPhyOriginDesc)) {                     /*  获得内存节点信息            */
        return  (PX_ERROR);
    }

    lib_memcpy(&_G_pphysicalDesc[_G_iPhyDescNum], pphyOriginDesc, stPhyOriginDesc);
    *ppphydesc = _G_pphysicalDesc;
    *ppvirdes  = _G_pvirtualDesc;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeKernelStartParamGet
** 功能描述: 获得设备树定义的系统内核启动参数
** 输　入  : pcParam       启动参数
**           stLen         缓冲区长度
** 输　出  : 实际拷贝的启动参数长度
** 全局变量:
** 调用模块:
                                       API 函数
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
** 函数名称: API_DeviceTreeLowLevelInit
** 功能描述: 设备树初级初始化，之后可以获取根节点信息
** 输　入  : pvDevtreeMem        设备树基地址
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeLowLevelInit (PVOID  pvDevtreeMem)
{
    if (!pvDevtreeMem) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (PX_ERROR);
    }

    if (fdt_check_header(pvDevtreeMem)) {                               /*  设备树头部校验              */
        _ErrorHandle(ERROR_DEVTREE_MAGIC_ERROR);
        return  (PX_ERROR);
    }

    if (__deviceTreeRootNodesScan(pvDevtreeMem)) {                      /*  扫描根节点信息              */
        _ErrorHandle(ERROR_DEVTREE_SCAN_ERROR);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
