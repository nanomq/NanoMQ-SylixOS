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
** 文   件   名: devtreeHighLevel.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 07 月 30 日
**
** 描        述: 设备树接口高层接口系统内核实现
**
**                      +------+
**                      | Root |
**                      +------+
**                       __|
**                      |
**                   +------+  +------+         +------+
**                   | Devm |--| Devn |-- ....--| Dev..|
**                   +------+  +------+         +------+
**                    __|         |__ ....         |__....
**                    |
**                +------+  +------+         +------+
**                | Devx |--| Devy |-- ....--| Dev..|
**                +------+  +------+         +------+
**              ....__|         |__ ....         |__ ....
**
**              设备树的树型结构，只有一个父节点，每个父节点只有一个子节点，
**          但子节点会有很多同一层次的兄弟节点。所有的兄弟节点都会指向该父节点。
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
#include "linux/log2.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define DEVTREE_MEM_GUARD           0xdeadbeef                          /*  内存警戒区数据              */
#define DEVTREE_MEM_GUARD_SIZE      4                                   /*  内存警戒区大小              */
#define FDT_MAX_DEPTH               64                                  /*  支持的最大树深度            */

#ifndef __GNUC__
#define __alignof__(x)              8
#endif
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineheadAliasesLookup;
/*********************************************************************************************************
** 函数名称: __deviceTreeMemoryPartGet
** 功能描述: 从设备树申请的内存中获取一块
** 输　入  : ppvMem         申请的内存基址（会改变）
**           stSize         申请的内存大小
**           stAlign        申请的对齐大小
** 输　出  : 申请的内存地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __deviceTreeMemoryPartGet (PVOID  *ppvMem, size_t  stSize, size_t  stAlign)
{
    PVOID  pvRes;

    *ppvMem = (PVOID)ROUND_UP(*ppvMem, stAlign);
    pvRes   = *ppvMem;
    *ppvMem = (PVOID)((size_t)*ppvMem + stSize);

    return  (pvRes);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemoryAlloc
** 功能描述: 设备树内存申请接口
** 输　入  : stSize         申请的内存大小
**           stAlign        申请的对齐大小
** 输　出  : 申请的内存地址
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  __deviceTreeMemoryAlloc (size_t  stSize, size_t  stAlign)
{
    return  (__SHEAP_ALLOC_ALIGN(stSize, stAlign));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemGuardSet
** 功能描述: 设置内存保护标志
** 输　入  : pvMem        内存基址
**           iSize        内存大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreeMemGuardSet (PVOID  pvMem, INT  iSize)
{
    *(UINT32 *)((addr_t)pvMem + iSize) = DEVTREE_MEM_GUARD;
}
/*********************************************************************************************************
** 函数名称: __deviceTreeMemGuardGet
** 功能描述: 获取内存保护标志
** 输　入  : pvMem        内存基址
**           iSize        内存大小
** 输　出  : 内存保护标志位置当前值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeMemGuardGet (PVOID  pvMem, INT  iSize)
{
    return  (*(UINT32 *)((addr_t)pvMem + iSize));
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodePropertiesPopulate
** 功能描述: 生成设备树属性内容
** 输　入  : pvDevTree       设备树基地址
**           iOffset         节点偏移
**           ppvMem          内存偏移
**           pdtnDev         当前用于生成属性的节点
**           pcNodeName      节点名称
**           bIsSkip         是否只遍历，不处理具体过程
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreeNodePropertiesPopulate (CPVOID            pvDevTree,
                                                 INT               iOffset,
                                                 PVOID            *ppvMem,
                                                 PLW_DEVTREE_NODE  pdtnDev,
                                                 CPCHAR            pcNodeName,
                                                 BOOL              bIsSkip)
{
    PLW_DEVTREE_PROPERTY  *ppdtpprev;
    PLW_DEVTREE_PROPERTY   pdtpcurr;
    INT                    iCur;
    INT                    iSize;
    BOOL                   bIsHasName = LW_FALSE;
    const UINT32          *puiVal;
    CPCHAR                 pcName;
    CPCHAR                 pcTmp;
    CPCHAR                 pcAddr;
    CPCHAR                 pcStart;

    ppdtpprev = &pdtnDev->DTN_pdtpproperties;

    for (iCur  = fdt_first_property_offset(pvDevTree, iOffset);
         iCur >= 0;
         iCur  = fdt_next_property_offset(pvDevTree, iCur)) {           /*  从指定偏移开始遍历属性节点  */

        puiVal = fdt_getprop_by_offset(pvDevTree,
                                       iCur,
                                       &pcName,
                                       &iSize);                         /*  获取属性                    */
        if (!puiVal) {
            DEVTREE_ERR("Cannot locate property at 0x%x\r\n", iCur);
            continue;
        }

        if (!pcName) {
            DEVTREE_ERR("Cannot find property name at 0x%x\r\n", iCur);
            continue;
        }

        if (!lib_strcmp(pcName, "name")) {                              /*  若属性名为 "name"           */
            bIsHasName = LW_TRUE;
        }

        pdtpcurr = __deviceTreeMemoryPartGet(ppvMem,                    /*  为属性分配内存              */
                                             sizeof(LW_DEVTREE_PROPERTY),
                                             __alignof__(LW_DEVTREE_PROPERTY));

        if (bIsSkip) {                                                  /*  如果只遍历，则直接下一个    */
            continue;
        }

        if (!strcmp(pcName, "phandle") && !pdtnDev->DTN_uiHandle) {     /*  若属性名为 "phandle"        */
            pdtnDev->DTN_uiHandle = BE32_TO_CPU(puiVal);                /*  记录句柄值                  */
        }

        pdtpcurr->DTP_pcName  = pcName;                                 /*  记录属性的名称              */
        pdtpcurr->DTP_iLength = iSize;                                  /*  记录属性值的长度            */
        pdtpcurr->DTP_pvValue = (UINT32 *)puiVal;                       /*  记录属性值的内容            */
       *ppdtpprev             = pdtpcurr;
        ppdtpprev             = &pdtpcurr->DTP_pdtpNext;
    }

    if (!bIsHasName) {                                                  /*  如果属性中有全路径名称      */
        pcTmp   = pcNodeName;
        pcStart = pcTmp;
        pcAddr  = LW_NULL;

        /*
         *  形如："/serial@e2900800" 这样的属性
         *  通过以下循环可以获取到值 "serial"
         */
        while (*pcTmp) {                                                /*  逐个字符查找                */
            if (*pcTmp == '@') {                                        /*  记录节点地址                */
                pcAddr = pcTmp;
            } else if (*pcTmp == '/') {
                pcStart = pcTmp + 1;
            }
            pcTmp++;
        }

        if (pcAddr < pcStart) {
            pcAddr = pcTmp;
        }

        iSize    = (pcAddr - pcStart) + 1;
        pdtpcurr = __deviceTreeMemoryPartGet(ppvMem,
                                             sizeof(LW_DEVTREE_PROPERTY) + iSize,
                                             __alignof__(LW_DEVTREE_PROPERTY));
        if (!bIsSkip) {
            pdtpcurr->DTP_pcName  = "name";
            pdtpcurr->DTP_iLength = iSize;
            pdtpcurr->DTP_pvValue = pdtpcurr + 1;
           *ppdtpprev             = pdtpcurr;
            ppdtpprev             = &pdtpcurr->DTP_pdtpNext;

            lib_memcpy(pdtpcurr->DTP_pvValue, pcStart, iSize - 1);
            ((PCHAR)pdtpcurr->DTP_pvValue)[iSize - 1] = 0;

            DEVTREE_MSG("Fixed up name for %s -> %s\r\n",
                        pcNodeName, pdtpcurr->DTP_pvValue);
        }
    }

    if (!bIsSkip) {
        *ppdtpprev = LW_NULL;
    }
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodePopulate
** 功能描述: 生成设备树节点
** 输　入  : pvDevTree       设备树基地址
**           iOffset         节点偏移
**           ppvMem          开辟的内存偏移
**           pdtnFather      父节点
**           ppdtnCur        用于获取当前节点
**           bIsSkip         是否只遍历，不处理具体过程
** 输　出  : LW_TRUE 表示生成成功，LW_FALSE 表示生成失败
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __deviceTreeNodePopulate (PVOID             pvDevTree,
                                       INT               iOffset,
                                       PVOID            *ppvMem,
                                       PLW_DEVTREE_NODE  pdtnFather,
                                       PLW_DEVTREE_NODE *ppdtnCur,
                                       BOOL              bIsSkip)
{
    PLW_DEVTREE_NODE  pdtnPtr;
    CPCHAR            pcPath;
    INT               iAllocLen;
    INT               iLen;

    pcPath = fdt_get_name(pvDevTree, iOffset, &iLen);                   /*  获取节点全路径名            */
    if (!pcPath) {
        *ppdtnCur = LW_NULL;
        return  (LW_FALSE);
    }

    iAllocLen = iLen + 1;                                               /*  内存大小需要增加 Full Name  */
    pdtnPtr   = __deviceTreeMemoryPartGet(ppvMem,                       /*  从指定内存中取出一块        */
                                          sizeof(LW_DEVTREE_NODE) + iAllocLen,
                                          __alignof__(LW_DEVTREE_NODE));
    if (!bIsSkip) {
        pdtnPtr->DTN_pcFullName = (PCHAR)pdtnPtr + sizeof(LW_DEVTREE_NODE);
        lib_memcpy((PVOID)pdtnPtr->DTN_pcFullName, pcPath, iAllocLen);
                                                                        /*  记录节点全路径名            */
        if (pdtnFather != LW_NULL) {
            pdtnPtr->DTN_pdtnparent   = pdtnFather;                     /*  记录父节点                  */
            pdtnPtr->DTN_pdtnsibling  = pdtnFather->DTN_pdtnchild;      /*  记录兄弟节点                */
            pdtnFather->DTN_pdtnchild = pdtnPtr;                        /*  更新父节点的子节点          */
        }
    }

    __deviceTreeNodePropertiesPopulate(pvDevTree,
                                       iOffset,
                                       ppvMem,
                                       pdtnPtr,
                                       pcPath,
                                       bIsSkip);                        /*  填充属性结构                */
    if (!bIsSkip) {
        pdtnPtr->DTN_pcName = API_DeviceTreePropertyGet(pdtnPtr,
                                                        "name",
                                                        LW_NULL);       /*  从属性结构中获取名称        */
        pdtnPtr->DTN_pcType = API_DeviceTreePropertyGet(pdtnPtr,
                                                        "device_type",
                                                        LW_NULL);       /*  从属性结构中获取名称        */
        if (!pdtnPtr->DTN_pcName) {
            pdtnPtr->DTN_pcName = "<NULL>";
        }

        if (!pdtnPtr->DTN_pcType) {
            pdtnPtr->DTN_pcType = "<NULL>";
        }
    }

    *ppdtnCur = pdtnPtr;                                                /*  记录当前解析到的节点        */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeNodesReverse
** 功能描述: 反向排序设备树节点
** 输　入  : pdtnParent       设备树父节点
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreeNodesReverse (PLW_DEVTREE_NODE  pdtnParent)
{
    PLW_DEVTREE_NODE  pdtnChild;
    PLW_DEVTREE_NODE  pdtnNext;

    pdtnChild = pdtnParent->DTN_pdtnchild;                              /*  获得当前的子节点            */
    while (pdtnChild) {                                                 /*  递归子节点的子节点          */
        __deviceTreeNodesReverse(pdtnChild);
        pdtnChild = pdtnChild->DTN_pdtnsibling;                         /*  遍历每一个兄弟节点          */
    }

    pdtnChild = pdtnParent->DTN_pdtnchild;                              /*  执行到此，为最后一个兄弟节点*/
    pdtnParent->DTN_pdtnchild = LW_NULL;
    while (pdtnChild) {
        pdtnNext = pdtnChild->DTN_pdtnsibling;                          /*  找到下一个兄弟节点          */

        pdtnChild->DTN_pdtnsibling = pdtnParent->DTN_pdtnchild;
        pdtnParent->DTN_pdtnchild  = pdtnChild;
        pdtnChild                  = pdtnNext;
    }
}
/*********************************************************************************************************
** 函数名称: __deviceTreeAliasAdd
** 功能描述: 将别名属性节点插入别名属性查找链表
** 输　入  : pdtaprop       别名属性节点
**           pdtnDev        设备树节点
**           iId            id 值
**           pcStem         设备名称
**           iStemLen       设备名称长度
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreeAliasAdd (PLW_DEVTREE_ALIAS_PROPERTY  pdtaprop,
                                   PLW_DEVTREE_NODE            pdtnDev,
                                   INT                         iId,
                                   CPCHAR                      pcStem,
                                   INT                         iStemLen)
{
    pdtaprop->DTALP_pcAlias = pcStem;
    pdtaprop->DTALP_pdtnDev = pdtnDev;
    pdtaprop->DTALP_iId     = iId;

    lib_strncpy(pdtaprop->DTALP_cStem, pcStem, iStemLen);
    pdtaprop->DTALP_cStem[iStemLen] = 0;

    _List_Line_Add_Ahead(&pdtaprop->DTALP_plineManage,
                         &_G_plineheadAliasesLookup);

    DEVTREE_MSG("Adding DT alias:%s: stem=%s id=%d\r\n",
                pdtaprop->DTALP_pcAlias,
                pdtaprop->DTALP_cStem,
                pdtaprop->DTALP_iId);
}
/*********************************************************************************************************
** 函数名称: __deviceTreeAliasScan
** 功能描述: 对别名属性节点进行扫描
** 输　入  : pfuncDtAlloc    内存申请函数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreeAliasScan (FUNC_DT_ALLOC  pfuncDtAlloc)
{
    PLW_DEVTREE_ALIAS_PROPERTY  pdtaprop;
    PLW_DEVTREE_PROPERTY        pdtpCur;
    PLW_DEVTREE_NODE            pdtnDev;
    CPCHAR                      pcStart;
    CPCHAR                      pcEnd;
    INT                         iLen;
    INT                         iId;

    _G_pdtnAliases = __deviceTreeFindNodeByPath("/aliases");            /*  找到 aliases 节点           */
    if (!_G_pdtnAliases) {
        return;
    }

    for (pdtpCur  = _G_pdtnAliases->DTN_pdtpproperties;
         pdtpCur != LW_NULL;
         pdtpCur  = pdtpCur->DTP_pdtpNext) {

        if (!strcmp(pdtpCur->DTP_pcName, "name")    ||
            !strcmp(pdtpCur->DTP_pcName, "phandle") ||
            !strcmp(pdtpCur->DTP_pcName, "linux,phandle")) {            /*  跳过不需要关心的数据        */
            continue;
        }

        pcStart = pdtpCur->DTP_pcName;
        pcEnd   = pcStart + lib_strlen(pcStart);

        pdtnDev = __deviceTreeFindNodeByPath(pdtpCur->DTP_pvValue);     /*  通过全路径名查找设备树节点  */
        if (!pdtnDev) {
            continue;
        }

        while (lib_isdigit(*(pcEnd - 1)) && (pcEnd > pcStart)) {        /*  找到数字起始的位置          */
            pcEnd--;
        }

        iLen = pcEnd - pcStart;
        iId  = lib_atoi(pcEnd);
        if (iId < 0) {                                                  /*  如果数字解析小于 0          */
            continue;
        }

        pdtaprop = pfuncDtAlloc(sizeof(LW_DEVTREE_ALIAS_PROPERTY) + iLen + 1,
                                __alignof__(LW_DEVTREE_ALIAS_PROPERTY));
        if (!pdtaprop) {
            return;
        }

        lib_bzero(pdtaprop, sizeof(LW_DEVTREE_ALIAS_PROPERTY) + iLen + 1);

        __deviceTreeAliasAdd(pdtaprop, pdtnDev,
                             iId, pcStart, iLen);                       /*  将 Alias 插入链表           */
    }
}
/*********************************************************************************************************
** 函数名称: __deviceTreePhandleCachePopulate
** 功能描述: 生成 Phandle Cache 信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __deviceTreePhandleCachePopulate (VOID)
{
    PLW_DEVTREE_NODE   pdtnDev;
    UINT32             uiCacheEntries;
    UINT32             uiPhandles = 0;

    _LIST_EACH_OF_ALLNODES(pdtnDev) {                                   /*  遍历结点，统计 phandle 数量 */
        if (pdtnDev->DTN_uiHandle) {
            uiPhandles++;
        }
    }

    if (!uiPhandles) {                                                  /*  如果没有 phandle，返回      */
        return;
    }

    uiCacheEntries        = roundup_pow_of_two(uiPhandles);             /*  找出小于 phandle 的 2^n 数  */
    _G_uiPhandleCacheMask = uiCacheEntries - 1;                         /*  计算 phandle 的掩码         */

    _G_ppdtnPhandleCache = __SHEAP_ZALLOC(uiCacheEntries * sizeof(PLW_DEVTREE_NODE));
    if (!_G_ppdtnPhandleCache) {                                        /*  开辟 phandle Cache 内存     */
        return;
    }

    _LIST_EACH_OF_ALLNODES(pdtnDev) {
        if (pdtnDev->DTN_uiHandle) {                                    /*  根据 phandle 索引填充 Cache */
            _G_ppdtnPhandleCache[pdtnDev->DTN_uiHandle & _G_uiPhandleCacheMask] = pdtnDev;
        }
    }
}
/*********************************************************************************************************
** 函数名称: __deviceTreeUnflattenNodes
** 功能描述: 进行设备树型结构解析;
**           当 pvMem 为空时，bIsSkip 为 LW_TRUE，此时只快速遍历，不处理过程，可快速得到树大小。
** 输　入  : pvDevTree       设备树基地址
**           pvMem           设备树开辟的内存基址
**           pdtnFather      父节点
**           ppdtnCur        用于获取当前节点
** 输　出  : 正确时返回当前遍历过的设备树内存偏移，
**           错误时返回 PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __deviceTreeUnflattenNodes (PVOID             pvDevTree,
                                       PVOID             pvMem,
                                       PLW_DEVTREE_NODE  pdtnFather,
                                       PLW_DEVTREE_NODE *ppdtnCur)
{
    PLW_DEVTREE_NODE  pdtnPointers[FDT_MAX_DEPTH];
    PLW_DEVTREE_NODE  pdtnRoot;
    INT               iOffset;
    INT               iDepth;
    INT               iInitialDepth;
    PVOID             pvBase  = pvMem;
    BOOL              bIsSkip = !pvMem;                                 /*  是否只遍历，不处理过程      */

    if (ppdtnCur) {                                                     /*  首先将当前节点置为 NULL     */
        *ppdtnCur = LW_NULL;
    }

    if (pdtnFather) {                                                   /*  如果有父节点                */
        iDepth        = 1;                                              /*  初始的树深度设置为 1        */
        iInitialDepth = 1;
    } else {
        iDepth        = 0;
        iInitialDepth = 0;
    }

    pdtnRoot             = pdtnFather;                                  /*  当前的根节点为 父节点       */
    pdtnPointers[iDepth] = pdtnFather;

    for (iOffset  = 0;
         (iOffset >= 0) && (iDepth >= iInitialDepth);                   /*  还没有遍历完整设备树        */
         iOffset  = fdt_next_node(pvDevTree, iOffset, &iDepth)) {

        if (iDepth >= FDT_MAX_DEPTH) {                                  /*  如果已经超出最大深度        */
            _ErrorHandle(ERROR_DEVTREE_DEPTH_ERROR);
            return  (PX_ERROR);
        }

        if (!API_DeviceTreeNodeIsOkayByOffset(pvDevTree, iOffset)) {    /*  如果节点不可用              */
            continue;
        }

        if (!__deviceTreeNodePopulate(pvDevTree,
                                      iOffset,
                                      &pvMem,                           /*  过程中会更新 pvMem 的位置   */
                                      pdtnPointers[iDepth],
                                      &pdtnPointers[iDepth + 1],
                                      bIsSkip)) {                       /*  生成可用的节点              */
             return  ((addr_t)pvMem - (addr_t)pvBase);                  /*  计算需要的内存大小          */
        }

        if (!bIsSkip &&                                                 /*  需要记录节点                */
            ppdtnCur &&
            (*ppdtnCur == LW_NULL)) {                                   /*  当前访问到的节点为空时，    */
           *ppdtnCur = pdtnPointers[iDepth + 1];                        /*  记录当前访问到最后的节点    */
        }

        if (!bIsSkip && !pdtnRoot) {                                    /*  需要记录节点且根节点为空    */
            pdtnRoot = pdtnPointers[iDepth + 1];                        /*  记录根节点                  */
        }
    }

    if ((iOffset < 0) && (iOffset != -FDT_ERR_NOTFOUND)) {              /*  如果设备树解析出错          */
        _ErrorHandle(ERROR_DEVTREE_POPULATE_ERROR);
        return  (PX_ERROR);
    }

    if (!bIsSkip) {                                                     /*  如果设备树解析结束          */
        __deviceTreeNodesReverse(pdtnRoot);                             /*  反向排序树的结构            */
    }

    return  ((addr_t)pvMem - (addr_t)pvBase);                           /*  计算需要的内存大小          */
}
/*********************************************************************************************************
** 函数名称: __deviceTreeUnflatten
** 功能描述: 按照设备树树形结构展开设备树。
**           先快速遍历一次设备树结构，获取设备树大小，再进行实际的设备树解析
** 输　入  : pvDevtreeMem    设备树基地址
**           pdtnFather      父节点
**           ppdtnCur        用于获取当前节点
**           pfuncDtAlloc    内存申请函数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __deviceTreeUnflatten (PVOID              pvDevtreeMem,
                                   PLW_DEVTREE_NODE   pdtnFather,
                                   PLW_DEVTREE_NODE  *ppdtnCur,
                                   FUNC_DT_ALLOC      pfuncDtAlloc)
{
    PVOID  pvMem;
    INT    iSize;

    DEVTREE_MSG("Start unflatten device tree:\r\n");
    DEVTREE_MSG("magic  : %08x\r\n", fdt_magic(pvDevtreeMem));
    DEVTREE_MSG("size   : %08x\r\n", fdt_totalsize(pvDevtreeMem));
    DEVTREE_MSG("version: %08x\r\n", fdt_version(pvDevtreeMem));

    iSize = __deviceTreeUnflattenNodes(pvDevtreeMem, LW_NULL,
                                       pdtnFather,   LW_NULL);          /*  获取设备树大小              */
    if (iSize < 0) {
        return  (PX_ERROR);
    }

    iSize = ROUND_UP(iSize, 4);                                         /*  找对齐的内存大小            */

    DEVTREE_MSG("Device tree is allocating %d bytes...\r\n", iSize);

    pvMem = pfuncDtAlloc(iSize + DEVTREE_MEM_GUARD_SIZE,
                         __alignof__(LW_DEVTREE_NODE));                 /*  申请设备树所需内存          */
    if (!pvMem) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    lib_bzero(pvMem, iSize);

    __deviceTreeMemGuardSet(pvMem, iSize);                              /*  设置内存警戒标志            */

    __deviceTreeUnflattenNodes(pvDevtreeMem, pvMem,
                               pdtnFather, ppdtnCur);                   /*  正式展开设备树              */

    if (__deviceTreeMemGuardGet(pvMem, iSize) != DEVTREE_MEM_GUARD) {   /*  如果产生了越界              */
        _ErrorHandle(ERROR_DEVTREE_MEM_OVERLAP);
        return  (PX_ERROR);
    }

    DEVTREE_MSG("Device tree unflattened success!\r\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeAliasIdGet
** 功能描述: 从别名属性链表中查找对应设备的 ID
** 输　入  : pdtnDev       设备树节点
**           pcStem        设备名称
** 输　出  : 设备的 ID
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeAliasIdGet (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcStem)
{
    PLW_DEVTREE_ALIAS_PROPERTY  pdtaprop;
    PLW_LIST_LINE               plineTemp;
    INT                         iId = -ENODEV;

    for (plineTemp  = _G_plineheadAliasesLookup;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pdtaprop = _LIST_ENTRY(plineTemp, LW_DEVTREE_ALIAS_PROPERTY, DTALP_plineManage);
        if (lib_strcmp(pdtaprop->DTALP_cStem, pcStem) != 0) {
            continue;
        }

        if (pdtnDev == pdtaprop->DTALP_pdtnDev) {
            iId = pdtaprop->DTALP_iId;
            break;
        }
    }

    return  (iId);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeHighLevelInit
** 功能描述: 设备树高级初始化，需要在系统初始化完成后调用
** 输　入  : pvDevtreeMem        设备树地址
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeHighLevelInit (PVOID  pvDevtreeMem)
{
    if (!pvDevtreeMem) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (PX_ERROR);
    }

    if (fdt_check_header(pvDevtreeMem)) {                               /*  设备树头部校验              */
        _ErrorHandle(ERROR_DEVTREE_MAGIC_ERROR);
        return  (PX_ERROR);
    }

    if (__deviceTreeUnflatten(pvDevtreeMem,
                              LW_NULL,
                              &_G_pdtnRoot,
                              __deviceTreeMemoryAlloc)) {               /*  实际生成设备树树形结构      */
        return  (PX_ERROR);
    }

    __deviceTreeAliasScan(__deviceTreeMemoryAlloc);                     /*  生成 Alias 节点             */

    __deviceTreePhandleCachePopulate();                                 /*  生成 Phandle Cache 信息     */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
