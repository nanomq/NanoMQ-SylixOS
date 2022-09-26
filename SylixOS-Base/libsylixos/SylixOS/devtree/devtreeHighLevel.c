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
** ��   ��   ��: devtreeHighLevel.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 07 �� 30 ��
**
** ��        ��: �豸���ӿڸ߲�ӿ�ϵͳ�ں�ʵ��
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
**              �豸�������ͽṹ��ֻ��һ�����ڵ㣬ÿ�����ڵ�ֻ��һ���ӽڵ㣬
**          ���ӽڵ���кܶ�ͬһ��ε��ֵܽڵ㡣���е��ֵܽڵ㶼��ָ��ø��ڵ㡣
**
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
#include "linux/log2.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define DEVTREE_MEM_GUARD           0xdeadbeef                          /*  �ڴ澯��������              */
#define DEVTREE_MEM_GUARD_SIZE      4                                   /*  �ڴ澯������С              */
#define FDT_MAX_DEPTH               64                                  /*  ֧�ֵ���������            */

#ifndef __GNUC__
#define __alignof__(x)              8
#endif
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_LIST_LINE_HEADER          _G_plineheadAliasesLookup;
/*********************************************************************************************************
** ��������: __deviceTreeMemoryPartGet
** ��������: ���豸��������ڴ��л�ȡһ��
** �䡡��  : ppvMem         ������ڴ��ַ����ı䣩
**           stSize         ������ڴ��С
**           stAlign        ����Ķ����С
** �䡡��  : ������ڴ��ַ
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __deviceTreeMemoryAlloc
** ��������: �豸���ڴ�����ӿ�
** �䡡��  : stSize         ������ڴ��С
**           stAlign        ����Ķ����С
** �䡡��  : ������ڴ��ַ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __deviceTreeMemoryAlloc (size_t  stSize, size_t  stAlign)
{
    return  (__SHEAP_ALLOC_ALIGN(stSize, stAlign));
}
/*********************************************************************************************************
** ��������: __deviceTreeMemGuardSet
** ��������: �����ڴ汣����־
** �䡡��  : pvMem        �ڴ��ַ
**           iSize        �ڴ��С
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __deviceTreeMemGuardSet (PVOID  pvMem, INT  iSize)
{
    *(UINT32 *)((addr_t)pvMem + iSize) = DEVTREE_MEM_GUARD;
}
/*********************************************************************************************************
** ��������: __deviceTreeMemGuardGet
** ��������: ��ȡ�ڴ汣����־
** �䡡��  : pvMem        �ڴ��ַ
**           iSize        �ڴ��С
** �䡡��  : �ڴ汣����־λ�õ�ǰֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __deviceTreeMemGuardGet (PVOID  pvMem, INT  iSize)
{
    return  (*(UINT32 *)((addr_t)pvMem + iSize));
}
/*********************************************************************************************************
** ��������: __deviceTreeNodePropertiesPopulate
** ��������: �����豸����������
** �䡡��  : pvDevTree       �豸������ַ
**           iOffset         �ڵ�ƫ��
**           ppvMem          �ڴ�ƫ��
**           pdtnDev         ��ǰ�����������ԵĽڵ�
**           pcNodeName      �ڵ�����
**           bIsSkip         �Ƿ�ֻ������������������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
         iCur  = fdt_next_property_offset(pvDevTree, iCur)) {           /*  ��ָ��ƫ�ƿ�ʼ�������Խڵ�  */

        puiVal = fdt_getprop_by_offset(pvDevTree,
                                       iCur,
                                       &pcName,
                                       &iSize);                         /*  ��ȡ����                    */
        if (!puiVal) {
            DEVTREE_ERR("Cannot locate property at 0x%x\r\n", iCur);
            continue;
        }

        if (!pcName) {
            DEVTREE_ERR("Cannot find property name at 0x%x\r\n", iCur);
            continue;
        }

        if (!lib_strcmp(pcName, "name")) {                              /*  ��������Ϊ "name"           */
            bIsHasName = LW_TRUE;
        }

        pdtpcurr = __deviceTreeMemoryPartGet(ppvMem,                    /*  Ϊ���Է����ڴ�              */
                                             sizeof(LW_DEVTREE_PROPERTY),
                                             __alignof__(LW_DEVTREE_PROPERTY));

        if (bIsSkip) {                                                  /*  ���ֻ��������ֱ����һ��    */
            continue;
        }

        if (!strcmp(pcName, "phandle") && !pdtnDev->DTN_uiHandle) {     /*  ��������Ϊ "phandle"        */
            pdtnDev->DTN_uiHandle = BE32_TO_CPU(puiVal);                /*  ��¼���ֵ                  */
        }

        pdtpcurr->DTP_pcName  = pcName;                                 /*  ��¼���Ե�����              */
        pdtpcurr->DTP_iLength = iSize;                                  /*  ��¼����ֵ�ĳ���            */
        pdtpcurr->DTP_pvValue = (UINT32 *)puiVal;                       /*  ��¼����ֵ������            */
       *ppdtpprev             = pdtpcurr;
        ppdtpprev             = &pdtpcurr->DTP_pdtpNext;
    }

    if (!bIsHasName) {                                                  /*  �����������ȫ·������      */
        pcTmp   = pcNodeName;
        pcStart = pcTmp;
        pcAddr  = LW_NULL;

        /*
         *  ���磺"/serial@e2900800" ����������
         *  ͨ������ѭ�����Ի�ȡ��ֵ "serial"
         */
        while (*pcTmp) {                                                /*  ����ַ�����                */
            if (*pcTmp == '@') {                                        /*  ��¼�ڵ��ַ                */
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
** ��������: __deviceTreeNodePopulate
** ��������: �����豸���ڵ�
** �䡡��  : pvDevTree       �豸������ַ
**           iOffset         �ڵ�ƫ��
**           ppvMem          ���ٵ��ڴ�ƫ��
**           pdtnFather      ���ڵ�
**           ppdtnCur        ���ڻ�ȡ��ǰ�ڵ�
**           bIsSkip         �Ƿ�ֻ������������������
** �䡡��  : LW_TRUE ��ʾ���ɳɹ���LW_FALSE ��ʾ����ʧ��
** ȫ�ֱ���:
** ����ģ��:
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

    pcPath = fdt_get_name(pvDevTree, iOffset, &iLen);                   /*  ��ȡ�ڵ�ȫ·����            */
    if (!pcPath) {
        *ppdtnCur = LW_NULL;
        return  (LW_FALSE);
    }

    iAllocLen = iLen + 1;                                               /*  �ڴ��С��Ҫ���� Full Name  */
    pdtnPtr   = __deviceTreeMemoryPartGet(ppvMem,                       /*  ��ָ���ڴ���ȡ��һ��        */
                                          sizeof(LW_DEVTREE_NODE) + iAllocLen,
                                          __alignof__(LW_DEVTREE_NODE));
    if (!bIsSkip) {
        pdtnPtr->DTN_pcFullName = (PCHAR)pdtnPtr + sizeof(LW_DEVTREE_NODE);
        lib_memcpy((PVOID)pdtnPtr->DTN_pcFullName, pcPath, iAllocLen);
                                                                        /*  ��¼�ڵ�ȫ·����            */
        if (pdtnFather != LW_NULL) {
            pdtnPtr->DTN_pdtnparent   = pdtnFather;                     /*  ��¼���ڵ�                  */
            pdtnPtr->DTN_pdtnsibling  = pdtnFather->DTN_pdtnchild;      /*  ��¼�ֵܽڵ�                */
            pdtnFather->DTN_pdtnchild = pdtnPtr;                        /*  ���¸��ڵ���ӽڵ�          */
        }
    }

    __deviceTreeNodePropertiesPopulate(pvDevTree,
                                       iOffset,
                                       ppvMem,
                                       pdtnPtr,
                                       pcPath,
                                       bIsSkip);                        /*  ������Խṹ                */
    if (!bIsSkip) {
        pdtnPtr->DTN_pcName = API_DeviceTreePropertyGet(pdtnPtr,
                                                        "name",
                                                        LW_NULL);       /*  �����Խṹ�л�ȡ����        */
        pdtnPtr->DTN_pcType = API_DeviceTreePropertyGet(pdtnPtr,
                                                        "device_type",
                                                        LW_NULL);       /*  �����Խṹ�л�ȡ����        */
        if (!pdtnPtr->DTN_pcName) {
            pdtnPtr->DTN_pcName = "<NULL>";
        }

        if (!pdtnPtr->DTN_pcType) {
            pdtnPtr->DTN_pcType = "<NULL>";
        }
    }

    *ppdtnCur = pdtnPtr;                                                /*  ��¼��ǰ�������Ľڵ�        */

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: __deviceTreeNodesReverse
** ��������: ���������豸���ڵ�
** �䡡��  : pdtnParent       �豸�����ڵ�
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __deviceTreeNodesReverse (PLW_DEVTREE_NODE  pdtnParent)
{
    PLW_DEVTREE_NODE  pdtnChild;
    PLW_DEVTREE_NODE  pdtnNext;

    pdtnChild = pdtnParent->DTN_pdtnchild;                              /*  ��õ�ǰ���ӽڵ�            */
    while (pdtnChild) {                                                 /*  �ݹ��ӽڵ���ӽڵ�          */
        __deviceTreeNodesReverse(pdtnChild);
        pdtnChild = pdtnChild->DTN_pdtnsibling;                         /*  ����ÿһ���ֵܽڵ�          */
    }

    pdtnChild = pdtnParent->DTN_pdtnchild;                              /*  ִ�е��ˣ�Ϊ���һ���ֵܽڵ�*/
    pdtnParent->DTN_pdtnchild = LW_NULL;
    while (pdtnChild) {
        pdtnNext = pdtnChild->DTN_pdtnsibling;                          /*  �ҵ���һ���ֵܽڵ�          */

        pdtnChild->DTN_pdtnsibling = pdtnParent->DTN_pdtnchild;
        pdtnParent->DTN_pdtnchild  = pdtnChild;
        pdtnChild                  = pdtnNext;
    }
}
/*********************************************************************************************************
** ��������: __deviceTreeAliasAdd
** ��������: ���������Խڵ����������Բ�������
** �䡡��  : pdtaprop       �������Խڵ�
**           pdtnDev        �豸���ڵ�
**           iId            id ֵ
**           pcStem         �豸����
**           iStemLen       �豸���Ƴ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __deviceTreeAliasScan
** ��������: �Ա������Խڵ����ɨ��
** �䡡��  : pfuncDtAlloc    �ڴ����뺯��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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

    _G_pdtnAliases = __deviceTreeFindNodeByPath("/aliases");            /*  �ҵ� aliases �ڵ�           */
    if (!_G_pdtnAliases) {
        return;
    }

    for (pdtpCur  = _G_pdtnAliases->DTN_pdtpproperties;
         pdtpCur != LW_NULL;
         pdtpCur  = pdtpCur->DTP_pdtpNext) {

        if (!strcmp(pdtpCur->DTP_pcName, "name")    ||
            !strcmp(pdtpCur->DTP_pcName, "phandle") ||
            !strcmp(pdtpCur->DTP_pcName, "linux,phandle")) {            /*  ��������Ҫ���ĵ�����        */
            continue;
        }

        pcStart = pdtpCur->DTP_pcName;
        pcEnd   = pcStart + lib_strlen(pcStart);

        pdtnDev = __deviceTreeFindNodeByPath(pdtpCur->DTP_pvValue);     /*  ͨ��ȫ·���������豸���ڵ�  */
        if (!pdtnDev) {
            continue;
        }

        while (lib_isdigit(*(pcEnd - 1)) && (pcEnd > pcStart)) {        /*  �ҵ�������ʼ��λ��          */
            pcEnd--;
        }

        iLen = pcEnd - pcStart;
        iId  = lib_atoi(pcEnd);
        if (iId < 0) {                                                  /*  ������ֽ���С�� 0          */
            continue;
        }

        pdtaprop = pfuncDtAlloc(sizeof(LW_DEVTREE_ALIAS_PROPERTY) + iLen + 1,
                                __alignof__(LW_DEVTREE_ALIAS_PROPERTY));
        if (!pdtaprop) {
            return;
        }

        lib_bzero(pdtaprop, sizeof(LW_DEVTREE_ALIAS_PROPERTY) + iLen + 1);

        __deviceTreeAliasAdd(pdtaprop, pdtnDev,
                             iId, pcStart, iLen);                       /*  �� Alias ��������           */
    }
}
/*********************************************************************************************************
** ��������: __deviceTreePhandleCachePopulate
** ��������: ���� Phandle Cache ��Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __deviceTreePhandleCachePopulate (VOID)
{
    PLW_DEVTREE_NODE   pdtnDev;
    UINT32             uiCacheEntries;
    UINT32             uiPhandles = 0;

    _LIST_EACH_OF_ALLNODES(pdtnDev) {                                   /*  ������㣬ͳ�� phandle ���� */
        if (pdtnDev->DTN_uiHandle) {
            uiPhandles++;
        }
    }

    if (!uiPhandles) {                                                  /*  ���û�� phandle������      */
        return;
    }

    uiCacheEntries        = roundup_pow_of_two(uiPhandles);             /*  �ҳ�С�� phandle �� 2^n ��  */
    _G_uiPhandleCacheMask = uiCacheEntries - 1;                         /*  ���� phandle ������         */

    _G_ppdtnPhandleCache = __SHEAP_ZALLOC(uiCacheEntries * sizeof(PLW_DEVTREE_NODE));
    if (!_G_ppdtnPhandleCache) {                                        /*  ���� phandle Cache �ڴ�     */
        return;
    }

    _LIST_EACH_OF_ALLNODES(pdtnDev) {
        if (pdtnDev->DTN_uiHandle) {                                    /*  ���� phandle ������� Cache */
            _G_ppdtnPhandleCache[pdtnDev->DTN_uiHandle & _G_uiPhandleCacheMask] = pdtnDev;
        }
    }
}
/*********************************************************************************************************
** ��������: __deviceTreeUnflattenNodes
** ��������: �����豸���ͽṹ����;
**           �� pvMem Ϊ��ʱ��bIsSkip Ϊ LW_TRUE����ʱֻ���ٱ�������������̣��ɿ��ٵõ�����С��
** �䡡��  : pvDevTree       �豸������ַ
**           pvMem           �豸�����ٵ��ڴ��ַ
**           pdtnFather      ���ڵ�
**           ppdtnCur        ���ڻ�ȡ��ǰ�ڵ�
** �䡡��  : ��ȷʱ���ص�ǰ���������豸���ڴ�ƫ�ƣ�
**           ����ʱ���� PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
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
    BOOL              bIsSkip = !pvMem;                                 /*  �Ƿ�ֻ���������������      */

    if (ppdtnCur) {                                                     /*  ���Ƚ���ǰ�ڵ���Ϊ NULL     */
        *ppdtnCur = LW_NULL;
    }

    if (pdtnFather) {                                                   /*  ����и��ڵ�                */
        iDepth        = 1;                                              /*  ��ʼ�����������Ϊ 1        */
        iInitialDepth = 1;
    } else {
        iDepth        = 0;
        iInitialDepth = 0;
    }

    pdtnRoot             = pdtnFather;                                  /*  ��ǰ�ĸ��ڵ�Ϊ ���ڵ�       */
    pdtnPointers[iDepth] = pdtnFather;

    for (iOffset  = 0;
         (iOffset >= 0) && (iDepth >= iInitialDepth);                   /*  ��û�б��������豸��        */
         iOffset  = fdt_next_node(pvDevTree, iOffset, &iDepth)) {

        if (iDepth >= FDT_MAX_DEPTH) {                                  /*  ����Ѿ�����������        */
            _ErrorHandle(ERROR_DEVTREE_DEPTH_ERROR);
            return  (PX_ERROR);
        }

        if (!API_DeviceTreeNodeIsOkayByOffset(pvDevTree, iOffset)) {    /*  ����ڵ㲻����              */
            continue;
        }

        if (!__deviceTreeNodePopulate(pvDevTree,
                                      iOffset,
                                      &pvMem,                           /*  �����л���� pvMem ��λ��   */
                                      pdtnPointers[iDepth],
                                      &pdtnPointers[iDepth + 1],
                                      bIsSkip)) {                       /*  ���ɿ��õĽڵ�              */
             return  ((addr_t)pvMem - (addr_t)pvBase);                  /*  ������Ҫ���ڴ��С          */
        }

        if (!bIsSkip &&                                                 /*  ��Ҫ��¼�ڵ�                */
            ppdtnCur &&
            (*ppdtnCur == LW_NULL)) {                                   /*  ��ǰ���ʵ��Ľڵ�Ϊ��ʱ��    */
           *ppdtnCur = pdtnPointers[iDepth + 1];                        /*  ��¼��ǰ���ʵ����Ľڵ�    */
        }

        if (!bIsSkip && !pdtnRoot) {                                    /*  ��Ҫ��¼�ڵ��Ҹ��ڵ�Ϊ��    */
            pdtnRoot = pdtnPointers[iDepth + 1];                        /*  ��¼���ڵ�                  */
        }
    }

    if ((iOffset < 0) && (iOffset != -FDT_ERR_NOTFOUND)) {              /*  ����豸����������          */
        _ErrorHandle(ERROR_DEVTREE_POPULATE_ERROR);
        return  (PX_ERROR);
    }

    if (!bIsSkip) {                                                     /*  ����豸����������          */
        __deviceTreeNodesReverse(pdtnRoot);                             /*  �����������Ľṹ            */
    }

    return  ((addr_t)pvMem - (addr_t)pvBase);                           /*  ������Ҫ���ڴ��С          */
}
/*********************************************************************************************************
** ��������: __deviceTreeUnflatten
** ��������: �����豸�����νṹչ���豸����
**           �ȿ��ٱ���һ���豸���ṹ����ȡ�豸����С���ٽ���ʵ�ʵ��豸������
** �䡡��  : pvDevtreeMem    �豸������ַ
**           pdtnFather      ���ڵ�
**           ppdtnCur        ���ڻ�ȡ��ǰ�ڵ�
**           pfuncDtAlloc    �ڴ����뺯��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
                                       pdtnFather,   LW_NULL);          /*  ��ȡ�豸����С              */
    if (iSize < 0) {
        return  (PX_ERROR);
    }

    iSize = ROUND_UP(iSize, 4);                                         /*  �Ҷ�����ڴ��С            */

    DEVTREE_MSG("Device tree is allocating %d bytes...\r\n", iSize);

    pvMem = pfuncDtAlloc(iSize + DEVTREE_MEM_GUARD_SIZE,
                         __alignof__(LW_DEVTREE_NODE));                 /*  �����豸�������ڴ�          */
    if (!pvMem) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    lib_bzero(pvMem, iSize);

    __deviceTreeMemGuardSet(pvMem, iSize);                              /*  �����ڴ澯���־            */

    __deviceTreeUnflattenNodes(pvDevtreeMem, pvMem,
                               pdtnFather, ppdtnCur);                   /*  ��ʽչ���豸��              */

    if (__deviceTreeMemGuardGet(pvMem, iSize) != DEVTREE_MEM_GUARD) {   /*  ���������Խ��              */
        _ErrorHandle(ERROR_DEVTREE_MEM_OVERLAP);
        return  (PX_ERROR);
    }

    DEVTREE_MSG("Device tree unflattened success!\r\n");

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeAliasIdGet
** ��������: �ӱ������������в��Ҷ�Ӧ�豸�� ID
** �䡡��  : pdtnDev       �豸���ڵ�
**           pcStem        �豸����
** �䡡��  : �豸�� ID
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_DeviceTreeHighLevelInit
** ��������: �豸���߼���ʼ������Ҫ��ϵͳ��ʼ����ɺ����
** �䡡��  : pvDevtreeMem        �豸����ַ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeHighLevelInit (PVOID  pvDevtreeMem)
{
    if (!pvDevtreeMem) {
        _ErrorHandle(ERROR_KERNEL_OBJECT_NULL);
        return  (PX_ERROR);
    }

    if (fdt_check_header(pvDevtreeMem)) {                               /*  �豸��ͷ��У��              */
        _ErrorHandle(ERROR_DEVTREE_MAGIC_ERROR);
        return  (PX_ERROR);
    }

    if (__deviceTreeUnflatten(pvDevtreeMem,
                              LW_NULL,
                              &_G_pdtnRoot,
                              __deviceTreeMemoryAlloc)) {               /*  ʵ�������豸�����νṹ      */
        return  (PX_ERROR);
    }

    __deviceTreeAliasScan(__deviceTreeMemoryAlloc);                     /*  ���� Alias �ڵ�             */

    __deviceTreePhandleCachePopulate();                                 /*  ���� Phandle Cache ��Ϣ     */

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
