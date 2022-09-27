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
** ��   ��   ��: devtreeIrq.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 06 �� 21 ��
**
** ��        ��: �豸���ӿ��ж���ؽӿ�ʵ��
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
** ��������: API_DeviceTreeIrqCountGet
** ��������: �豸���ڵ���ж���Դ����ȡ
** �䡡��  : pdtnDev          ָ�����豸���ڵ�
** �䡡��  : �ж���Դ����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
** ��������: API_DeviceTreeIrqFindParent
** ��������: �����豸���ڵ���жϸ��ڵ�
** �䡡��  : pdtnChild          ָ�����豸���ڵ�
** �䡡��  : �жϸ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEVTREE_NODE  API_DeviceTreeIrqFindParent (PLW_DEVTREE_NODE  pdtnChild)
{
    PLW_DEVTREE_NODE  pdtnDev;
    UINT32            uiParent;

    if (!pdtnChild) {                                                   /*  ����ǰ�ڵ�Ϊ�գ����ڵ�Ϊ��  */
        return  (LW_NULL);
    }

    do {
        if (API_DeviceTreePropertyU32Read(pdtnChild,
                                         "interrupt-parent",
                                          &uiParent)) {                 /*  ��ȡ���ڵ�� phandle        */
            pdtnDev = pdtnChild->DTN_pdtnparent;                        /*  ����ȡʧ�ܣ�ȡ��ǰ�ڵ㸸�ڵ�*/
        } else  {
            pdtnDev = API_DeviceTreeFindNodeByPhandle(uiParent);        /*  ���� phandle ���ҽڵ�       */
        }
        pdtnChild = pdtnDev;

    } while (pdtnDev &&                                                 /*  �ڵ��� interrupt-cells ���� */
             (API_DeviceTreePropertyGet(pdtnDev, "#interrupt-cells", LW_NULL) == LW_NULL));

    return  (pdtnDev);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeIrqRawParse
** ��������: �� Irq ���Խ��н���
** �䡡��  : puiAddr          Irq ���ԵĻ���ַ
**           pdtpaOutIrq      �洢 Irq ��Ϣ�Ĳ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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

    pdtnIpar = pdtpaOutIrq->DTPH_pdtnDev;                               /*  ��ȡ�жϸ��ڵ�              */

    do {
        if (!API_DeviceTreePropertyU32Read(pdtnIpar,
                                           "#interrupt-cells",
                                           &uiIntSize)) {               /*  ��ȡ "#interrupt-cells"     */
            break;
        }
        pdtnDev  = pdtnIpar;
        pdtnIpar = API_DeviceTreeIrqFindParent(pdtnIpar);
    } while (pdtnIpar);

    if (pdtnIpar == LW_NULL) {                                          /*  ����жϸ��ڵ�Ϊ��          */
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
                                            LW_NULL);                   /*  ���� "#address-cells" ����  */
        pdtnDev = pdtnOld->DTN_pdtnparent;
        pdtnOld = pdtnDev;
    } while (pdtnOld && (puiTmp == LW_NULL));

    pdtnOld    = LW_NULL;
    uiAddrSize = (puiTmp == LW_NULL) ? 2 : be32toh(*puiTmp);            /*  �õ���ַռ�õĴ�С          */

    DEVTREE_MSG(" -> addrsize=%d\r\n", uiAddrSize);

    if ((uiAddrSize + uiIntSize) > MAX_PHANDLE_ARGS) {                  /*  ������ֽ������ڲ�������    */
        iRet = -EFAULT;
        goto  __error_handle;
    }

    for (i = 0; i < uiAddrSize; i++) {                                  /*  ��ȡ��ַ�ֽ�                */
        uiInitialMatchArray[i] = puiAddr ? puiAddr[i] : 0;
    }

    for (i = 0; i < uiIntSize; i++) {                                   /*  ��ȡ�жϲ���                */
        uiInitialMatchArray[uiAddrSize + i] = htobe32(pdtpaOutIrq->DTPH_uiArgs[i]);
    }

    while (pdtnIpar != LW_NULL) {
        if (API_DeviceTreePropertyBoolRead(pdtnIpar,
                                           "interrupt-controller")) {   /*  ���� "interrupt-controller" */
            DEVTREE_MSG(" -> got it !\r\n");
            return  (ERROR_NONE);
        }

        if (uiAddrSize && !puiAddr) {
            DEVTREE_MSG(" -> no reg passed in when needed !\n");
            goto  __error_handle;
        }

        puiImap = API_DeviceTreePropertyGet(pdtnIpar,
                                            "interrupt-map",
                                            &iImapLen);                 /*  ���� "interrupt-map"        */
        if (puiImap == LW_NULL) {
            DEVTREE_MSG(" -> no map, getting parent\n");
            pdtnNewParent = API_DeviceTreeIrqFindParent(pdtnIpar);
            goto  __skip_level;
        }
        iImapLen /= sizeof(UINT32);

        puiImask = API_DeviceTreePropertyGet(pdtnIpar,
                                             "interrupt-map-mask",
                                             LW_NULL);                  /*  ���� "interrupt-map-mask"   */
        if (!puiImask) {
            puiImask = uiDummyImask;
        }

        iMatch = 0;                                                     /*  ���� "interrupt-map"        */
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
** ��������: API_DeviceTreeIrqOneParse
** ��������: ����һ���ж���Դ
** �䡡��  : pdtnDev       �豸���ڵ�
**           iIndex        �ж���Դ���
**           pdtpaOutIrq   ���������ж���Դ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
                                                 iIndex, pdtpaOutIrq);  /*  ���԰����ж���չ��Դ����    */
    if (!iRet) {
        return  API_DeviceTreeIrqRawParse(puiAddr, pdtpaOutIrq);
    }

    pnoTemp = API_DeviceTreeIrqFindParent(pdtnDev);                     /*  ���Ҷ�Ӧ���жϸ��ڵ�        */
    if (pnoTemp == LW_NULL) {
        return  (-EINVAL);
    }

    if (API_DeviceTreePropertyU32Read(pnoTemp,
                                      "#interrupt-cells",
                                      &uiIntSize)) {                    /*  ��ȡ interrupt-cells ��С   */
        iRet = -EINVAL;
        goto  __error_handle;
    }

    DEVTREE_MSG(" parent=%p, intsize=%d\r\n", pnoTemp, uiIntSize);

    pdtpaOutIrq->DTPH_pdtnDev    = pnoTemp;
    pdtpaOutIrq->DTPH_iArgsCount = uiIntSize;

    for (i = 0; i < uiIntSize; i++) {                                   /*  ��ȡ interrupts ����        */
        iRet = API_DeviceTreePropertyU32IndexRead(pdtnDev,
                                                  "interrupts",
                                                  (iIndex * uiIntSize) + i,
                                                  pdtpaOutIrq->DTPH_uiArgs + i);
        if (iRet) {
            goto  __error_handle;
        }
    }

    DEVTREE_MSG(" intspec=%d\r\n", *pdtpaOutIrq->DTPH_uiArgs);

    iRet = API_DeviceTreeIrqRawParse(puiAddr, pdtpaOutIrq);             /*  ��������ֵ                  */

__error_handle:
    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeIrqGet
** ��������: ��ȡ�豸���ڵ���ж���Դ��ת��Ϊ SylixOS �жϺŷ���
** �䡡��  : pdtnDev       �豸���ڵ�
**           iIndex        �ж���Դ���
**           pulVector     �洢��ȡ�� SylixOS �жϺ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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

    iRet = API_DeviceTreeIrqOneParse(pdtnDev, iIndex, &dtpaOutIrq);     /*  ����һ���ж���Դ            */
    if (iRet) {
        return  (iRet);
    }

    pirqctrldev = API_IrqCtrlDevtreeNodeMatch(dtpaOutIrq.DTPH_pdtnDev); /*  �ҵ���Ӧ���жϿ�����        */
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
** ��������: API_DeviceTreeIrqToResource
** ��������: ��ȡ�豸���ڵ���ж���Դ��ת��Ϊ��Դ����
** �䡡��  : pdtnDev       �豸���ڵ�
**           iIndex        �ж���Դ���
**           pdevresource  ��Դ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeIrqToResource (PLW_DEVTREE_NODE  pdtnDev,
                                  INT               iIndex,
                                  PLW_DEV_RESOURCE  pdevresource)
{
    ULONG  ulVector;
    INT    iRet;

    iRet = API_DeviceTreeIrqGet(pdtnDev, iIndex, &ulVector);            /*  ת���� SylixOS ���жϺ�     */
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    if (pdevresource && ulVector) {                                     /*  ����Դָ����Чʱ����ת��    */
        CPCHAR  pcName = LW_NULL;

        lib_bzero(pdevresource, sizeof(LW_DEV_RESOURCE));

        API_DeviceTreePropertyStringIndexRead(pdtnDev,
                                              "interrupt-names",
                                              iIndex,
                                              &pcName);                 /*  ��ȡ�ж���������            */

        pdevresource->irq.DEVRES_ulIrq = ulVector;                      /*  ��¼�жϺ�                  */
        pdevresource->DEVRES_pcName    = pcName ?                       /*  ��¼�ж���                  */
                                         pcName : __deviceTreeNodeFullName(pdtnDev);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeIrqToResouceTable
** ��������: ��ȡ�豸���ڵ���ж���Դ��ת��Ϊ��Դ��
** �䡡��  : pdtnDev       �豸���ڵ�
**           pdevresource  ��Դ��ָ��
**           iNrIrqs       �ж���Դ����
** �䡡��  : �Ѿ�ת�����ж���Դ����
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
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
