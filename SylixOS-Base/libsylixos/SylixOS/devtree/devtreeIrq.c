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
** 文   件   名: devtreeIrq.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 06 月 21 日
**
** 描        述: 设备树接口中断相关接口实现
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
** 函数名称: API_DeviceTreeIrqCountGet
** 功能描述: 设备树节点的中断资源数获取
** 输　入  : pdtnDev          指定的设备树节点
** 输　出  : 中断资源数量
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqCountGet (PLW_DEVTREE_NODE  pdtnDev)
{
    LW_DEVTREE_PHANDLE_ARGS  dtpaIrq;
    INT                      iNr = 0;

    while (API_DeviceTreeIrqOneParse(pdtnDev, iNr, &dtpaIrq) == ERROR_NONE) {
        iNr++;
    }

    return  (iNr);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIrqFindParent
** 功能描述: 查找设备树节点的中断父节点
** 输　入  : pdtnChild          指定的设备树节点
** 输　出  : 中断父节点
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeIrqFindParent (PLW_DEVTREE_NODE  pdtnChild)
{
    PLW_DEVTREE_NODE  pdtnDev;
    UINT32            uiParent;

    if (!pdtnChild) {                                                   /*  若当前节点为空，父节点为空  */
        return  (LW_NULL);
    }

    do {
        if (API_DeviceTreePropertyU32Read(pdtnChild,
                                         "interrupt-parent",
                                          &uiParent)) {                 /*  读取父节点的 phandle        */
            pdtnDev = pdtnChild->DTN_pdtnparent;                        /*  若读取失败，取当前节点父节点*/
        } else  {
            pdtnDev = API_DeviceTreeFindNodeByPhandle(uiParent);        /*  根据 phandle 查找节点       */
        }
        pdtnChild = pdtnDev;

    } while (pdtnDev &&                                                 /*  节点需 interrupt-cells 属性 */
             (API_DeviceTreePropertyGet(pdtnDev, "#interrupt-cells", LW_NULL) == LW_NULL));

    return  (pdtnDev);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIrqRawParse
** 功能描述: 对 Irq 属性进行解析
** 输　入  : puiAddr          Irq 属性的基地址
**           pdtpaOutIrq      存储 Irq 信息的参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqRawParse (const  UINT32  *puiAddr, PLW_DEVTREE_PHANDLE_ARGS  pdtpaOutIrq)
{
    static UINT32            uiDummyImask[MAX_PHANDLE_ARGS];
    static BOOL              bIsInited       = LW_FALSE;
           PLW_DEVTREE_NODE  pdtnIpar;
           PLW_DEVTREE_NODE  pdtnDev;
           PLW_DEVTREE_NODE  pdtnOld         = LW_NULL;
           PLW_DEVTREE_NODE  pdtnNewParent   = LW_NULL;

           UINT32            uiInitialMatchArray[MAX_PHANDLE_ARGS];
     const UINT32           *puiMatchArray   = uiInitialMatchArray;
     const UINT32           *puiTmp;
     const UINT32           *puiImap;
     const UINT32           *puiImask;

           UINT32            uiIntSize       = 1;
           UINT32            uiAddrSize;
           UINT32            uiNewIntSize    = 0;
           UINT32            uiNewAddrSize   = 0;
           INT               iImapLen;
           INT               iMatch;
           INT               i;
           INT               iRet            = -EINVAL;

    if (!pdtpaOutIrq) {
        _ErrorHandle(EINVAL);
        return  (-EINVAL);
    }

    if (!bIsInited) {
        bIsInited = LW_TRUE;
        lib_memset(uiDummyImask, 0xff, sizeof(uiDummyImask));
    }

    pdtnIpar = pdtpaOutIrq->DTPH_pdtnDev;                               /*  获取中断父节点              */

    do {
        if (!API_DeviceTreePropertyU32Read(pdtnIpar,
                                           "#interrupt-cells",
                                           &uiIntSize)) {               /*  读取 "#interrupt-cells"     */
            break;
        }
        pdtnDev  = pdtnIpar;
        pdtnIpar = API_DeviceTreeIrqFindParent(pdtnIpar);
    } while (pdtnIpar);

    if (pdtnIpar == LW_NULL) {                                          /*  如果中断父节点为空          */
        DEVTREE_MSG(" -> no parent found !\r\n");
        goto  __error_handle;
    }

    DEVTREE_MSG("of_irq_parse_raw: ipar=%p, size=%d\r\n", pdtnIpar, uiIntSize);

    if (pdtpaOutIrq->DTPH_iArgsCount != uiIntSize) {
        goto  __error_handle;
    }

    pdtnOld = pdtnIpar;
    do {
        puiTmp  = API_DeviceTreePropertyGet(pdtnOld,
                                            "#address-cells",
                                            LW_NULL);                   /*  查找 "#address-cells" 属性  */
        pdtnDev = pdtnOld->DTN_pdtnparent;
        pdtnOld = pdtnDev;
    } while (pdtnOld && (puiTmp == LW_NULL));

    pdtnOld    = LW_NULL;
    uiAddrSize = (puiTmp == LW_NULL) ? 2 : be32toh(*puiTmp);            /*  得到地址占用的大小          */

    DEVTREE_MSG(" -> addrsize=%d\r\n", uiAddrSize);

    if ((uiAddrSize + uiIntSize) > MAX_PHANDLE_ARGS) {                  /*  如果总字节数大于参数长度    */
        iRet = -EFAULT;
        goto  __error_handle;
    }

    for (i = 0; i < uiAddrSize; i++) {                                  /*  读取地址字节                */
        uiInitialMatchArray[i] = puiAddr ? puiAddr[i] : 0;
    }

    for (i = 0; i < uiIntSize; i++) {                                   /*  读取中断参数                */
        uiInitialMatchArray[uiAddrSize + i] = htobe32(pdtpaOutIrq->DTPH_uiArgs[i]);
    }

    while (pdtnIpar != LW_NULL) {
        if (API_DeviceTreePropertyBoolRead(pdtnIpar,
                                           "interrupt-controller")) {   /*  查找 "interrupt-controller" */
            DEVTREE_MSG(" -> got it !\r\n");
            return  (ERROR_NONE);
        }

        if (uiAddrSize && !puiAddr) {
            DEVTREE_MSG(" -> no reg passed in when needed !\n");
            goto  __error_handle;
        }

        puiImap = API_DeviceTreePropertyGet(pdtnIpar,
                                            "interrupt-map",
                                            &iImapLen);                 /*  查找 "interrupt-map"        */
        if (puiImap == LW_NULL) {
            DEVTREE_MSG(" -> no map, getting parent\n");
            pdtnNewParent = API_DeviceTreeIrqFindParent(pdtnIpar);
            goto  __skip_level;
        }
        iImapLen /= sizeof(UINT32);

        puiImask = API_DeviceTreePropertyGet(pdtnIpar,
                                             "interrupt-map-mask",
                                             LW_NULL);                  /*  查找 "interrupt-map-mask"   */
        if (!puiImask) {
            puiImask = uiDummyImask;
        }

        iMatch = 0;                                                     /*  解析 "interrupt-map"        */
        while (iImapLen > (uiAddrSize + uiIntSize + 1) && !iMatch) {
            iMatch = 1;
            for (i = 0;
                 i < (uiAddrSize + uiIntSize);
                 i++, iImapLen--) {
                iMatch &= !((puiMatchArray[i] ^ *puiImap++) & puiImask[i]);
            }

            DEVTREE_MSG(" -> match=%d (imaplen=%d)\n", iMatch, iImapLen);

            pdtnNewParent = API_DeviceTreeFindNodeByPhandle(BE32_TO_CPU(puiImap));
            puiImap++;
            --iImapLen;

            if (pdtnNewParent == LW_NULL) {
                DEVTREE_MSG(" -> imap parent not found !\n");
                goto  __error_handle;
            }

            if (!API_DeviceTreeNodeIsOkay(pdtnNewParent)) {
                iMatch = 0;
            }

            if (API_DeviceTreePropertyU32Read(pdtnNewParent,
                                              "#interrupt-cells",
                                              &uiNewIntSize)) {
                DEVTREE_MSG(" -> parent lacks #interrupt-cells!\n");
                goto  __error_handle;
            }

            if (API_DeviceTreePropertyU32Read(pdtnNewParent,
                                              "#address-cells",
                                              &uiNewAddrSize)) {
                uiNewAddrSize = 0;
            }

            DEVTREE_MSG(" -> newintsize=%d, newaddrsize=%d\n",
                    uiNewIntSize, uiNewAddrSize);

            if (((uiNewAddrSize + uiNewIntSize) > MAX_PHANDLE_ARGS) ||
                (iImapLen < (uiNewAddrSize + uiNewIntSize))) {
                iRet = -EFAULT;
                goto  __error_handle;
            }

            puiImap  += uiNewAddrSize + uiNewIntSize;
            iImapLen -= uiNewAddrSize + uiNewIntSize;

            DEVTREE_MSG(" -> imaplen=%d\n", iImapLen);
        }

        if (!iMatch) {
            goto  __error_handle;
        }

        puiMatchArray = puiImap - uiNewAddrSize - uiNewIntSize;
        for (i = 0; i < uiNewIntSize; i++) {
            pdtpaOutIrq->DTPH_uiArgs[i] = BE32_TO_CPU(puiImap - uiNewIntSize + i);
        }
        uiIntSize                    = uiNewIntSize;
        pdtpaOutIrq->DTPH_iArgsCount = uiNewIntSize;
        uiAddrSize                   = uiNewAddrSize;

__skip_level:
        pdtpaOutIrq->DTPH_pdtnDev = pdtnNewParent;
        DEVTREE_MSG(" -> new parent: %p\n", pdtnNewParent);
        pdtnIpar   = pdtnNewParent;
        pdtnNewParent = LW_NULL;
    }

    iRet = -ENOENT;

__error_handle:
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIrqOneParse
** 功能描述: 解析一条中断资源
** 输　入  : pdtnDev       设备树节点
**           iIndex        中断资源序号
**           pdtpaOutIrq   解析出的中断资源参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqOneParse (PLW_DEVTREE_NODE          pdtnDev,
                                INT                       iIndex,
                                PLW_DEVTREE_PHANDLE_ARGS  pdtpaOutIrq)
{
    PLW_DEVTREE_NODE  pnoTemp;
    const   UINT32   *puiAddr;
    UINT32            uiIntSize;
    INT               i;
    INT               iRet;

    DEVTREE_MSG("of_irq_parse_one: dev=%p, index=%d\r\n", pdtnDev, iIndex);

    puiAddr = API_DeviceTreePropertyGet(pdtnDev, "reg", LW_NULL);

    iRet    = API_DeviceTreePhandleParseWithArgs(pdtnDev,
                                                 "interrupts-extended",
                                                 "#interrupt-cells",
                                                 iIndex, pdtpaOutIrq);  /*  尝试按照中断扩展资源解析    */
    if (!iRet) {
        return  API_DeviceTreeIrqRawParse(puiAddr, pdtpaOutIrq);
    }

    pnoTemp = API_DeviceTreeIrqFindParent(pdtnDev);                     /*  查找对应的中断父节点        */
    if (pnoTemp == LW_NULL) {
        return  (-EINVAL);
    }

    if (API_DeviceTreePropertyU32Read(pnoTemp,
                                      "#interrupt-cells",
                                      &uiIntSize)) {                    /*  获取 interrupt-cells 大小   */
        iRet = -EINVAL;
        goto  __error_handle;
    }

    DEVTREE_MSG(" parent=%p, intsize=%d\r\n", pnoTemp, uiIntSize);

    pdtpaOutIrq->DTPH_pdtnDev    = pnoTemp;
    pdtpaOutIrq->DTPH_iArgsCount = uiIntSize;

    for (i = 0; i < uiIntSize; i++) {                                   /*  读取 interrupts 属性        */
        iRet = API_DeviceTreePropertyU32IndexRead(pdtnDev,
                                                  "interrupts",
                                                  (iIndex * uiIntSize) + i,
                                                  pdtpaOutIrq->DTPH_uiArgs + i);
        if (iRet) {
            goto  __error_handle;
        }
    }

    DEVTREE_MSG(" intspec=%d\r\n", *pdtpaOutIrq->DTPH_uiArgs);

    iRet = API_DeviceTreeIrqRawParse(puiAddr, pdtpaOutIrq);             /*  解析属性值                  */

__error_handle:
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIrqGet
** 功能描述: 获取设备树节点的中断资源，转换为 SylixOS 中断号返回
** 输　入  : pdtnDev       设备树节点
**           iIndex        中断资源序号
**           pulVector     存储获取的 SylixOS 中断号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqGet (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex, ULONG  *pulVector)
{
    LW_DEVTREE_PHANDLE_ARGS  dtpaOutIrq;
    PLW_IRQCTRL_DEV          pirqctrldev;
    ULONG                    ulHwIrq;
    ULONG                    ulVector;
    UINT                     uiType;
    INT                      iRet;

    if (!pulVector) {
        return  (PX_ERROR);
    }

    iRet = API_DeviceTreeIrqOneParse(pdtnDev, iIndex, &dtpaOutIrq);     /*  解析一条中断资源            */
    if (iRet) {
        return  (iRet);
    }

    pirqctrldev = API_IrqCtrlDevtreeNodeMatch(dtpaOutIrq.DTPH_pdtnDev); /*  找到对应的中断控制器        */
    if (!pirqctrldev) {
        return  (PX_ERROR);
    }

    iRet = API_IrqCtrlDevtreeTrans(pirqctrldev, &dtpaOutIrq, &ulHwIrq, &uiType);
    if (iRet) {
        return  (iRet);
    }

    iRet = API_IrqCtrlFindVectorByHwIrq(pirqctrldev, ulHwIrq, &ulVector);
    if (iRet != PX_ERROR) {
        *pulVector = ulVector;

    } else {
        iRet = API_IrqCtrlHwIrqMapToVector(pirqctrldev, ulHwIrq, &ulVector);
        if (iRet != PX_ERROR) {
            *pulVector = ulVector;
        }
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIrqToResource
** 功能描述: 获取设备树节点的中断资源，转换为资源表项
** 输　入  : pdtnDev       设备树节点
**           iIndex        中断资源序号
**           pdevresource  资源表指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqToResource (PLW_DEVTREE_NODE  pdtnDev,
                                  INT               iIndex,
                                  PLW_DEV_RESOURCE  pdevresource)
{
    ULONG  ulVector;
    INT    iRet;

    iRet = API_DeviceTreeIrqGet(pdtnDev, iIndex, &ulVector);            /*  转换成 SylixOS 的中断号     */
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    if (pdevresource && ulVector) {                                     /*  当资源指针有效时进行转换    */
        CPCHAR  pcName = LW_NULL;

        lib_bzero(pdevresource, sizeof(LW_DEV_RESOURCE));

        API_DeviceTreePropertyStringIndexRead(pdtnDev,
                                              "interrupt-names",
                                              iIndex,
                                              &pcName);                 /*  获取中断名称属性            */

        pdevresource->irq.DEVRES_ulIrq = ulVector;                      /*  记录中断号                  */
        pdevresource->DEVRES_pcName    = pcName ?                       /*  记录中断名                  */
                                         pcName : __deviceTreeNodeFullName(pdtnDev);
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeIrqToResouceTable
** 功能描述: 获取设备树节点的中断资源，转换为资源表
** 输　入  : pdtnDev       设备树节点
**           pdevresource  资源表指针
**           iNrIrqs       中断资源数量
** 输　出  : 已经转换的中断资源数量
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqToResouceTable (PLW_DEVTREE_NODE  pdtnDev,
                                      PLW_DEV_RESOURCE  pdevresource,
                                      INT               iNrIrqs)
{
    INT  i;

    for (i = 0; i < iNrIrqs; i++) {
        if (API_DeviceTreeIrqToResource(pdtnDev, i, pdevresource) < 0) {
            break;
        }
    }

    return  (i);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
