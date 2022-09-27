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
** 文   件   名: devtree.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 06 月 17 日
**
** 描        述: 设备树头文件
*********************************************************************************************************/

#ifndef __DEVTREE_H
#define __DEVTREE_H

/*********************************************************************************************************
  调试宏
*********************************************************************************************************/

#define DEVTREE_DEBUG_EN                     0

#if DEVTREE_DEBUG_EN > 0
#define DEVTREE_MSG(fmt, args...)            _DebugFormat(__PRINTMESSAGE_LEVEL, fmt, ##args)
#define DEVTREE_ERR(fmt, args...)            _DebugFormat(__ERRORMESSAGE_LEVEL, fmt, ##args)
#else
#define DEVTREE_MSG(fmt, args...)
#define DEVTREE_ERR(fmt, args...)
#endif

/*********************************************************************************************************
  默认定义
*********************************************************************************************************/

#define ROOT_NODE_SIZE_CELLS_DEFAULT         1
#define ROOT_NODE_ADDR_CELLS_DEFAULT         1

/*********************************************************************************************************
  无效地址定义
*********************************************************************************************************/

#define OF_BAD_ADDR                         (-1)

/*********************************************************************************************************
  加载标识定义
*********************************************************************************************************/

#define OF_POPULATED                         1
#define OF_POPULATED_BUS                     2
#define OF_PHANDLE_ILLEGAL                   0xdeadbeef

/*********************************************************************************************************
  资源计算宏
*********************************************************************************************************/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)                       (sizeof(x) / sizeof((x)[0]))
#endif
#define RESOURCE_SIZE(res)                  ((res).iomem.DEVRES_ulEnd - \
                                             (res).iomem.DEVRES_ulStart + 1)

/*********************************************************************************************************
  设备树相关定义
*********************************************************************************************************/

#define LW_DEVTREE_TABLE_END                {}

#define LW_DEVTREE_DECLARE(name, compatible, probefunc)     \
static LW_DEVTREE_TABLE  _G_##name##Match[] = {             \
    { .DTITEM_cCompatible = compatible,  },                 \
    {}                                                      \
};                                                          \
static LW_DRV_INSTANCE   _G_##name##Instance = {            \
    .DRVHD_pMatchTable = _G_##name##Match,                  \
    .DRVHD_pfuncProbe  = probefunc,                         \
}

#define LW_DEVTREE_MATCH_DECLARE(compatible, probefunc) {   \
    .DTITEM_cCompatible = compatible,                       \
    .DTITEM_pvData      = probefunc,                        \
}

/*********************************************************************************************************
  类型定义
*********************************************************************************************************/

typedef struct devtree_bus {
    CPCHAR                DTBUS_pcAddresses;
    INT                 (*DTBUS_match)(PLW_DEVTREE_NODE  pdtnParent);   /*  总线匹配                    */
    VOID                (*DTBUS_cellsCount)(PLW_DEVTREE_NODE pdtnChild,
                                            INT            *piCellAddr,
                                            INT            *piCellSize);/*  cells 数量获取              */
    UINT64              (*DTBUS_map)(UINT32  *puiAddr,
                                     UINT32  *puiRange,
                                     INT      iAddrNum,
                                     INT      iSizeNum,
                                     INT      iPAddrNum);
    INT                 (*DTBUS_translate)(UINT32  *puiAddr,
                                           UINT64   ullOffset,
                                           INT      iAddrNum);
} LW_DEVTREE_BUS;
typedef LW_DEVTREE_BUS               *PLW_DEVTREE_BUS;

typedef struct devtree_alias_prop {
    LW_LIST_LINE          DTALP_plineManage;                            /*  别名属性节点链表管理        */
    CPCHAR                DTALP_pcAlias;                                /*  别名属性节点名指针          */
    PLW_DEVTREE_NODE      DTALP_pdtnDev;                                /*  别名属性节点指向的设备树节点*/
    INT                   DTALP_iId;
    CHAR                  DTALP_cStem[0];                               /*  开辟实际存储节点名的内存    */
} LW_DEVTREE_ALIAS_PROPERTY;
typedef LW_DEVTREE_ALIAS_PROPERTY    *PLW_DEVTREE_ALIAS_PROPERTY;

typedef struct devtree_phandle_iterator {
    CPCHAR                DTPHI_pcCellsName;
    INT                   DTPHI_iCellCount;
    PLW_DEVTREE_NODE      DTPHI_pdtnParent;

    const UINT32         *DTPHI_puiListEnd;
    const UINT32         *DTPHI_puiPhandleEnd;

    const UINT32         *DTPHI_puiCurrent;
    UINT32                DTPHI_uiCurCount;
    UINT32                DTPHI_uiPhandle;
    PLW_DEVTREE_NODE      DTPHI_pdtnDev;
} LW_DEVTREE_PHANDLE_ITERATOR;
typedef LW_DEVTREE_PHANDLE_ITERATOR  *PLW_DEVTREE_PHANDLE_ITERATOR;

typedef struct devtree_pci_range_parser {
    PLW_DEVTREE_NODE      DTPRP_pdtnDev;
    const UINT32         *DTPRP_puiRange;
    const UINT32         *DTPRP_puiEnd;
    INT                   DTPRP_iNp;
    INT                   DTPRP_iPna;
} LW_DEVTREE_PCI_RANGE_PARSER;
typedef LW_DEVTREE_PCI_RANGE_PARSER  *PLW_DEVTREE_PCI_RANGE_PARSER;

typedef struct devtree_pci_range {
    UINT32                DTPR_uiPciSpace;
    UINT64                DTPR_ullPciAddr;
    UINT64                DTPR_ullCpuAddr;
    UINT64                DTPR_ullSize;
    UINT32                DTPR_uiFlags;
} LW_DEVTREE_PCI_RANGE;
typedef LW_DEVTREE_PCI_RANGE         *PLW_DEVTREE_PCI_RANGE;

typedef PVOID (*FUNC_DT_ALLOC)(size_t  stSize, size_t  stAlign);
typedef INT   (*FUNC_SCAN_CALLBACK)(PVOID   pvBase,  INT  iOffset,
                                    CPCHAR  cpcName, INT  iDepth, PVOID  pvData);
typedef INT   (*DEVTREE_INIT_FUNC)(PLW_DEVTREE_NODE  pdtnDev);

/*********************************************************************************************************
  节点固定属性获取
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeNAddrCells(PLW_DEVTREE_NODE  pdtnDev);

LW_API INT                   API_DeviceTreeNSizeCells(PLW_DEVTREE_NODE  pdtnDev);

/*********************************************************************************************************
  地址相关接口
*********************************************************************************************************/

LW_API UINT64                API_DeviceTreeAddressTranslate(PLW_DEVTREE_NODE  pdtnDev,
                                                            const UINT32     *puiInAddr);

LW_API INT                   API_DeviceTreeRegAddressGet(PLW_DEVTREE_NODE  pdtnDev,
                                                         PLW_DEV_RESOURCE  pdevresource);

LW_API const UINT32*         API_DeviceTreeAddressGet(PLW_DEVTREE_NODE  pdtnDev,
                                                      INT               iIndex,
                                                      UINT64           *pullSize);

LW_API INT                   API_DeviceTreeResourceGet(PLW_DEVTREE_NODE  pdtnDev,
                                                       INT               iIndex,
                                                       PLW_DEV_RESOURCE  pdevresource);

LW_API PVOID                 API_DeviceTreeAddressIoremap(PLW_DEVTREE_NODE  pdtnDev, INT  iIndex);

/*********************************************************************************************************
  属性读取接口
*********************************************************************************************************/

LW_API PVOID                 API_DeviceTreePropertyGet(PLW_DEVTREE_NODE  pdtnDev,
                                                       CPCHAR            pcName,
                                                       INT              *piLen);

LW_API INT                   API_DeviceTreePropertyU32Read(PLW_DEVTREE_NODE  pdtnDev,
                                                           CPCHAR            pcPropname,
                                                           UINT32           *puiOutValue);

LW_API INT                   API_DeviceTreePropertyU32ArrayRead(PLW_DEVTREE_NODE  pdtnDev,
                                                                CPCHAR            pcPropname,
                                                                UINT32           *puiOutValue,
                                                                size_t            stSize);

LW_API INT                   API_DeviceTreePropertyU32VaraiableArrayRead(const PLW_DEVTREE_NODE  pdtnDev,
                                                                         CPCHAR            pcPropname,
                                                                         UINT32           *puiOutValue,
                                                                         size_t            stMin,
                                                                         size_t            stMax);

LW_API UINT32*               API_DeviceTreePropertyU32Next(PLW_DEVTREE_PROPERTY  pdtproperty,
                                                           UINT32               *puiCur,
                                                           UINT32               *puiOut);

LW_API BOOL                  API_DeviceTreePropertyBoolRead(PLW_DEVTREE_NODE  pdtnDev,
                                                            CPCHAR            pcPropName);

LW_API INT                   API_DeviceTreePropertyU32IndexRead(PLW_DEVTREE_NODE  pdtnDev,
                                                                CPCHAR            pcPropname,
                                                                UINT32            uiIndex,
                                                                UINT32           *puiOutValue);

LW_API INT                   API_DeviceTreePropertyStringHelperRead(PLW_DEVTREE_NODE  pdtnDev,
                                                                    CPCHAR            pcPropName,
                                                                    CPCHAR           *ppcOutStrs,
                                                                    size_t            stSize,
                                                                    INT               iSkip,
                                                                    INT              *piCount);

LW_API CPCHAR                API_DeviceTreePropertyStringNext(PLW_DEVTREE_PROPERTY  pdtproperty,
                                                              CPCHAR                pcCur);

LW_API INT                   API_DeviceTreePropertyStringRead(PLW_DEVTREE_NODE  pdtnDev,
                                                              CPCHAR            pcPropName,
                                                              CPCHAR           *ppcOutString);

LW_API INT                   API_DeviceTreePropertyStringMatch(PLW_DEVTREE_NODE  pdtnDev,
                                                               CPCHAR            pcPropName,
                                                               CPCHAR            pcString);

LW_API INT                   API_DeviceTreePropertyStringIndexRead(const PLW_DEVTREE_NODE  pdtnDev,
                                                                   CPCHAR                  pcPropName,
                                                                   INT                     iIndex,
                                                                   CPCHAR                 *ppcOutPut);

LW_API PLW_DEVTREE_PROPERTY  API_DeviceTreePropertyFind(const PLW_DEVTREE_NODE  pdtnDev,
                                                        CPCHAR                  pcPropName,
                                                        INT                    *piLen);

LW_API INT                   API_DeviceTreePropertyStringCount(const PLW_DEVTREE_NODE  pdtnDev,
                                                               CPCHAR                  pcPropName);

LW_API BOOL                  API_DeviceTreeNodeIsOkayByOffset(PVOID  pvDevTree, INT  iOffset);

LW_API BOOL                  API_DeviceTreeNodeIsOkay(PLW_DEVTREE_NODE  pdtnDev);

LW_API INT                   API_DeviceTreeModaliasGet(PLW_DEVTREE_NODE  pdtnDev,
                                                       PCHAR             pcName,
                                                       INT               iLen);

/*********************************************************************************************************
  phandle 相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreePhandleIteratorInit(PLW_DEVTREE_PHANDLE_ITERATOR  pdtpiterator,
                                                               const PLW_DEVTREE_NODE        pdtnDev,
                                                               CPCHAR                        pcListName,
                                                               CPCHAR                        pcCellsName,
                                                               INT                           iCellCount);

LW_API INT                   API_DeviceTreePhandleIteratorNext(PLW_DEVTREE_PHANDLE_ITERATOR  pdtpiterator);

LW_API INT                   API_DeviceTreePhandleCountWithArgs(const PLW_DEVTREE_NODE     pdtnDev,
                                                                CPCHAR                     pcListName,
                                                                CPCHAR                     pcCellsName);

LW_API INT                   API_DeviceTreePhandleParseFixedArgs(const PLW_DEVTREE_NODE    pdtnDev,
                                                                 CPCHAR                    pcListName,
                                                                 CPCHAR                    pcCellsName,
                                                                 INT                       iCellCount,
                                                                 INT                       iIndex,
                                                                 PLW_DEVTREE_PHANDLE_ARGS  pdtpaOutArgs);

LW_API INT                   API_DeviceTreePhandleParseWithArgs(const PLW_DEVTREE_NODE     pdtnDev,
                                                                CPCHAR                     pcListName,
                                                                CPCHAR                     pcCellsName,
                                                                INT                        iIndex,
                                                                PLW_DEVTREE_PHANDLE_ARGS   pdtpargsOut);

LW_API PLW_DEVTREE_NODE      API_DeviceTreePhandleParse(const PLW_DEVTREE_NODE     pdtnDev,
                                                        CPCHAR                     pcPhandleName,
                                                        INT                        iIndex);

/*********************************************************************************************************
  查找相关接口
*********************************************************************************************************/

LW_API PLW_DEVTREE_NODE      API_DeviceTreeFindNodeOptsByPath(CPCHAR  pcPath, CPCHAR  *ppcOpts);

LW_API PLW_DEVTREE_NODE      API_DeviceTreeFindNodeByPhandle(UINT32  uiHandle);

LW_API PLW_DEVTREE_NODE      API_DeviceTreeFindAllNodes(PLW_DEVTREE_NODE  pdtnPrev);

/*********************************************************************************************************
  遍历相关接口
*********************************************************************************************************/

LW_API PLW_DEVTREE_NODE      API_DeviceTreeNextChildGet(const PLW_DEVTREE_NODE  pdtnDev,
                                                              PLW_DEVTREE_NODE  pdtnPrev);

/*********************************************************************************************************
  系统相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeLowLevelInit(PVOID  pvDevtreeMem);

LW_API INT                   API_DeviceTreeHighLevelInit(PVOID  pvDevTreeMem);

LW_API ssize_t               API_DeviceTreeKernelStartParamGet(PCHAR  pcParam, size_t  stLen);

LW_API INT                   API_DeviceTreeKernelVmmParamGet(PVOID                  pvDevtreeMem,
                                                             PLW_MMU_PHYSICAL_DESC  pphyOriginDesc,
                                                             size_t                 stPhyOriginDesc,
                                                             PLW_MMU_PHYSICAL_DESC *ppphydesc,
                                                             PLW_MMU_VIRTUAL_DESC  *ppvirdes);

LW_API INT                   API_DeviceTreeAliasIdGet(PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcStem);

LW_API INT                   API_DeviceTreeDevPopulate(PLW_DEVTREE_NODE   pdtnDev,
                                                       PLW_DEVTREE_TABLE  pdttMatch);

LW_API INT                   API_DeviceTreeDefaultPopulate(VOID);

LW_API INT                   API_DeviceTreeDrvMatchDev(PLW_DEV_INSTANCE  pdevinstance,
                                                       PLW_DRV_INSTANCE  pdrvinstance);

LW_API INT                   API_DeviceTreeDevEarlyInit(PLW_DEVTREE_TABLE  pdttMatch);

LW_API INT                   API_DeviceTreeIsCompatible(PLW_DEVTREE_NODE  pdtnDev,
                                                        PCHAR             pcCompat);

LW_API PVOID                 API_DeviceTreeDevGetMatchData(PLW_DEV_INSTANCE  pdevinstance);

LW_API PLW_DEVTREE_TABLE     API_DeviceTreeDevGetMatchTable(PLW_DEVTREE_TABLE  pdttMatches,
                                                            PLW_DEVTREE_NODE   pdtnDev);

/*********************************************************************************************************
  中断相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeIrqOneParse(PLW_DEVTREE_NODE          pdtnDev,
                                                       INT                       iIndex,
                                                       PLW_DEVTREE_PHANDLE_ARGS  pdtpaOut);

LW_API INT                   API_DeviceTreeIrqCountGet(PLW_DEVTREE_NODE  pdtnDev);

LW_API PLW_DEVTREE_NODE      API_DeviceTreeIrqFindParent(PLW_DEVTREE_NODE  pdtnChild);

LW_API INT                   API_DeviceTreeIrqRawParse(const  UINT32             *puiAddr,
                                                       PLW_DEVTREE_PHANDLE_ARGS   pdtpaOutIrq);

LW_API INT                   API_DeviceTreeIrqToResource(PLW_DEVTREE_NODE  pdtnDev,
                                                         INT               iIndex,
                                                         PLW_DEV_RESOURCE  pdevresource);

LW_API INT                   API_DeviceTreeIrqToResouceTable(PLW_DEVTREE_NODE  pdtnDev,
                                                             PLW_DEV_RESOURCE  pdevresource,
                                                             INT               iNrIrqs);

LW_API INT                   API_DeviceTreeIrqGet(PLW_DEVTREE_NODE   pdtnDev,
                                                  INT                iIndex,
                                                  ULONG             *pulVector);

/*********************************************************************************************************
  pinctrl 相关接口
*********************************************************************************************************/

LW_API VOID                  API_DeviceTreePinCtrlMapsFree(PLW_PINCTRL   ppinctrl);

LW_API INT                   API_DeviceTreePinCtrlMapsCreate(PLW_PINCTRL   ppinctrl);

LW_API PLW_PINCTRL_DEV       API_DeviceTreePinCtrlDevGet(PLW_DEVTREE_NODE  pdtnDev);

/*********************************************************************************************************
  clock 相关接口
*********************************************************************************************************/

LW_API PLW_CLOCK             API_DeviceTreeClockGetByName(PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcName);

LW_API CPCHAR                API_DeviceTreeParentClockNameGet(PLW_DEVTREE_NODE  pdtnDev, INT  iIndex);

LW_API PLW_CLOCK             API_DeviceTreeClockGet(PLW_DEVTREE_NODE  pdtnDev, INT  iIndex);

/*********************************************************************************************************
  MDIO 相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeMdioRegister(PMDIO_ADAPTER      pmdioadapter,
                                                        PLW_DEVTREE_NODE   pdtnDev);

LW_API INT                   API_DeviceTreeMdioDevRegister(PMDIO_ADAPTER      pmdioadapter,
                                                           PLW_DEVTREE_NODE   pdtnDev,
                                                           UINT               uiAddr);

LW_API MDIO_DEVICE          *API_DeviceTreeMdioDevFind(PLW_DEVTREE_NODE  pdtnDev);

/*********************************************************************************************************
  I2C 相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeI2cAdapterRegister(PLW_DT_I2C_ADAPTER  pi2cadapter,
                                                              PLW_DEVTREE_NODE    pdtnDev,
                                                              CPCHAR              pcName);

LW_API INT                   API_DeviceTreeI2cDevRegister(PLW_DT_I2C_ADAPTER  pi2cadapter,
                                                          PLW_DEVTREE_NODE    pdtnDev);

/*********************************************************************************************************
  SPI 相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeSpiCtrlRegister(PLW_DT_SPI_CTRL    pspictrl,
                                                           PLW_DEVTREE_NODE   pdtnDev,
                                                           CPCHAR             pcName);

LW_API INT                   API_DeviceTreeSpiDevRegister(PLW_DT_SPI_CTRL    pspictrl,
                                                          PLW_DEVTREE_NODE   pdtnDev);

/*********************************************************************************************************
  PCI 相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreePciBusRangeParse(PLW_DEVTREE_NODE  pdtnDev,
                                                            PLW_DEV_RESOURCE  pdevresource);

LW_API INT                   API_DeviceTreePciHostBridgeResourcesGet(PLW_DEVTREE_NODE     pdtnDev,
                                                                     UINT8                ucBusNo,
                                                                     UINT8                ucBusMax,
                                                                     LW_LIST_LINE_HEADER *pplineheadResource,
                                                                     UINT64              *pullIoBase);

LW_API INT                   API_DeviceTreePciRangesParse(PLW_DEVTREE_NODE     pdtnDev,
                                                          LW_LIST_LINE_HEADER *pplineheadResource,
                                                          PLW_DEV_RESOURCE    *pdevresBusRange);

/*********************************************************************************************************
  GPIO 相关接口
*********************************************************************************************************/

LW_API INT                   API_DeviceTreeGpioCtrlRegister(PLW_DT_GPIO_CTRL     pdtgpioctrl,
                                                            CPCHAR               pcName);

LW_API VOID                  API_DeviceTreeGpioCtrlRemove(PLW_DT_GPIO_CTRL       pdtgpioctrl);

LW_API INT                   API_DeviceTreeGpioPinRangeAdd(PLW_DT_GPIO_CTRL      pdtgpioctrl,
                                                           PLW_PINCTRL_DEV       ppinctldev,
                                                           UINT                  uiGpioOffset,
                                                           UINT                  uiPinOffset,
                                                           UINT                  uiNPins);

LW_API VOID                  API_DeviceTreeGpioPinRangeRemove(PLW_DT_GPIO_CTRL   pdtgpioctrl);

LW_API INT                   API_DeviceTreeGpioNamedGpioGet(PLW_DEVTREE_NODE     pdtnDev,
                                                            CPCHAR               pcListName,
                                                            INT                  iIndex);

LW_API INT                   API_DeviceTreeGpioNamedCountGet(PLW_DEVTREE_NODE    pdtnDev,
                                                             CPCHAR              pcListName);

#include "devtree_error.h"
#include "devtree_inline.h"
#include "devtree_value.h"

#endif                                                                  /*  __DEVTREE_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
