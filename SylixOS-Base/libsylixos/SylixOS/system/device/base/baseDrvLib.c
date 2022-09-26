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
** ��   ��   ��: baseDrvLib.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 10 �� 28 ��
**
** ��        ��: ����������ܿ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "baseDrvLib.h"
/*********************************************************************************************************
  ��̬��������
*********************************************************************************************************/
static INT              __busAddDevice(PLW_DEV_INSTANCE  pdevinstance);
static VOID             __busDelDevice(PLW_DEV_INSTANCE  pdevinstance);
static INT              __busAddDriver(PLW_DRV_INSTANCE  pdrvinstance);
static VOID             __busDelDriver(PLW_DRV_INSTANCE  pdrvinstance);
static VOID             __devInit(PLW_DEV_INSTANCE  pdevinstance);
static INT              __devAdd(PLW_DEV_INSTANCE   pdevinstance);
static VOID             __devDel(PLW_DEV_INSTANCE   pdevinstance);
static VOID             __devReleaseDrv(PLW_DEV_INSTANCE  pdevinstance);
static INT              __drvAttach(PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance);
static VOID             __drvDetach(PLW_DRV_INSTANCE  pdrvinstance);
static PLW_DRV_INSTANCE __drvFind(CPCHAR  pcName, PLW_BUS_TYPE  pbustype);
static INT              __drvMatchDev(PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_INLINE VOID __baseSemInit (LW_OBJECT_HANDLE  *pulSem)
{
    if (*pulSem == LW_OBJECT_HANDLE_INVALID) {
        *pulSem = API_SemaphoreMCreate("dt_base_lock", LW_PRIO_DEF_CEILING,
                                       LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                       LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                       LW_NULL);
    }
}
/*********************************************************************************************************
  ���ֲ������ĺ궨��
*********************************************************************************************************/
#define __BUS_HD_LOCK_INIT(pbustype)            __baseSemInit(&pbustype->BUS_hBusLock)
#define __BUS_HD_LOCK_DEL(pbustype)             API_SemaphoreMDelete(&pbustype->BUS_hBusLock)
#define __BUS_HD_UNLOCK(pbustype)               API_SemaphoreMPost(pbustype->BUS_hBusLock)
#define __BUS_HD_LOCK(pbustype)                 API_SemaphoreMPend(pbustype->BUS_hBusLock,       \
                                                                   LW_OPTION_WAIT_INFINITE)

#define __DEV_HD_LOCK_INIT(pdevinstance)        __baseSemInit(&pdevinstance->DEVHD_hDevLock);
#define __DEV_HD_LOCK_DEL(pbustype)             API_SemaphoreMDelete(&pdevinstance->DEVHD_hDevLock)
#define __DEV_HD_UNLOCK(pdevinstance)           API_SemaphoreMPost(pdevinstance->DEVHD_hDevLock)
#define __DEV_HD_LOCK(pdevinstance)             API_SemaphoreMPend(pdevinstance->DEVHD_hDevLock, \
                                                                   LW_OPTION_WAIT_INFINITE)

#define __BUS_DEV_LIST_LOCK_INIT(pbustype)      __baseSemInit(&pbustype->BUS_hDevListLock)
#define __BUS_DEV_LIST_LOCK_DEL(pbustype)       API_SemaphoreMDelete(&pbustype->BUS_hDevListLock)
#define __BUS_DEV_LIST_UNLOCK(pbustype)         API_SemaphoreMPost(pbustype->BUS_hDevListLock)
#define __BUS_DEV_LIST_LOCK(pbustype)           API_SemaphoreMPend(pbustype->BUS_hDevListLock,   \
                                                                  LW_OPTION_WAIT_INFINITE)

#define __BUS_DRV_LIST_LOCK_INIT(pbustype)      __baseSemInit(&pbustype->BUS_hDrvListLock)
#define __BUS_DRV_LIST_LOCK_DEL(pbustype)       API_SemaphoreMDelete(&pbustype->BUS_hDrvListLock)
#define __BUS_DRV_LIST_UNLOCK(pbustype)         API_SemaphoreMPost(pbustype->BUS_hDrvListLock)
#define __BUS_DRV_LIST_LOCK(pbustype)           API_SemaphoreMPend(pbustype->BUS_hDrvListLock,   \
                                                                   LW_OPTION_WAIT_INFINITE)

#define __DRV_DEV_LIST_LOCK_INIT(pdrvinstance)  __baseSemInit(&pdrvinstance->DRVHD_hDevListLock);
#define __DRV_DEV_LIST_LOCK_DEL(pdrvinstance)   API_SemaphoreMDelete(&pdrvinstance->DRVHD_hDevListLock);
#define __DRV_DEV_LIST_UNLOCK(pdrvinstance)     API_SemaphoreMPost(pdrvinstance->DRVHD_hDevListLock)
#define __DRV_DEV_LIST_LOCK(pdrvinstance)       API_SemaphoreMPend(pdrvinstance->DRVHD_hDevListLock, \
                                                                   LW_OPTION_WAIT_INFINITE)
/*********************************************************************************************************
** ��������: __busAddDevice
** ��������: ������������豸
** �䡡��  : pdevinstance      �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __busAddDevice (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DRV_INSTANCE  pdrvinstance;
    PLW_LIST_LINE     plineNode;
    PLW_BUS_TYPE      pbustype = pdevinstance->DEVHD_pbustype;
    INT               iRet;

    __BUS_DEV_LIST_LOCK(pbustype);
    _List_Line_Add_Ahead(&pdevinstance->DEVHD_lineBus,
                         &pbustype->BUS_plineDevList);                  /*  ����豸����������          */
    __BUS_DEV_LIST_UNLOCK(pbustype);

    if ((pbustype->BUS_uiFlag & BUS_AUTO_PROBE) &&                      /*  ����ʹ�����Զ� probe        */
        (LW_NULL == pdevinstance->DEVHD_pdrvinstance)) {

        __DEV_HD_LOCK(pdevinstance);
        __BUS_DRV_LIST_LOCK(pbustype);

        for (plineNode  = pbustype->BUS_plineDrvList;
             plineNode != LW_NULL;
             plineNode  = _list_line_get_next(plineNode)) {             /*  ���������ϵ�����            */

            pdrvinstance = _LIST_ENTRY(plineNode, LW_DRV_INSTANCE, DRVHD_lineBus);
            iRet = __drvMatchDev(pdevinstance, pdrvinstance);           /*  �豸����������ƥ��          */
            if (!iRet) {
                __drvAttach(pdevinstance, pdrvinstance);                /*  ƥ��ɹ���������            */
                break;
            }
        }

        __BUS_DRV_LIST_UNLOCK(pbustype);
        __DEV_HD_UNLOCK(pdevinstance);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __busDelDevice
** ��������: ��������ɾ���豸
** �䡡��  : pdevinstance      �豸ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __busDelDevice (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_BUS_TYPE  pbustype = pdevinstance->DEVHD_pbustype;

    __BUS_DEV_LIST_LOCK(pbustype);
    _List_Line_Del(&pdevinstance->DEVHD_lineBus, &pbustype->BUS_plineDevList);
    __BUS_DEV_LIST_UNLOCK(pbustype);

    __devReleaseDrv(pdevinstance);
}
/*********************************************************************************************************
** ��������: __busAddDriver
** ��������: �������������
** �䡡��  : pdrvinstance      ����ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __busAddDriver (PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DEV_INSTANCE  pdevinstance;
    PLW_LIST_LINE     plineNode;
    PLW_BUS_TYPE      pbustype = pdrvinstance->DRVHD_pbustype;
    INT               iRet;

    __DRV_DEV_LIST_LOCK_INIT(pdrvinstance);                             /*  ��ʼ���������豸���������  */
    pdrvinstance->DRVHD_plineDevList = LW_NULL;                         /*  ��ʼ���������豸����        */

    __BUS_DRV_LIST_LOCK(pbustype);
    _List_Line_Add_Ahead(&pdrvinstance->DRVHD_lineBus,
                         &pbustype->BUS_plineDrvList);                  /*  ��������ӵ���������������  */
    __BUS_DRV_LIST_UNLOCK(pbustype);

    if (pbustype->BUS_uiFlag & BUS_AUTO_PROBE) {                        /*  �����������Զ� probe        */

        __BUS_DEV_LIST_LOCK(pbustype);
        for (plineNode  = pbustype->BUS_plineDevList;
             plineNode != LW_NULL;
             plineNode  = _list_line_get_next(plineNode)) {             /*  ���������ϵ��豸            */

            pdevinstance = _LIST_ENTRY(plineNode, LW_DEV_INSTANCE, DEVHD_lineBus);
            
            __DEV_HD_LOCK(pdevinstance);
            iRet = __drvMatchDev(pdevinstance, pdrvinstance);           /*  ����豸�������Ƿ�ƥ��      */
            if (!iRet) {
                __drvAttach(pdevinstance, pdrvinstance);                /*  �豸������ƥ��������й���  */
            }
            __DEV_HD_UNLOCK(pdevinstance);
        }
        __BUS_DEV_LIST_UNLOCK(pbustype);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __busDelDriver
** ��������: ��������ɾ������
** �䡡��  : pdrvinstance      ����ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __busDelDriver (PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_BUS_TYPE  pbustype = pdrvinstance->DRVHD_pbustype;

    __BUS_DRV_LIST_LOCK(pbustype);
    _List_Line_Del(&pdrvinstance->DRVHD_lineBus, &pbustype->BUS_plineDrvList);
    __BUS_DRV_LIST_UNLOCK(pbustype);

    __drvDetach(pdrvinstance);
}
/*********************************************************************************************************
** ��������: __devInit
** ��������: �豸��ʼ��
** �䡡��  : pdevinstance      �豸ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __devInit (PLW_DEV_INSTANCE  pdevinstance)
{
    __DEV_HD_LOCK_INIT(pdevinstance);
}
/*********************************************************************************************************
** ��������: __devReleaseDrv
** ��������: ����豸�������İ�
** �䡡��  : pdevinstance      �豸ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __devReleaseDrv (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DRV_INSTANCE  pdrvinstance = pdevinstance->DEVHD_pdrvinstance;

    if (pdrvinstance) {
        __DEV_HD_LOCK(pdevinstance);

        /*
         *  ���� remove �ӿ�
         */
        if (pdevinstance->DEVHD_pbustype->BUS_pfuncRemove) {
            pdevinstance->DEVHD_pbustype->BUS_pfuncRemove(pdevinstance);

        } else if (pdrvinstance->DRVHD_pfuncRemove) {
            pdrvinstance->DRVHD_pfuncRemove(pdevinstance);
        }

        __DRV_DEV_LIST_LOCK(pdrvinstance);
        _List_Line_Del(&pdevinstance->DEVHD_lineDrv,
                       &pdrvinstance->DRVHD_plineDevList);              /* ������������ɾ���豸         */
        __DRV_DEV_LIST_UNLOCK(pdrvinstance);

        /*
         *  �������
         */
        pdevinstance->DEVHD_pdrvinstance = LW_NULL;
        API_DevSetDrvdata(pdevinstance, LW_NULL);

        __DEV_HD_UNLOCK(pdevinstance);
    }
}
/*********************************************************************************************************
** ��������: __devDel
** ��������: ɾ���豸
** �䡡��  : pdevinstance      �豸ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __devDel (PLW_DEV_INSTANCE  pdevinstance)
{
    __busDelDevice(pdevinstance);
}
/*********************************************************************************************************
** ��������: __devAdd
** ��������: ����豸
** �䡡��  : pdevinstance      �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __devAdd (PLW_DEV_INSTANCE  pdevinstance)
{
    return  (__busAddDevice(pdevinstance));
}
/*********************************************************************************************************
** ��������: __drvFind
** ��������: �������ϲ�������
** �䡡��  : pcName      ��������
**           pbustype    ����
** �䡡��  : �ɹ���������ʵ��ָ��
**           ʧ�ܷ��� LW_NULL
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static PLW_DRV_INSTANCE  __drvFind (CPCHAR  pcName, PLW_BUS_TYPE  pbustype)
{
    PLW_DRV_INSTANCE  pdrvinstance;
    PLW_LIST_LINE     plineNode;

    __BUS_DRV_LIST_LOCK(pbustype);
    for (plineNode  = pbustype->BUS_plineDrvList;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode),
         pdrvinstance = LW_NULL) {                                      /*  ���������ϵ���������        */

        pdrvinstance = _LIST_ENTRY(plineNode, LW_DRV_INSTANCE, DRVHD_lineBus);
        if (!lib_strcmp(pdrvinstance->DRVHD_pcName, pcName)) {          /*  ͨ�����ֽ��бȽ�ƥ��        */
            break;
        }
    }
    __BUS_DRV_LIST_UNLOCK(pbustype);

    if (plineNode == LW_NULL) {                                         /*  ���ûƥ�䵽                */
        pdrvinstance = LW_NULL;
    }

    return  (pdrvinstance);
}
/*********************************************************************************************************
** ��������: __drvMatchDev
** ��������: ����������豸�Ƿ�ƥ��
** �䡡��  : pdevinstance      �豸ʵ��ָ��
**           pdrvinstance      ����ʵ��ָ��
** �䡡��  : ƥ�䷵�� 0, ��ƥ�䷵������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __drvMatchDev (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    INT   iRet = 0;

    /*
     *  ����豸�������Ƿ�ƥ�䣬bus ��ƥ�亯����������Ĭ��ƥ��
     */
    if (pdrvinstance->DRVHD_pbustype->BUS_pfuncMatch) {
        iRet = pdrvinstance->DRVHD_pbustype->BUS_pfuncMatch(pdevinstance, pdrvinstance);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** ��������: __drvAttach
** ��������: ���豸��������
** �䡡��  : pdevinstance    �豸ʵ��ָ��
**           pdrvinstance    ����ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT __drvAttach (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    UINT    uiFlag;
    INT     iRet;

    /*
     *  ���豸������
     */
    __DRV_DEV_LIST_LOCK(pdrvinstance);
    _List_Line_Add_Ahead(&pdevinstance->DEVHD_lineDrv, &pdrvinstance->DRVHD_plineDevList);
    __DRV_DEV_LIST_UNLOCK(pdrvinstance);
    pdevinstance->DEVHD_pdrvinstance = pdrvinstance;

    iRet = API_PinBind(pdevinstance);                                   /*  �����豸���ŵİ󶨴���      */
    if (iRet) {
        goto  __error_handle;
    }

    uiFlag = pdevinstance->DEVHD_pbustype->BUS_uiFlag;
    if (pdevinstance->DEVHD_pbustype->BUS_pfuncProbe) {                 /*  ���ߵ� probe ������������� */
        iRet = pdevinstance->DEVHD_pbustype->BUS_pfuncProbe(pdevinstance);
    } else {
        uiFlag |= BUS_FORCE_DRV_PROBE;
    }

    if ((uiFlag & BUS_FORCE_DRV_PROBE) && (pdrvinstance->DRVHD_pfuncProbe)) {
        iRet = pdrvinstance->DRVHD_pfuncProbe(pdevinstance);            /*  ���� probe �������������   */
    }

    if (iRet) {
        goto  __error_handle;
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "Bind driver %s and device %s\r\n",
                 pdrvinstance->DRVHD_pcName, pdevinstance->DEVHD_pcName);
    
    return  (ERROR_NONE);

__error_handle:
    __DRV_DEV_LIST_LOCK(pdrvinstance);
    _List_Line_Del(&pdevinstance->DEVHD_lineDrv,
                   &pdrvinstance->DRVHD_plineDevList);                  /*  ����ʧ��������          */
    __DRV_DEV_LIST_UNLOCK(pdrvinstance);
    pdevinstance->DEVHD_pdrvinstance = NULL;
    
    _DebugFormat(__ERRORMESSAGE_LEVEL,"Failed to probe device: %s\r\n",
                 pdevinstance->DEVHD_pcName);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __drvDetach
** ��������: ��������ϵ��豸�������İ�
** �䡡��  : pdrvinstance      ����ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  __drvDetach (PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DEV_INSTANCE     pdevinstance;
    PLW_LIST_LINE        plineNode;

    __DRV_DEV_LIST_LOCK(pdrvinstance);
    for (plineNode  = pdrvinstance->DRVHD_plineDevList;                 /*  ���������Ϲ������豸        */
         plineNode != NULL;) {

        pdevinstance  = _LIST_ENTRY(plineNode, LW_DEV_INSTANCE, DEVHD_lineBus);
        plineNode     = _list_line_get_next(plineNode);

        __DEV_HD_LOCK(pdevinstance);

        /*
         *  ���� remove �ӿ�
         */
        if (pdevinstance->DEVHD_pbustype->BUS_pfuncRemove) {
            pdevinstance->DEVHD_pbustype->BUS_pfuncRemove(pdevinstance);

        } else if(pdrvinstance->DRVHD_pfuncRemove) {
            pdrvinstance->DRVHD_pfuncRemove(pdevinstance);
        }

        _List_Line_Del(&pdevinstance->DEVHD_lineDrv,
                       &pdrvinstance->DRVHD_plineDevList);             /*  ������������ɾ���豸         */

        /*
         *  �������
         */
        pdevinstance->DEVHD_pdrvinstance = LW_NULL;
        API_DevSetDrvdata(pdevinstance, LW_NULL);

        __DEV_HD_UNLOCK(pdevinstance);
    }
    __DRV_DEV_LIST_UNLOCK(pdrvinstance);
}
/*********************************************************************************************************
** ��������: API_BusInit
** ��������: ���߳�ʼ��
** �䡡��  : pbustype      ����ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_BusInit (PLW_BUS_TYPE  pbustype)
{
    if (pbustype == LW_NULL) {
        return  (PX_ERROR);
    }

    __BUS_HD_LOCK_INIT(pbustype);                                       /*  ���߲�������ʼ��            */
    __BUS_DEV_LIST_LOCK_INIT(pbustype);                                 /*  �������豸�����������ʼ��  */
    __BUS_DRV_LIST_LOCK_INIT(pbustype);                                 /*  ���������������������ʼ��  */
    pbustype->BUS_plineDevList = LW_NULL;                               /*  �豸�����ʼ��              */
    pbustype->BUS_plineDrvList = LW_NULL;                               /*  ���������ʼ��              */
    pbustype->BUS_uiStatus = BUS_INITIALIZED;                           /*  ��������״̬�ѳ�ʼ��        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_BusFindDevice
** ��������: ͨ���豸���ڵ�����������Ƿ����д��豸
** �䡡��  : pbustype      ����ָ��
**           pdtnDev       �豸���ڵ�
** �䡡��  : �ɹ����ز��ҵ����豸ָ��, ʧ�ܷ��� LW_NULL
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_DEV_INSTANCE  API_BusFindDevice (PLW_BUS_TYPE  pbustype, PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEV_INSTANCE  pdevinstance = LW_NULL;
    PLW_LIST_LINE     plineNode;

    __BUS_DEV_LIST_LOCK(pbustype);
    for (plineNode  = pbustype->BUS_plineDevList;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  ���������ϵ���ע���豸      */

        pdevinstance = _LIST_ENTRY(plineNode, LW_DEV_INSTANCE, DEVHD_lineBus);
        if (pdevinstance->DEVHD_pdtnDev == pdtnDev) {                   /*  �豸���豸���ڵ���ͬ��ƥ��  */
            break;
        }
        pdevinstance = LW_NULL;
    }
    __BUS_DEV_LIST_UNLOCK(pbustype);

    return  (pdevinstance);
}
/*********************************************************************************************************
** ��������: API_DriverRegister
** ��������: ����ע��
** �䡡��  : pdrvinstance      ����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DriverRegister (PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DRV_INSTANCE  pdrvinstanceOther;
    PLW_BUS_TYPE      pbustype;
    INT               iRet;

    if (pdrvinstance == LW_NULL) {
        return  (PX_ERROR);
    }

    pbustype = pdrvinstance->DRVHD_pbustype;
    if (pbustype == LW_NULL) {
        return  (PX_ERROR);
    }

    if (!(pbustype->BUS_uiStatus & BUS_INITIALIZED)) {                  /*  ������δ��ʼ�����ش���      */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Bus %s was not initialized.\r\n",
                     pbustype->BUS_pcName);
        return  (PX_ERROR);
    }
    
    if (!pdrvinstance->DRVHD_pcName) {                                  /*  ��������δ��ʼ������ʧ��    */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Driver name is NULL\r\n");
        return  (PX_ERROR);
    }
    
    pdrvinstanceOther = __drvFind(pdrvinstance->DRVHD_pcName,
                                  pdrvinstance->DRVHD_pbustype);        /*  ���������Ƿ���ע��          */
    if (pdrvinstanceOther) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Driver %s is already registered\r\n",
                     pdrvinstance->DRVHD_pcName);
        return  (PX_ERROR);
    }

    iRet = __busAddDriver(pdrvinstance);                                /*  ��δע�ᣬ����������ע��    */

    return  (iRet);
}
/*********************************************************************************************************
** ��������: API_DriverUnregister
** ��������: ж������
** �䡡��  : pdrvinstance    ����ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_DriverUnregister (PLW_DRV_INSTANCE  pdrvinstance)
{
    if ((pdrvinstance == LW_NULL) || (pdrvinstance->DRVHD_pbustype == LW_NULL)) {
        return;
    }

    __busDelDriver(pdrvinstance);
}
/*********************************************************************************************************
** ��������: API_DeviceRegister
** ��������: ע���豸
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
INT  API_DeviceRegister (PLW_DEV_INSTANCE  pdevinstance)
{
    UINT  uiStatus;

    if ((pdevinstance == LW_NULL) ||
        (pdevinstance->DEVHD_pbustype == LW_NULL)) {
        return  (PX_ERROR);
    }

    uiStatus = pdevinstance->DEVHD_pbustype->BUS_uiStatus;
    if (!(uiStatus & BUS_INITIALIZED)) {                                /*  ������δ��ʼ������ʧ��      */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Bus %s was not initialized.\r\n",
                     pdevinstance->DEVHD_pbustype->BUS_pcName);
        return  (PX_ERROR);
    }

    __devInit(pdevinstance);

    return  (__devAdd(pdevinstance));
}
/*********************************************************************************************************
** ��������: API_DeviceUnregister
** ��������: ж���豸
** �䡡��  : pdevinstance    �豸ʵ��ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_DeviceUnregister (PLW_DEV_INSTANCE  pdevinstance)
{
    if ((pdevinstance == LW_NULL) ||
        (pdevinstance->DEVHD_pbustype == LW_NULL)) {
        return;
    }

    __devDel(pdevinstance);
}
/*********************************************************************************************************
** ��������: API_DevSetDrvdata
** ��������: �����豸˽������
** �䡡��  : pdevinstance    �豸ʵ��ָ��
**           pvData          �豸˽������ָ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
VOID  API_DevSetDrvdata (PLW_DEV_INSTANCE  pdevinstance, PVOID  pvData)
{
    pdevinstance->DEVHD_pvPrivData = pvData;
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
