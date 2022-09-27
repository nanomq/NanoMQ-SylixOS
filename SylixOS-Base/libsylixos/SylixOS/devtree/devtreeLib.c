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
** 文   件   名: devtreeLib.c
**
** 创   建   人: Wang.Xuan(王翾)
**
** 文件创建日期: 2019 年 07 月 31 日
**
** 描        述: 设备树通用接口
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
  全局可见变量
*********************************************************************************************************/
PLW_DEVTREE_NODE        _G_pdtnRoot;                                    /*  root   节点                 */
PLW_DEVTREE_NODE        _G_pdtnAliases;                                 /*  aliases 节点                */
PLW_DEVTREE_NODE       *_G_ppdtnPhandleCache;                           /*  phandle cache               */
UINT32                  _G_uiPhandleCacheMask;
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define DEVTREE_MAX_ADDR_CELLS          4
#define DEVTREE_CHECK_ADDR_COUNT(na)    ((na) > 0 && (na) <= DEVTREE_MAX_ADDR_CELLS)
#define DEVTREE_CHECK_COUNTS(na, ns)    ((na) > 0 && (na) <= DEVTREE_MAX_ADDR_CELLS && (ns) > 0)
/*********************************************************************************************************
  函数声明
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
  全局变量
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
** 函数名称: __deviceTreeAddrDump
** 功能描述: 打印 addr cells 里的信息
** 输　入  : pcStr     前缀输出字符串
**           puiAddr   addr cells 地址
**           iNumAddr  addr cells 的个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeDefaultCountCells
** 功能描述: 默认总线类型计算 cells 个数
** 输　入  : pdtnDev        设备树节点
**           piCellAddr     存储 addr cells 的个数
**           piCellSize     存储 size cells 的个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeDefaultMap
** 功能描述: 默认总线设备的映射方法
** 输　入  : puiAddr         当前地址
**           puiRange        range 属性指针
**           iNumAddr        address-cells
**           iNumSize        address-size
**           iParentNumAddr  父节点的 address-cells
** 输　出  : 当前偏移
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeDefaultTranslate
** 功能描述: 默认地址总线类型的地址转换方法
** 输　入  : puiAddr      存储转换后地址的变量指针
**           ullOffset    转换的偏移地址
**           iNumAddr     地址 cells 的个数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeBusMatch
** 功能描述: 匹配合适的 bus
** 输　入  : pdtnDev        用于匹配的设备树节点
** 输　出  : 匹配到的 bus 或 LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_DEVTREE_BUS  __deviceTreeBusMatch (PLW_DEVTREE_NODE  pdtnDev)
{
    INT  i;

    for (i = 0; i < ARRAY_SIZE(_G_dtbBusses); i++) {
        if (!_G_dtbBusses[i].DTBUS_match ||                             /*  默认按照 default bus 匹配   */
             _G_dtbBusses[i].DTBUS_match(pdtnDev)) {
            return  (&_G_dtbBusses[i]);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __deviceTreePropertyStringIndexRead
** 功能描述: 按照序号读取 String 类型属性
** 输　入  : pdtnDev           设备树节点
**           pcPropName        属性名称
**           iIndex            属性序号
**           ppcOutput         读取的 String 属性
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeTranslateOne
** 功能描述: 将地址属性基址转换为实际 64 位地址
**           注：ranges 属性值为空时，表示 1:1 映射
** 输　入  : pdtnParent        父设备树节点
**           pdtbBus           对应的总线类型
**           pdtbParentBus     父设备对应的总线类型
**           puiAddr           当前地址
**           iNumAddr          address-cells
**           iNumSize          address-size
**           iParentNumAddr    父节点的 address-cells
**           pcResourceProp    按指定的属性类型转换
** 输　出  : 实际 64 位地址
** 全局变量:
** 调用模块:
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
                                          (INT *)&uiResourceLen);       /*  读取父节点的 ranges 属性    */
    if ((puiRanges     == LW_NULL) ||                                   /*  如果 ranges 属性为空        */
        (uiResourceLen == 0)) {                                         /*  或者 ranges 属性长度为 0    */
        ullOffset = __deviceTreeNumberRead(puiAddr, iNumAddr);          /*  读取地址属性值              */
        lib_bzero(puiAddr, iParentNumAddr * 4);
        DEVTREE_MSG("empty ranges; 1:1 translation\r\n");
        goto  __finish;
    }

    DEVTREE_MSG("walking ranges...\r\n");

    /*
     *  以下对 ranges 属性进行处理
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
** 函数名称: __deviceTreeAddressTranslate
** 功能描述: 将地址属性基址转换为实际 64 位地址
** 输　入  : pdtnDev           设备树节点
**           puiInAddr         地址属性基址
**           pcResourceProp    按指定的属性类型转换
** 输　出  : 实际 64 位地址
** 全局变量:
** 调用模块:
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

    pdtnParent = pdtnDev->DTN_pdtnparent;                               /*  获取父节点                  */
    if (pdtnParent == LW_NULL) {
        goto  __end;                                                    /*  如果父节点为空，直接返回    */
    }

    pdtbBus = __deviceTreeBusMatch(pdtnParent);                         /*  找到对应的总线类型          */
    pdtbBus->DTBUS_cellsCount(pdtnDev, &iNumAddr, &iNumSize);           /*  获取 #size-cells            */
                                                                        /*  和 #address-cells           */

    if (!DEVTREE_CHECK_COUNTS(iNumAddr, iNumSize)) {                    /*  判断 cells 是否有效         */
        DEVTREE_MSG("Bad cell count for %s\r\n",
                    pdtnDev->DTN_pcFullName);
        goto  __end;
    }

    lib_memcpy(uiAddr, puiInAddr, iNumAddr * 4);                        /*  按 #address-cells 拷贝数值  */

    __deviceTreeAddrDump("OF: translating address:", uiAddr, iNumAddr);

    for (;;) {                                                          /*  开始翻译地址                */
        pdtnDev    = pdtnParent;                                        /*  获取父节点                  */
        pdtnParent = pdtnDev->DTN_pdtnparent;                           /*  获取爷爷节点                */

        if (pdtnParent == LW_NULL) {                                    /*  如果爷爷节点为 "/"          */
            DEVTREE_MSG("OF: reached root node\r\n");
            ullResult = __deviceTreeNumberRead(uiAddr, iNumAddr);       /*  当前地址基址为最终属性基址  */
            break;
        }

        pdtbParentBus = __deviceTreeBusMatch(pdtnParent);               /*  获取爷爷节点的总线类型      */
        pdtbParentBus->DTBUS_cellsCount(pdtnDev,
                                   &iParentNumAddr,                     /*  获取 #address-cells         */
                                   &iParentNumSize);                    /*  和 #size-cells              */

        if (!DEVTREE_CHECK_COUNTS(iParentNumAddr, iParentNumSize)) {    /*  判断 cells 是否有效         */
            DEVTREE_MSG("Bad cell count for %s\r\n",
                        pdtnDev->DTN_pcFullName);
            break;
        }

        if (__deviceTreeTranslateOne(pdtnDev,  pdtbBus,
                                     pdtbParentBus, uiAddr,
                                     iNumAddr, iNumSize,
                                     iParentNumAddr, pcResourceProp)) { /*  应用地址转换                */
            break;
        }

        iNumAddr = iParentNumAddr;                                      /*  按照爷爷节点信息更新        */
        iNumSize = iParentNumSize;
        pdtbBus  = pdtbParentBus;

        __deviceTreeAddrDump("OF: one level translation:", uiAddr, iNumAddr);
    }

__end:
    return  (ullResult);                                                /*  返回最终转换出的 64 位地址  */
}
/*********************************************************************************************************
** 函数名称: __deviceTreeFindNodeByParentPath
** 功能描述: 通过设备树节点的父节点全路径查找节点
** 输　入  : pdtnParent  设备树节点的父节点
**           pcPath      节点路径
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
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
** 函数名称: __deviceTreeFindNodeByFullPath
** 功能描述: 通过设备树节点全路径查找设备树节点
** 输　入  : pdtnDev     开始的设备树节点
**           pcPath      节点全路径
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_DEVTREE_NODE  __deviceTreeFindNodeByFullPath (PLW_DEVTREE_NODE  pdtnDev,
                                                         CPCHAR            pcPath)
{
    CPCHAR   pcSeparator = lib_strchr(pcPath, ':');

    while (pdtnDev && (*pcPath == '/')) {
        pcPath++;                                                       /* 从 '/' 之后字符开始查找      */
        pdtnDev = __deviceTreeFindNodeByParentPath(pdtnDev, pcPath);
        pcPath  = strchrnul(pcPath, '/');
        if (pcSeparator && (pcSeparator < pcPath)) {
            break;
        }
    }

    return  (pdtnDev);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeNAddrCells
** 功能描述: 获取 addr cells 的个数
** 输　入  : pdtnDev        设备树节点
** 输　出  : addr cells 的个数
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeNAddrCells (PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiCells;

    if (!pdtnDev) {
        return  (0);
    }

    do {
        if (pdtnDev->DTN_pdtnparent) {                                  /*  如果有父节点，获取其父节点  */
            pdtnDev = pdtnDev->DTN_pdtnparent;
        }

        if (!API_DeviceTreePropertyU32Read(pdtnDev,
                                           "#address-cells",
                                           &uiCells)) {                 /*  读取 "#address-cells" 属性  */
            return  (uiCells);
        }
    } while (pdtnDev->DTN_pdtnparent);                                  /*  若未读到，向上级父节点查找  */

    return  (ROOT_NODE_ADDR_CELLS_DEFAULT);                             /*  若正常读取不到，返回默认值  */
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeNSizeCells
** 功能描述: 获取 size cells 的个数
** 输　入  : pdtnDev        设备树节点
** 输　出  : size cells 的个数
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeNSizeCells (PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiCells;

    if (!pdtnDev) {
        return  (0);
    }

    do {
        if (pdtnDev->DTN_pdtnparent) {                                  /*  如果有父节点，获取其父节点  */
            pdtnDev = pdtnDev->DTN_pdtnparent;
        }

        if (!API_DeviceTreePropertyU32Read(pdtnDev,
                                           "#size-cells",
                                           &uiCells)) {                 /*  读取 "#size-cells" 属性     */
            return  (uiCells);
        }
    } while (pdtnDev->DTN_pdtnparent);                                  /*  若未读到，向上级父节点查找  */

    return  (ROOT_NODE_SIZE_CELLS_DEFAULT);                             /*  若正常读取不到，返回默认值  */
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeAddressGet
** 功能描述: 获取地址属性
**           如对于 reg = <0x00 0x5000800 0x00 0x400>;
**           该函数同时可以获取 addr 基址 与 size 值。
** 输　入  : pdtnDev        用于获取属性的设备树节点
**           iIndex         指定的 cells 序号
**           pullSize       输出的资源大小
** 输　出  : 地址属性对应的地址
** 全局变量:
** 调用模块:
**                                            API 函数
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
    if (pdtnParent == LW_NULL) {                                        /*  当前节点的父节点不能为空    */
        return  (LW_NULL);
    }

    pdtbBus = __deviceTreeBusMatch(pdtnParent);                         /*  找到对应的总线类型          */
    pdtbBus->DTBUS_cellsCount(pdtnDev, &iNumAddr, &iNumSize);
    if (!DEVTREE_CHECK_ADDR_COUNT(iNumAddr)) {                          /*  如果 addr cells 不正确      */
        return  (LW_NULL);
    }

    puiProp = API_DeviceTreePropertyGet(pdtnDev,
                                        pdtbBus->DTBUS_pcAddresses,
                                        (INT *)&uiPSize);               /*  获取对应属性名的属性值地址  */
    if (puiProp == LW_NULL) {
        return  (LW_NULL);
    }

    uiPSize /= 4;                                                       /*  换算为 N 个 INT 长度        */
    iOneSize = iNumAddr + iNumSize;                                     /*  一个 cells 的长度           */

    for (i = 0;
         uiPSize >= iOneSize;
         uiPSize -= iOneSize, puiProp += iOneSize, i++) {
        if (i == iIndex) {                                              /*  获取指定序号的 cells 属性值 */
            if (pullSize) {                                             /*  获取资源 size 大小          */
                *pullSize = __deviceTreeNumberRead(puiProp + iNumAddr,
                                                   iNumSize);
            }
            return  (puiProp);                                          /*  返回资源 addr 地址          */
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeAddressTranslate
** 功能描述: 将地址属性基址转换为实际 64 位地址
** 输　入  : pdtnDev        设备树节点
**           puiInAddr      地址属性基址
** 输　出  : 实际 64 位地址
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreeResourceGet
** 功能描述: 将地址转换为资源变量
** 输　入  : pdtnDev        设备树节点
**           iIndex         指定的 cells 序号
**           pdevresource   存储地址范围的资源变量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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

    puiAddr = API_DeviceTreeAddressGet(pdtnDev, iIndex, &ullSize);      /*  获取地址基址和地址范围      */
    if (puiAddr == LW_NULL) {
        return  (-EINVAL);
    }

    __deviceTreePropertyStringIndexRead(pdtnDev,
                                        "reg-names",
                                        iIndex,
                                        &pcName);                       /*  尝试读取 "reg-names" 属性   */

    ulTmpAddr = API_DeviceTreeAddressTranslate(pdtnDev, puiAddr);       /*  对地址进行转换              */
    if (ulTmpAddr == PX_ERROR) {
        return  (-EINVAL);
    }

    lib_bzero(pdevresource, sizeof(LW_DEV_RESOURCE));

    pdevresource->iomem.DEVRES_ulStart = ulTmpAddr;                     /*  填充起始地址                */
    pdevresource->iomem.DEVRES_ulEnd   = ulTmpAddr + ullSize - 1;       /*  填充结束地址                */
    pdevresource->DEVRES_pcName        = pcName ?
                                         pcName :
                                         pdtnDev->DTN_pcFullName;       /*  填充资源名称                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeRegAddressGet
** 功能描述: 获取设备寄存器地址范围
** 输　入  : pdtnDev        设备树节点
**           pdevresource   存储地址范围的资源变量
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeRegAddressGet (PLW_DEVTREE_NODE  pdtnDev, PLW_DEV_RESOURCE  pdevresource)
{
    LW_DEV_RESOURCE   resTemp;
    PLW_DEV_RESOURCE  presTemp;
    INT               iNumReg = 0;
    INT               i;

    if (!pdevresource) {                                                /*  存储地址的资源变量不能为空  */
        return  (PX_ERROR);
    }

    while (!API_DeviceTreeResourceGet(pdtnDev, iNumReg, &resTemp)) {    /*  获取存储地址的 cells 数量   */
        iNumReg++;
    }

    presTemp = pdevresource;

    if (iNumReg) {
        for (i = 0; i < iNumReg; i++, presTemp++) {                     /*  获取地址资源                */
            API_DeviceTreeResourceGet(pdtnDev, i, presTemp);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeAddressIoremap
** 功能描述: 获取设备树中的资源地址，并进行映射
** 输　入  : pdtnDev     设备树节点
**           iIndex      设备树序号
** 输　出  : 映射出的虚拟地址
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreeFindNodeOptsByPath
** 功能描述: 通过节点路径查找设备树节点，并获取冒号分隔符之后的字符
** 输　入  : pcPath     节点路径
**                      有效的节点路径只有以下几种：
**                      （1）完整路径名，形如：/soc@03000000/spi@05010000
**                      （2）别名，形如：spi0
**                      （3）别名 + 相对路径名
**           ppcOpts    获取的分隔符之后的字符
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeFindNodeOptsByPath (CPCHAR  pcPath, CPCHAR  *ppcOpts)
{
    PLW_DEVTREE_NODE      pdtnDev     = LW_NULL;
    PLW_DEVTREE_PROPERTY  pdtprop;
    CPCHAR                pcSeparator;                                  /*  获取冒号分隔符的位置        */
    INT                   iLen;
    CPCHAR                pcTmp;

    if (!pcPath) {
        return  (LW_NULL);
    }

    pcSeparator = lib_strchr(pcPath, ':');                              /*  获取冒号分隔符的位置        */

    if (ppcOpts) {
        *ppcOpts = pcSeparator ? (pcSeparator + 1) : LW_NULL;           /*  取冒号之后的字符串          */
    }

    if (lib_strcmp(pcPath, "/") == 0) {                                 /*  如果是根节点                */
        return  (_G_pdtnRoot);
    }

    if (*pcPath != '/') {                                               /*  如果当前路径首字符不是 '/'  */
        if (!_G_pdtnAliases) {                                          /*  aliases 节点不能为空        */
            return  (LW_NULL);
        }

        pcTmp = pcSeparator;
        if (!pcTmp) {
            pcTmp = lib_strchrnul(pcPath, '/');                         /*  返回首字符或末尾空字符      */
        }
        iLen = pcTmp - pcPath;                                          /*  得到路径字符串长度          */

        for (pdtprop  = _G_pdtnAliases->DTN_pdtpproperties;
             pdtprop != LW_NULL;
             pdtprop  = pdtprop->DTP_pdtpNext) {                        /*  遍历 alises 节点每个属性    */

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

    if (!pdtnDev) {                                                     /*  若未找到，则从根节点开始查找*/
        pdtnDev = _G_pdtnRoot;
    }
    pdtnDev = __deviceTreeFindNodeByFullPath(pdtnDev, pcPath);

    return  (pdtnDev);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeNextChildGet
** 功能描述: 获取设备树节点的下一个孩子节点
** 输　入  : pdtnDev     设备树节点
**           pdtnPrev    前一个节点
** 输　出  : 下一个孩子节点
** 全局变量:
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DeviceTreeFindAllNodes
** 功能描述: 查找下一个节点
** 输　入  : pdtnPrev     起始节点
** 输　出  : 设备树节点
** 全局变量:
** 调用模块:
**                                            API 函数
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
