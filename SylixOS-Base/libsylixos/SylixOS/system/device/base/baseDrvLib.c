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
** 文   件   名: baseDrvLib.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 10 月 28 日
**
** 描        述: 基础驱动框架库
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "baseDrvLib.h"
/*********************************************************************************************************
  静态函数声明
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
  内联函数
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
  各种操作锁的宏定义
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
** 函数名称: __busAddDevice
** 功能描述: 在总线上添加设备
** 输　入  : pdevinstance      设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __busAddDevice (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DRV_INSTANCE  pdrvinstance;
    PLW_LIST_LINE     plineNode;
    PLW_BUS_TYPE      pbustype = pdevinstance->DEVHD_pbustype;
    INT               iRet;

    __BUS_DEV_LIST_LOCK(pbustype);
    _List_Line_Add_Ahead(&pdevinstance->DEVHD_lineBus,
                         &pbustype->BUS_plineDevList);                  /*  添加设备到总线链表          */
    __BUS_DEV_LIST_UNLOCK(pbustype);

    if ((pbustype->BUS_uiFlag & BUS_AUTO_PROBE) &&                      /*  总线使能了自动 probe        */
        (LW_NULL == pdevinstance->DEVHD_pdrvinstance)) {

        __DEV_HD_LOCK(pdevinstance);
        __BUS_DRV_LIST_LOCK(pbustype);

        for (plineNode  = pbustype->BUS_plineDrvList;
             plineNode != LW_NULL;
             plineNode  = _list_line_get_next(plineNode)) {             /*  遍历总线上的驱动            */

            pdrvinstance = _LIST_ENTRY(plineNode, LW_DRV_INSTANCE, DRVHD_lineBus);
            iRet = __drvMatchDev(pdevinstance, pdrvinstance);           /*  设备与驱动进行匹配          */
            if (!iRet) {
                __drvAttach(pdevinstance, pdrvinstance);                /*  匹配成功关联驱动            */
                break;
            }
        }

        __BUS_DRV_LIST_UNLOCK(pbustype);
        __DEV_HD_UNLOCK(pdevinstance);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __busDelDevice
** 功能描述: 从总线上删除设备
** 输　入  : pdevinstance      设备实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: __busAddDriver
** 功能描述: 向总线添加驱动
** 输　入  : pdrvinstance      驱动实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __busAddDriver (PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DEV_INSTANCE  pdevinstance;
    PLW_LIST_LINE     plineNode;
    PLW_BUS_TYPE      pbustype = pdrvinstance->DRVHD_pbustype;
    INT               iRet;

    __DRV_DEV_LIST_LOCK_INIT(pdrvinstance);                             /*  初始化驱动的设备链表操作锁  */
    pdrvinstance->DRVHD_plineDevList = LW_NULL;                         /*  初始化驱动的设备链表        */

    __BUS_DRV_LIST_LOCK(pbustype);
    _List_Line_Add_Ahead(&pdrvinstance->DRVHD_lineBus,
                         &pbustype->BUS_plineDrvList);                  /*  将驱动添加到总线驱动链表中  */
    __BUS_DRV_LIST_UNLOCK(pbustype);

    if (pbustype->BUS_uiFlag & BUS_AUTO_PROBE) {                        /*  设置了总线自动 probe        */

        __BUS_DEV_LIST_LOCK(pbustype);
        for (plineNode  = pbustype->BUS_plineDevList;
             plineNode != LW_NULL;
             plineNode  = _list_line_get_next(plineNode)) {             /*  遍历总线上的设备            */

            pdevinstance = _LIST_ENTRY(plineNode, LW_DEV_INSTANCE, DEVHD_lineBus);
            
            __DEV_HD_LOCK(pdevinstance);
            iRet = __drvMatchDev(pdevinstance, pdrvinstance);           /*  检查设备和驱动是否匹配      */
            if (!iRet) {
                __drvAttach(pdevinstance, pdrvinstance);                /*  设备和驱动匹配上则进行关联  */
            }
            __DEV_HD_UNLOCK(pdevinstance);
        }
        __BUS_DEV_LIST_UNLOCK(pbustype);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __busDelDriver
** 功能描述: 从总线上删除驱动
** 输　入  : pdrvinstance      驱动实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: __devInit
** 功能描述: 设备初始化
** 输　入  : pdevinstance      设备实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __devInit (PLW_DEV_INSTANCE  pdevinstance)
{
    __DEV_HD_LOCK_INIT(pdevinstance);
}
/*********************************************************************************************************
** 函数名称: __devReleaseDrv
** 功能描述: 解除设备与驱动的绑定
** 输　入  : pdevinstance      设备实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __devReleaseDrv (PLW_DEV_INSTANCE  pdevinstance)
{
    PLW_DRV_INSTANCE  pdrvinstance = pdevinstance->DEVHD_pdrvinstance;

    if (pdrvinstance) {
        __DEV_HD_LOCK(pdevinstance);

        /*
         *  调用 remove 接口
         */
        if (pdevinstance->DEVHD_pbustype->BUS_pfuncRemove) {
            pdevinstance->DEVHD_pbustype->BUS_pfuncRemove(pdevinstance);

        } else if (pdrvinstance->DRVHD_pfuncRemove) {
            pdrvinstance->DRVHD_pfuncRemove(pdevinstance);
        }

        __DRV_DEV_LIST_LOCK(pdrvinstance);
        _List_Line_Del(&pdevinstance->DEVHD_lineDrv,
                       &pdrvinstance->DRVHD_plineDevList);              /* 从驱动链表中删除设备         */
        __DRV_DEV_LIST_UNLOCK(pdrvinstance);

        /*
         *  清空数据
         */
        pdevinstance->DEVHD_pdrvinstance = LW_NULL;
        API_DevSetDrvdata(pdevinstance, LW_NULL);

        __DEV_HD_UNLOCK(pdevinstance);
    }
}
/*********************************************************************************************************
** 函数名称: __devDel
** 功能描述: 删除设备
** 输　入  : pdevinstance      设备实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __devDel (PLW_DEV_INSTANCE  pdevinstance)
{
    __busDelDevice(pdevinstance);
}
/*********************************************************************************************************
** 函数名称: __devAdd
** 功能描述: 添加设备
** 输　入  : pdevinstance      设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __devAdd (PLW_DEV_INSTANCE  pdevinstance)
{
    return  (__busAddDevice(pdevinstance));
}
/*********************************************************************************************************
** 函数名称: __drvFind
** 功能描述: 在总线上查找驱动
** 输　入  : pcName      驱动名字
**           pbustype    总线
** 输　出  : 成功返回驱动实例指针
**           失败返回 LW_NULL
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static PLW_DRV_INSTANCE  __drvFind (CPCHAR  pcName, PLW_BUS_TYPE  pbustype)
{
    PLW_DRV_INSTANCE  pdrvinstance;
    PLW_LIST_LINE     plineNode;

    __BUS_DRV_LIST_LOCK(pbustype);
    for (plineNode  = pbustype->BUS_plineDrvList;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode),
         pdrvinstance = LW_NULL) {                                      /*  遍历总线上的驱动链表        */

        pdrvinstance = _LIST_ENTRY(plineNode, LW_DRV_INSTANCE, DRVHD_lineBus);
        if (!lib_strcmp(pdrvinstance->DRVHD_pcName, pcName)) {          /*  通过名字进行比较匹配        */
            break;
        }
    }
    __BUS_DRV_LIST_UNLOCK(pbustype);

    if (plineNode == LW_NULL) {                                         /*  如果没匹配到                */
        pdrvinstance = LW_NULL;
    }

    return  (pdrvinstance);
}
/*********************************************************************************************************
** 函数名称: __drvMatchDev
** 功能描述: 检查驱动和设备是否匹配
** 输　入  : pdevinstance      设备实例指针
**           pdrvinstance      驱动实例指针
** 输　出  : 匹配返回 0, 不匹配返回其他
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __drvMatchDev (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    INT   iRet = 0;

    /*
     *  检测设备和驱动是否匹配，bus 的匹配函数不存在则默认匹配
     */
    if (pdrvinstance->DRVHD_pbustype->BUS_pfuncMatch) {
        iRet = pdrvinstance->DRVHD_pbustype->BUS_pfuncMatch(pdevinstance, pdrvinstance);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __drvAttach
** 功能描述: 将设备和驱动绑定
** 输　入  : pdevinstance    设备实例指针
**           pdrvinstance    驱动实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT __drvAttach (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    UINT    uiFlag;
    INT     iRet;

    /*
     *  绑定设备和驱动
     */
    __DRV_DEV_LIST_LOCK(pdrvinstance);
    _List_Line_Add_Ahead(&pdevinstance->DEVHD_lineDrv, &pdrvinstance->DRVHD_plineDevList);
    __DRV_DEV_LIST_UNLOCK(pdrvinstance);
    pdevinstance->DEVHD_pdrvinstance = pdrvinstance;

    iRet = API_PinBind(pdevinstance);                                   /*  进行设备引脚的绑定处理      */
    if (iRet) {
        goto  __error_handle;
    }

    uiFlag = pdevinstance->DEVHD_pbustype->BUS_uiFlag;
    if (pdevinstance->DEVHD_pbustype->BUS_pfuncProbe) {                 /*  总线的 probe 函数存在则调用 */
        iRet = pdevinstance->DEVHD_pbustype->BUS_pfuncProbe(pdevinstance);
    } else {
        uiFlag |= BUS_FORCE_DRV_PROBE;
    }

    if ((uiFlag & BUS_FORCE_DRV_PROBE) && (pdrvinstance->DRVHD_pfuncProbe)) {
        iRet = pdrvinstance->DRVHD_pfuncProbe(pdevinstance);            /*  驱动 probe 函数存在则调用   */
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
                   &pdrvinstance->DRVHD_plineDevList);                  /*  处理失败则解除绑定          */
    __DRV_DEV_LIST_UNLOCK(pdrvinstance);
    pdevinstance->DEVHD_pdrvinstance = NULL;
    
    _DebugFormat(__ERRORMESSAGE_LEVEL,"Failed to probe device: %s\r\n",
                 pdevinstance->DEVHD_pcName);
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __drvDetach
** 功能描述: 解除驱动上的设备与驱动的绑定
** 输　入  : pdrvinstance      驱动实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __drvDetach (PLW_DRV_INSTANCE  pdrvinstance)
{
    PLW_DEV_INSTANCE     pdevinstance;
    PLW_LIST_LINE        plineNode;

    __DRV_DEV_LIST_LOCK(pdrvinstance);
    for (plineNode  = pdrvinstance->DRVHD_plineDevList;                 /*  遍历驱动上关联的设备        */
         plineNode != NULL;) {

        pdevinstance  = _LIST_ENTRY(plineNode, LW_DEV_INSTANCE, DEVHD_lineBus);
        plineNode     = _list_line_get_next(plineNode);

        __DEV_HD_LOCK(pdevinstance);

        /*
         *  调用 remove 接口
         */
        if (pdevinstance->DEVHD_pbustype->BUS_pfuncRemove) {
            pdevinstance->DEVHD_pbustype->BUS_pfuncRemove(pdevinstance);

        } else if(pdrvinstance->DRVHD_pfuncRemove) {
            pdrvinstance->DRVHD_pfuncRemove(pdevinstance);
        }

        _List_Line_Del(&pdevinstance->DEVHD_lineDrv,
                       &pdrvinstance->DRVHD_plineDevList);             /*  从驱动链表中删除设备         */

        /*
         *  清空数据
         */
        pdevinstance->DEVHD_pdrvinstance = LW_NULL;
        API_DevSetDrvdata(pdevinstance, LW_NULL);

        __DEV_HD_UNLOCK(pdevinstance);
    }
    __DRV_DEV_LIST_UNLOCK(pdrvinstance);
}
/*********************************************************************************************************
** 函数名称: API_BusInit
** 功能描述: 总线初始化
** 输　入  : pbustype      总线实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_BusInit (PLW_BUS_TYPE  pbustype)
{
    if (pbustype == LW_NULL) {
        return  (PX_ERROR);
    }

    __BUS_HD_LOCK_INIT(pbustype);                                       /*  总线操作锁初始化            */
    __BUS_DEV_LIST_LOCK_INIT(pbustype);                                 /*  总线上设备链表操作锁初始化  */
    __BUS_DRV_LIST_LOCK_INIT(pbustype);                                 /*  总线上驱动链表操作锁初始化  */
    pbustype->BUS_plineDevList = LW_NULL;                               /*  设备链表初始化              */
    pbustype->BUS_plineDrvList = LW_NULL;                               /*  驱动链表初始化              */
    pbustype->BUS_uiStatus = BUS_INITIALIZED;                           /*  设置总线状态已初始化        */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_BusFindDevice
** 功能描述: 通过设备树节点查找总线上是否已有此设备
** 输　入  : pbustype      总线指针
**           pdtnDev       设备树节点
** 输　出  : 成功返回查找到的设备指针, 失败返回 LW_NULL
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_DEV_INSTANCE  API_BusFindDevice (PLW_BUS_TYPE  pbustype, PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEV_INSTANCE  pdevinstance = LW_NULL;
    PLW_LIST_LINE     plineNode;

    __BUS_DEV_LIST_LOCK(pbustype);
    for (plineNode  = pbustype->BUS_plineDevList;
         plineNode != LW_NULL;
         plineNode  = _list_line_get_next(plineNode)) {                 /*  遍历总线上的已注册设备      */

        pdevinstance = _LIST_ENTRY(plineNode, LW_DEV_INSTANCE, DEVHD_lineBus);
        if (pdevinstance->DEVHD_pdtnDev == pdtnDev) {                   /*  设备的设备树节点相同则匹配  */
            break;
        }
        pdevinstance = LW_NULL;
    }
    __BUS_DEV_LIST_UNLOCK(pbustype);

    return  (pdevinstance);
}
/*********************************************************************************************************
** 函数名称: API_DriverRegister
** 功能描述: 驱动注册
** 输　入  : pdrvinstance      驱动指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
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

    if (!(pbustype->BUS_uiStatus & BUS_INITIALIZED)) {                  /*  总线尚未初始化返回错误      */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Bus %s was not initialized.\r\n",
                     pbustype->BUS_pcName);
        return  (PX_ERROR);
    }
    
    if (!pdrvinstance->DRVHD_pcName) {                                  /*  驱动名字未初始化返回失败    */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Driver name is NULL\r\n");
        return  (PX_ERROR);
    }
    
    pdrvinstanceOther = __drvFind(pdrvinstance->DRVHD_pcName,
                                  pdrvinstance->DRVHD_pbustype);        /*  查找驱动是否已注册          */
    if (pdrvinstanceOther) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Driver %s is already registered\r\n",
                     pdrvinstance->DRVHD_pcName);
        return  (PX_ERROR);
    }

    iRet = __busAddDriver(pdrvinstance);                                /*  尚未注册，则在总线上注册    */

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_DriverUnregister
** 功能描述: 卸载驱动
** 输　入  : pdrvinstance    驱动指针
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
**                                            API 函数
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
** 函数名称: API_DeviceRegister
** 功能描述: 注册设备
** 输　入  : pdevinstance    设备实例指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
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
    if (!(uiStatus & BUS_INITIALIZED)) {                                /*  总线尚未初始化返回失败      */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Bus %s was not initialized.\r\n",
                     pdevinstance->DEVHD_pbustype->BUS_pcName);
        return  (PX_ERROR);
    }

    __devInit(pdevinstance);

    return  (__devAdd(pdevinstance));
}
/*********************************************************************************************************
** 函数名称: API_DeviceUnregister
** 功能描述: 卸载设备
** 输　入  : pdevinstance    设备实例指针
** 输　出  : NONE
** 全局变量: 
** 调用模块:
**                                            API 函数
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
** 函数名称: API_DevSetDrvdata
** 功能描述: 设置设备私有数据
** 输　入  : pdevinstance    设备实例指针
**           pvData          设备私有数据指针
** 输　出  : NONE
** 全局变量: 
** 调用模块:
**                                            API 函数
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
