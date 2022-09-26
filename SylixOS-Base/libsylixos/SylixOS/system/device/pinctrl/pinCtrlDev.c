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
** 文   件   名: pinCtrlDev.c
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 02 日
**
** 描        述: pinCtrl 控制器接口实现
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "pinCtrl.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_LIST_LINE_HEADER               _G_plinePinCtrlDevs;           /*  pinctrl 控制器链表          */
/*********************************************************************************************************
  同步锁
*********************************************************************************************************/
static LW_OBJECT_HANDLE                  _G_hPctlDevLock = LW_OBJECT_HANDLE_INVALID;
static LW_OBJECT_HANDLE                  _G_hPctlPinLock = LW_OBJECT_HANDLE_INVALID;

#define __PIN_LOCK()                     API_SemaphoreMPend(_G_hPctlPinLock, LW_OPTION_WAIT_INFINITE)
#define __PIN_UNLOCK()                   API_SemaphoreMPost(_G_hPctlPinLock)
#define __PIN_LOCK_RETURN_NULL()         do { \
                                             if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) { \
                                                 return  (LW_NULL);                             \
                                             }                                                  \
                                             __PIN_LOCK();                                      \
                                         } while (0)
#define __PIN_UNLOCK_RETURN_NULL()       __PIN_UNLOCK()
#define __PIN_LOCK_RETURN_VOID()         do { \
                                             if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) { \
                                                 return;                                        \
                                             }                                                  \
                                             __PIN_LOCK();                                      \
                                         } while (0)
#define __PIN_UNLOCK_RETURN_VOID()       __PIN_UNLOCK()

#define __PCTLDEV_LOCK()                 API_SemaphoreMPend(_G_hPctlDevLock, LW_OPTION_WAIT_INFINITE)
#define __PCTLDEV_UNLOCK()               API_SemaphoreMPost(_G_hPctlDevLock)
#define __PCTLDEV_LOCK_RETURN_NULL()     do { \
                                             if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) { \
                                                 return  (LW_NULL);                             \
                                             }                                                  \
                                             __PCTLDEV_LOCK();                                  \
                                         } while (0)
#define __PCTLDEV_UNLOCK_RETURN_NULL()   __PCTLDEV_UNLOCK()
/*********************************************************************************************************
** 函数名称: __gpioToPin
** 功能描述: GPIO 转换为芯片引脚
** 输　入  : ppgpiorange        GPIO 范围
**           uiGpio             GPIO 序号
**           puiPin             输出芯片引脚
** 输　出  : 芯片引脚序号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static inline INT  __gpioToPin (PLW_PINCTRL_GPIO_RANGE  ppgpiorange, UINT  uiGpio, UINT  *puiPin)
{
    UINT  uiOffset = uiGpio - ppgpiorange->PCTLGR_uiGpioBase;

    if (uiOffset > ppgpiorange->PCTLGR_uiNPins) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    *puiPin = ppgpiorange->PCTLGR_uiPinBase + uiOffset;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlPinDescRegister
** 功能描述: 注册一个引脚的描述
** 输　入  : ppinctldev         引脚控制器
**           ppctlpindesc       引脚控制描述
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pinCtrlPinDescRegister (PLW_PINCTRL_DEV       ppinctldev,
                                      PLW_PINCTRL_PIN_DESC  ppctlpindesc)
{
    PLW_PIN_DESC  pindesc;

    pindesc = API_PinCtrlPinDescGet(ppinctldev,
                                    ppctlpindesc->PCTLPD_uiIndex);      /*  查看引脚是否已经被注册      */
    if (pindesc) {
        PCTL_LOG(PCTL_LOG_BUG, "pin %d already registered\r\n",
                 ppctlpindesc->PCTLPD_uiIndex);
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pindesc = __SHEAP_ZALLOC(sizeof(LW_PIN_DESC));
    if (!pindesc) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pindesc->PIN_ppinctldev = ppinctldev;
    pindesc->PIN_pcName     = ppctlpindesc->PCTLPD_pcName;
    pindesc->PIN_pvData     = ppctlpindesc->PCTLPD_pvData;
    pindesc->PIN_uiPin      = ppctlpindesc->PCTLPD_uiIndex;

    __PIN_LOCK();
    _List_Line_Add_Ahead(&pindesc->PIN_lineManage, &ppinctldev->PCTLD_plineDescs);
    __PIN_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlPinDescsAdd
** 功能描述: 注册引脚集合的描述
** 输　入  : ppinctldev          引脚控制器
**           ppctlpindesc        引脚控制描述集合
**           uiDescsNum          引脚控制描述集合元素个数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __pinCtrlPinDescsAdd (PLW_PINCTRL_DEV       ppinctldev,
                                  PLW_PINCTRL_PIN_DESC  ppctlpindescs,
                                  UINT                  uiDescsNum)
{
    INT     iRet = 0;
    UINT    i;

    for (i = 0; i < uiDescsNum; i++) {
        iRet = __pinCtrlPinDescRegister(ppinctldev, &ppctlpindescs[i]);
        if (iRet) {
            return  (iRet);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlPinDescsFree
** 功能描述: 释放引脚集合的描述
** 输　入  : ppinctldev         引脚控制器
**           ppctlpindescs      引脚控制描述集合
**           uiDescsNum         引脚控制描述集合元素个数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __pinCtrlPinDescsFree (PLW_PINCTRL_DEV       ppinctldev,
                                    PLW_PINCTRL_PIN_DESC  ppctlpindescs,
                                    UINT                  uiDescsNum)
{
    PLW_PIN_DESC     pindesc;
    INT              i;

    for (i = 0; i < uiDescsNum; i++) {
        pindesc = API_PinCtrlPinDescGet(ppinctldev,
                                        ppctlpindescs[i].PCTLPD_uiIndex);
        if (pindesc) {
            API_PinCtrlPinDescDel(ppinctldev, pindesc);
            __SHEAP_FREE(pindesc);
        }
    }
}
/*********************************************************************************************************
** 函数名称: __pinCtrlGpioRangeMatch
** 功能描述: 匹配 GPIO 范围
** 输　入  : ppinctldev         引脚控制器
**           uiGpio             GPIO 序号
** 输　出  : 匹配到的引脚范围
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_PINCTRL_GPIO_RANGE  __pinCtrlGpioRangeMatch (PLW_PINCTRL_DEV      ppinctldev,
                                                        UINT                 uiGpio)
{
    PLW_LIST_LINE           plineTemp;
    PLW_PINCTRL_GPIO_RANGE  pRange;

    for (plineTemp  = ppinctldev->PCTLD_plineGpioRange;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pRange = _LIST_ENTRY(plineTemp, LW_PINCTRL_GPIO_RANGE, PCTLGR_lineManage);

        if (uiGpio >= pRange->PCTLGR_uiGpioBase &&
            uiGpio < pRange->PCTLGR_uiGpioBase + pRange->PCTLGR_uiNPins) {

            return  (pRange);
        }
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __pinCtrlDevGpioRangeGet
** 功能描述: 获取 GPIO 范围
** 输　入  : uiGpio             GPIO 序号
**           ppGpioRange        GPIO 范围输出参数
** 输　出  : GPIO 范围
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PLW_PINCTRL_DEV  __pinCtrlDevGpioRangeGet (UINT  uiGpio, PLW_PINCTRL_GPIO_RANGE  *ppGpioRange)
{
    PLW_PINCTRL_DEV         ppinctldev;
    PLW_LIST_LINE           plineTemp;
    PLW_PINCTRL_GPIO_RANGE  pRange;

    if (!ppGpioRange) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    __PCTLDEV_LOCK_RETURN_NULL();
    for (plineTemp  = _G_plinePinCtrlDevs;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppinctldev = _LIST_ENTRY(plineTemp, LW_PINCTRL_DEV, PCTLD_lineManage);

        pRange = __pinCtrlGpioRangeMatch(ppinctldev, uiGpio);
        if (pRange) {
            *ppGpioRange = pRange;
            __PCTLDEV_UNLOCK_RETURN_NULL();
            return  (ppinctldev);
        }
    }
    __PCTLDEV_UNLOCK_RETURN_NULL();

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlPinDescGet
** 功能描述: 获取一个引脚描述
** 输　入  : ppinctldev      引脚控制器
**           uiPin           引脚序号
** 输　出  : 获得的引脚描述
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_PIN_DESC  API_PinCtrlPinDescGet (PLW_PINCTRL_DEV  ppinctldev, UINT  uiPin)
{
    PLW_PIN_DESC     pindesc;
    PLW_LIST_LINE    plineTemp;

    if (!ppinctldev) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    __PIN_LOCK_RETURN_NULL();
    for (plineTemp  = ppinctldev->PCTLD_plineDescs;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        pindesc = _LIST_ENTRY(plineTemp, LW_PIN_DESC, PIN_lineManage);
        if (pindesc->PIN_uiPin == uiPin) {
            __PIN_UNLOCK_RETURN_NULL();
            return  (pindesc);
        }
    }
    __PIN_UNLOCK_RETURN_NULL();

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlPinDescDel
** 功能描述: 删除一个引脚描述
** 输　入  : ppinctldev      引脚控制器
**           pindesc         引脚描述
** 输　出  : NONE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_PinCtrlPinDescDel (PLW_PINCTRL_DEV  ppinctldev, PLW_PIN_DESC  pindesc)
{
    if (!ppinctldev || !pindesc) {
        return;
    }

    __PIN_LOCK_RETURN_VOID();
    _List_Line_Del(&pindesc->PIN_lineManage, &ppinctldev->PCTLD_plineDescs);
    __PIN_UNLOCK_RETURN_VOID();
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlDevGetByDevtreeNode
** 功能描述: 从设备树节点获取引脚控制器
** 输　入  : pdtnDev      设备树节点
** 输　出  : 查找到的引脚控制器
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DEV  API_PinCtrlDevGetByDevtreeNode (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL_DEV  ppinctldev;
    PLW_LIST_LINE    plineTemp;

    __PCTLDEV_LOCK_RETURN_NULL();
    for (plineTemp  = _G_plinePinCtrlDevs;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        ppinctldev = _LIST_ENTRY(plineTemp, LW_PINCTRL_DEV, PCTLD_lineManage);
        if (ppinctldev->PCTLD_pdtnDev == pdtnDev) {
            __PCTLDEV_UNLOCK_RETURN_NULL();
            return  (ppinctldev);
        }
    }
    __PCTLDEV_UNLOCK_RETURN_NULL();

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlDescBuild
** 功能描述: 生成引脚控制器的描述结构
** 输　入  : pcName          引脚控制器的名称
**           ppinctlops      引脚控制操作集
**           ppinmuxops      引脚复用操作集
**           ppinconfops     引脚配置操作集
**           ppctlpindescs   引脚的描述集合
**           uiPinsNum       引脚的数量
** 输　出  : 创建的引脚控制器描述结构
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DESC  API_PinCtrlDescBuild (PCHAR                 pcName,
                                        PLW_PINCTRL_OPS       ppinctlops,
                                        PLW_PINMUX_OPS        ppinmuxops,
                                        PLW_PINCONF_OPS       ppinconfops,
                                        PLW_PINCTRL_PIN_DESC  ppctlpindescs,
                                        INT                   uiPinsNum)
{
    PLW_PINCTRL_DESC  ppinctrldesc;

    if (!ppinctlops || !ppinmuxops || !ppinconfops || !ppctlpindescs) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    ppinctrldesc = (PLW_PINCTRL_DESC)__SHEAP_ALLOC(sizeof(LW_PINCTRL_DESC));
    if (ppinctrldesc == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    ppinctrldesc->PCTLD_ppinctlops      = ppinctlops;
    ppinctrldesc->PCTLD_ppinmuxops      = ppinmuxops;
    ppinctrldesc->PCTLD_ppinconfops     = ppinconfops;
    ppinctrldesc->PCTLD_ppinctrlpindesc = ppctlpindescs;
    ppinctrldesc->PCTLD_uiPinsNum       = uiPinsNum;
    ppinctrldesc->PCTLD_pcName          = pcName;

    return  (ppinctrldesc);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlDevCreate
** 功能描述: 创建一个引脚控制器
** 输　入  : ppinctldesc     API_PinCtrlDescBuild 返回的引脚描述
**           pvData          引脚控制器私有参数
**           pdtnDev         控制器的设备树节点
** 输　出  : 创建的引脚控制器
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_PINCTRL_DEV  API_PinCtrlDevCreate (PLW_PINCTRL_DESC  ppinctldesc,
                                       PVOID             pvData,
                                       PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_PINCTRL_DEV  ppinctldev;
    INT              iRet;

    if (!ppinctldesc) {                                                 /*  参数检查                    */
        PCTL_LOG(PCTL_LOG_ERR, "pinctrl desc not found.\r\n");
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    ppinctldev = (PLW_PINCTRL_DEV)__SHEAP_ZALLOC(sizeof(LW_PINCTRL_DEV));
    if (ppinctldev == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    if (_G_hPctlDevLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlDevLock = API_SemaphoreMCreate("ppinctldev_lock", LW_PRIO_DEF_CEILING,
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    if (_G_hPctlPinLock == LW_OBJECT_HANDLE_INVALID) {
        _G_hPctlPinLock = API_SemaphoreMCreate("pctlpin_lock", LW_PRIO_DEF_CEILING,
                                               LW_OPTION_WAIT_PRIORITY |
                                               LW_OPTION_INHERIT_PRIORITY |
                                               LW_OPTION_DELETE_SAFE |
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    }

    ppinctldev->PCTLD_ppinctldesc = ppinctldesc;
    ppinctldev->PCTLD_pvData   = pvData;
    ppinctldev->PCTLD_pdtnDev  = pdtnDev;

    iRet = __pinCtrlPinDescsAdd(ppinctldev,
                                ppinctldesc->PCTLD_ppinctrlpindesc,
                                ppinctldesc->PCTLD_uiPinsNum);          /*  注册控制器相关的引脚        */
    if (iRet) {
        __pinCtrlPinDescsFree(ppinctldev,
                              ppinctldesc->PCTLD_ppinctrlpindesc,
                              ppinctldesc->PCTLD_uiPinsNum);
    }

    __PCTLDEV_LOCK();
    _List_Line_Add_Ahead(&ppinctldev->PCTLD_lineManage, &_G_plinePinCtrlDevs);
    __PCTLDEV_UNLOCK();

    return  (ppinctldev);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlGpioRangeAdd
** 功能描述: 添加 GPIO 引脚范围
** 输　入  : ppinctrldev           引脚控制设备
**           ppgpiorange           引脚范围
** 输　出  : ERROR_CODE
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlGpioRangeAdd (PLW_PINCTRL_DEV  ppinctrldev, PLW_PINCTRL_GPIO_RANGE  ppgpiorange)
{
    if (!ppinctrldev || !ppgpiorange) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __PCTLDEV_LOCK();
    _List_Line_Add_Ahead(&ppgpiorange->PCTLGR_lineManage, &ppinctrldev->PCTLD_plineGpioRange);
    __PCTLDEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlGpioRangeRemove
** 功能描述: 删除 GPIO 引脚范围
** 输　入  : ppinctrldev           引脚控制设备
**           ppgpiorange           引脚范围
** 输　出  : ERROR_CODE
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlGpioRangeRemove (PLW_PINCTRL_DEV  ppinctrldev, PLW_PINCTRL_GPIO_RANGE  ppgpiorange)
{
    if (!ppinctrldev || !ppgpiorange) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    __PCTLDEV_LOCK();
    _List_Line_Del(&ppgpiorange->PCTLGR_lineManage, &ppinctrldev->PCTLD_plineGpioRange);
    __PCTLDEV_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlGpioRequest
** 功能描述: 申请 GPIO 引脚
** 输　入  : uiGpio       GPIO 编号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PinCtrlGpioRequest (UINT  uiGpio)
{
    PLW_PINCTRL_DEV         ppinctldev;
    PLW_PINCTRL_GPIO_RANGE  ppgpiorange;
    UINT                    uiPin;
    PLW_PINMUX_OPS          ppinmuxOps;
    INT                     iRet;

    ppinctldev = __pinCtrlDevGpioRangeGet(uiGpio, &ppgpiorange);
    if (!ppinctldev) {
        return  (PX_ERROR);
    }

    iRet = __gpioToPin(ppgpiorange, uiGpio, &uiPin);
    if (iRet != ERROR_NONE) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    ppinmuxOps = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    if (ppinmuxOps->pinmuxGpioRequest) {
        iRet = ppinmuxOps->pinmuxGpioRequest(ppinctldev, ppgpiorange, uiPin);
        if (iRet != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_PinCtrlGpioFree
** 功能描述: 释放 GPIO 引脚
** 输　入  : uiGpio       GPIO 编号
** 输　出  : NONE
** 全局变量:
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
VOID  API_PinCtrlGpioFree (UINT  uiGpio)
{
    PLW_PINCTRL_DEV         ppinctldev;
    PLW_PINCTRL_GPIO_RANGE  ppgpiorange;
    UINT                    uiPin;
    PLW_PINMUX_OPS          ppinmuxOps;
    INT                     iRet;

    ppinctldev = __pinCtrlDevGpioRangeGet(uiGpio, &ppgpiorange);
    if (!ppinctldev) {
        return;
    }

    iRet = __gpioToPin(ppgpiorange, uiGpio, &uiPin);
    if (iRet != ERROR_NONE) {
        return;
    }

    ppinmuxOps = ppinctldev->PCTLD_ppinctldesc->PCTLD_ppinmuxops;
    if (ppinmuxOps->pinmuxGpioFree) {
        ppinmuxOps->pinmuxGpioFree(ppinctldev, ppgpiorange, uiPin);
    }
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
