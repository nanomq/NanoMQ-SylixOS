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
** 文   件   名: regulator.c
**
** 创   建   人: Li.Zhi (李植)
**
** 文件创建日期: 2022 年 05 月 13 日
**
** 描        述: regulator 设备驱动框架
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
#include <limits.h>
#include <string.h>
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_hRegulatorListLock = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER          _G_plineRegulatorHeader;
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __REGULATOR_LIST_LOCK()     API_SemaphoreBPend(_G_hRegulatorListLock, LW_OPTION_WAIT_INFINITE)
#define __REGULATOR_LIST_UNLOCK()   API_SemaphoreBPost(_G_hRegulatorListLock)
/*********************************************************************************************************
** 函数名称: __regulatorMapVoltageIterate
** 功能描述: 查找合适的电压索引
** 输　入  : pdtregulator      设备指针
**           iMin              最小电压值
**           iMax              最大电压值
** 输　出  : 电压索引, 错误时返回 PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __regulatorMapVoltageIterate (PLW_DT_REGULATOR  pdtregulator,
                                          INT               iMin,
                                          INT               iMax)
{
    INT                     iBestVal = INT_MAX;
    INT                     iSelector= 0;
    INT                     i;
    INT                     iRet;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;

    for (i = 0; i < pdtregulator->DTREGULATOR_iVoltagesNum; i++) {      /*  遍历全部电压值              */
        iRet = pdtregulatorops->REGOPS_pfuncListVoltage(pdtregulator, i);
        if (iRet < 0) {
            continue;
        }

        if ((iRet < iBestVal) && (iRet >= iMin) && (iRet <= iMax)) {
            iBestVal  = iRet;
            iSelector = i;
        }
    }

    if (iBestVal == INT_MAX) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (iSelector);
}
/*********************************************************************************************************
** 函数名称: __regulatorMapVoltage
** 功能描述: 查找合适的电压索引
** 输　入  : pdtregulator      设备指针
**           iMin              最小电压值
**           iMax              最大电压值
** 输　出  : 电压索引, 错误时返回 PX_ERROR
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __regulatorMapVoltage (PLW_DT_REGULATOR  pdtregulator,
                                   INT               iMin,
                                   INT               iMax)
{
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;
    if (pdtregulatorops->REGOPS_pfuncMapVoltage) {
        return  (pdtregulatorops->REGOPS_pfuncMapVoltage(pdtregulator, iMin, iMax));
    }

    return  (__regulatorMapVoltageIterate(pdtregulator, iMin, iMax));   /*  查找最合适的电压            */
}
/*********************************************************************************************************
** 函数名称: __regulatorEnable
** 功能描述: 使能 Regulator
** 输　入  : pdtregulator      设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __regulatorEnable (PLW_DT_REGULATOR  pdtregulator)
{
    INT     iRet;

    if ((pdtregulator->DTREGULATOR_iUseCount == 0) && pdtregulator->DTREGULATOR_pSupply) {
        iRet = __regulatorEnable(pdtregulator->DTREGULATOR_pSupply);
        if (iRet < 0) {
            return  (PX_ERROR);
        }
    }

    if (pdtregulator->DTREGULATOR_iUseCount == 0) {
        iRet = API_RegulatorIsEnabled(pdtregulator);
        if (!iRet) {
            iRet = API_RegulatorEnable(pdtregulator);
            if (iRet < 0) {
                return  (PX_ERROR);
            }
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __regulatorVoltageRecordUpdate
** 功能描述: 更新 Regulator 所记录的电压信息
** 输　入  : pdtregulator      设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __regulatorVoltageRecordUpdate (PLW_DT_REGULATOR  pdtregulator)
{
    INT                     i;
    INT                     iValue;
    INT                     iMinVoltage = INT_MAX;
    INT                     iMaxVoltage = INT_MIN;
    INT                     iMin;
    INT                     iMax;
    INT                     iCount;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iMin   = pdtregulator->DTREGULATOR_iMinVoltage;
    iMax   = pdtregulator->DTREGULATOR_iMaxVoltage;
    iCount = pdtregulator->DTREGULATOR_iVoltagesNum;

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;

    if (pdtregulatorops->REGOPS_pfuncListVoltage && pdtregulator->DTREGULATOR_iVoltagesNum) {
        if ((iCount == 1) && !iMin) {
            iMin = 1;
            iMax = INT_MAX;
            pdtregulator->DTREGULATOR_iMinVoltage = iMin;
            pdtregulator->DTREGULATOR_iMaxVoltage = iMax;
        }

        if ((iMin == 0) && (iMax == 0)) {
            return  (ERROR_NONE);
        }

        if ((iMin <= 0) || (iMax <= 0) || (iMax < iMin)) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "invalid voltage constraints\n");
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        for (i = 0; i < iCount; i++) {
            iValue = pdtregulatorops->REGOPS_pfuncListVoltage(pdtregulator, i);
            if (iValue <= 0) {
                continue;
            }

            if ((iValue >= iMin) && (iValue < iMinVoltage)) {
                iMinVoltage = iValue;
            }

            if ((iValue <= iMax) && (iValue > iMaxVoltage)) {
                iMaxVoltage = iValue;
            }
        }

        if (iMaxVoltage < iMinVoltage) {
            _DebugFormat(__ERRORMESSAGE_LEVEL,
                         "unsupportable voltage constraints %u-%uuV\n",
                         iMinVoltage, iMaxVoltage);
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if (pdtregulator->DTREGULATOR_iMinVoltage < iMinVoltage) {      /*  修改电压信息                */
            pdtregulator->DTREGULATOR_iMinVoltage = iMinVoltage;
        }
        if (pdtregulator->DTREGULATOR_iMaxVoltage > iMaxVoltage) {
            pdtregulator->DTREGULATOR_iMaxVoltage = iMaxVoltage;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __regulatorConstraintsVoltageSet
** 功能描述: 参考 Regulator 已有信息设置电压
** 输　入  : pdtregulator      设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __regulatorConstraintsVoltageSet (PLW_DT_REGULATOR  pdtregulator)
{
    INT     iRet;
    INT     iTargetMin;
    INT     iTargetMax;
    INT     iCurrentVoltage;

    if (pdtregulator->DTREGULATOR_uiApplyVoltage &&
        pdtregulator->DTREGULATOR_iMinVoltage    &&
        pdtregulator->DTREGULATOR_iMaxVoltage) {

        iCurrentVoltage = API_RegulatorVoltageGet(pdtregulator);        /*  获取当前电压值              */
        if ((iCurrentVoltage < 0) && (errno == ENOTRECOVERABLE)) {      /*  设置未初始设备电压          */
            API_RegulatorVoltageSet(pdtregulator,
                                    pdtregulator->DTREGULATOR_iMinVoltage,
                                    pdtregulator->DTREGULATOR_iMaxVoltage);
            iCurrentVoltage = API_RegulatorVoltageGet(pdtregulator);
        }

        if (iCurrentVoltage < 0) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "failed to get the current voltage.\r\n");
            return  (iCurrentVoltage);
        }

        iTargetMin = iCurrentVoltage;
        iTargetMax = iCurrentVoltage;

        if (iCurrentVoltage < pdtregulator->DTREGULATOR_iMinVoltage) {
            iTargetMin = pdtregulator->DTREGULATOR_iMinVoltage;
            iTargetMax = pdtregulator->DTREGULATOR_iMinVoltage;
        }

        if (iCurrentVoltage > pdtregulator->DTREGULATOR_iMaxVoltage) {
            iTargetMin = pdtregulator->DTREGULATOR_iMaxVoltage;
            iTargetMax = pdtregulator->DTREGULATOR_iMaxVoltage;
        }

        if ((iTargetMin != iCurrentVoltage) || (iTargetMax != iCurrentVoltage)) {
            iRet = API_RegulatorVoltageSet(pdtregulator,
                                           iTargetMin,
                                           iTargetMax);
            if (iRet < 0) {
                _DebugFormat(__ERRORMESSAGE_LEVEL,
                             "failed to apply %d-%duV constraint.\r\n",
                             iTargetMin, iTargetMax);
                return  (PX_ERROR);
            }
        }
    }

    if (__regulatorVoltageRecordUpdate(pdtregulator)) {                 /*  更新设备电压信息            */
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __regulatorConstraintsSet
** 功能描述: 根据已有信息设置 Regulator
** 输　入  : pdtregulator      设备指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __regulatorConstraintsSet (PLW_DT_REGULATOR  pdtregulator)
{
    INT     iRet;

    iRet = __regulatorConstraintsVoltageSet(pdtregulator);
    if (iRet != 0) {
        return iRet;
    }

    if (pdtregulator->DTREGULATOR_uiAlwaysOn) {
        if (pdtregulator->DTREGULATOR_pSupply) {                        /*  如果存在上层设备，则使能    */
            iRet = __regulatorEnable(pdtregulator->DTREGULATOR_pSupply);
            if (iRet < 0) {
                pdtregulator->DTREGULATOR_pSupply = LW_NULL;
                return  (PX_ERROR);
            }
        }

        iRet = API_RegulatorEnable(pdtregulator);                       /*  设备使能                    */
        if (!iRet) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "failed to enable.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorInit
** 功能描述: 初始化 Regulator 组件库
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
VOID  API_RegulatorInit (VOID)
{
    if (_G_hRegulatorListLock == LW_OBJECT_HANDLE_INVALID) {            /*  初始 regulator 锁           */
        _G_hRegulatorListLock = API_SemaphoreBCreate("Regulator_listlock",
                                                     LW_TRUE, LW_OPTION_WAIT_FIFO |
                                                     LW_OPTION_OBJECT_GLOBAL,
                                                     LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_RegulatorRegister
** 功能描述: 注册设备
** 输　入  : pdtregulator     设备指针
**           pcName           设备名称
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RegulatorRegister (PLW_DT_REGULATOR  pdtregulator, CPCHAR  pcName)
{
    INT                     iRet;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator || !pcName || !pdtregulator->DTREGULATOR_pdtregulatorops) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;
    if ((pdtregulator->DTREGULATOR_iType != REGULATOR_VOLTAGE) &&       /*  检查 regulator 操作集合     */
        (pdtregulator->DTREGULATOR_iType != REGULATOR_CURRENT)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pdtregulatorops->REGOPS_pfuncGetVoltageSel) {
        if (pdtregulatorops->REGOPS_pfuncGetVoltage ||
            !pdtregulatorops->REGOPS_pfuncListVoltage) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    if (pdtregulatorops->REGOPS_pfuncSetVoltageSel) {
        if (pdtregulatorops->REGOPS_pfuncSetVoltage ||
            !pdtregulatorops->REGOPS_pfuncListVoltage) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }

    lib_strlcpy(pdtregulator->DTREGULATOR_cName, pcName, LW_CFG_OBJECT_NAME_SIZE);
    pdtregulator->DTREGULATOR_iUseCount = 0;

    iRet = __regulatorConstraintsSet(pdtregulator);                     /*  获取 regulator 固有信息     */
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    __REGULATOR_LIST_LOCK();                                            /*  将 regulator 添加到链表中   */
    _List_Line_Add_Ahead(&pdtregulator->DTREGULATOR_lineManage, &_G_plineRegulatorHeader);
    __REGULATOR_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorGet
** 功能描述: 获取设备
** 输　入  : pdtnDev               设备节点
** 输　出  : pdtregulator          设备指针
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
PLW_DT_REGULATOR  API_RegulatorGet (PLW_DEVTREE_NODE  pdtnDev)
{
    LW_LIST_LINE       *plineTemp;
    PLW_DT_REGULATOR    pdtregulator = LW_NULL;

    if (!pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    __REGULATOR_LIST_LOCK();                                            /*  锁定 regulator 链表         */
    for (plineTemp  = _G_plineRegulatorHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  遍历 regulator 链表         */

        pdtregulator = _LIST_ENTRY(plineTemp, LW_DT_REGULATOR, DTREGULATOR_lineManage);
        if (pdtregulator->DTREGULATOR_pdev->DEVHD_pdtnDev == pdtnDev) {
            break;
        }
    }
    __REGULATOR_LIST_UNLOCK();                                          /*  解锁 BUS 链表               */

    if (plineTemp) {
        return  (pdtregulator);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** 函数名称: API_RegulatorVoltageGet
** 功能描述: 获取电压值
** 输　入  : pdtregulator               设备指针
** 输　出  : 电压值, 错误时返回 PX_ERROR
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RegulatorVoltageGet (PLW_DT_REGULATOR  pdtregulator)
{
    INT                     iSel;
    INT                     iRet;
    BOOL                    bBypassed;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;

    if (pdtregulatorops->REGOPS_pfuncGetBypass) {                       /*  由其他电源提供              */
        iRet = pdtregulatorops->REGOPS_pfuncGetBypass(pdtregulator, &bBypassed);
        if (iRet < 0) {
            return  (PX_ERROR);
        }

        if (bBypassed) {
            if (!pdtregulator->DTREGULATOR_pSupply) {                   /*  需要将其他电源设为上层设备  */
                _DebugFormat(__ERRORMESSAGE_LEVEL, "bypassed regulator has no supply!\r\n");
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }

            return  (API_RegulatorVoltageGet(pdtregulator->DTREGULATOR_pSupply));
        }
    }

    if (pdtregulatorops->REGOPS_pfuncGetVoltageSel) {
        iSel = pdtregulatorops->REGOPS_pfuncGetVoltageSel(pdtregulator);
        if (iSel < 0) {
            return  (PX_ERROR);
        }

        iRet = pdtregulatorops->REGOPS_pfuncListVoltage(pdtregulator, iSel);

    } else if (pdtregulatorops->REGOPS_pfuncGetVoltage) {
        iRet = pdtregulatorops->REGOPS_pfuncGetVoltage(pdtregulator);

    } else if (pdtregulatorops->REGOPS_pfuncListVoltage) {
        iRet = pdtregulatorops->REGOPS_pfuncListVoltage(pdtregulator, 0);

    } else if (pdtregulator->DTREGULATOR_iFixedVoltage &&
               (pdtregulator->DTREGULATOR_iVoltagesNum == 1)) {
        iRet = pdtregulator->DTREGULATOR_iFixedVoltage;

    } else if (pdtregulator->DTREGULATOR_pSupply) {                     /*  获取上层结构所提供的电压    */
        iRet = API_RegulatorVoltageGet(pdtregulator->DTREGULATOR_pSupply);

    } else {
        return  (PX_ERROR);
    }

    if (iRet < 0) {
        return  (PX_ERROR);
    }

    return  (iRet - pdtregulator->DTREGULATOR_iVoltagesOffset);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorVoltageSet
** 功能描述: 设置电压值
** 输　入  : pdtregulator      设备指针
**           iMin              最小值
**           iMax              最大值
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RegulatorVoltageSet (PLW_DT_REGULATOR  pdtregulator,
                              INT               iMin,
                              INT               iMax)
{
    INT                     iRet;
    UINT                    iSelector;
    INT                     iOld;
    INT                     idelay       = 0;
    INT                     ibestVal     = 0;
    INT                     iOldSelector = -1;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;

    iOld = API_RegulatorVoltageGet(pdtregulator);

    iMin += pdtregulator->DTREGULATOR_iVoltagesOffset;
    iMax += pdtregulator->DTREGULATOR_iVoltagesOffset;

    if (API_RegulatorIsEnabled(pdtregulator) &&                         /*  检查 regulator 是否已使能   */
        pdtregulatorops->REGOPS_pfuncSetVoltageTimeSel &&
        pdtregulatorops->REGOPS_pfuncGetVoltageSel) {
        iOldSelector = pdtregulatorops->REGOPS_pfuncGetVoltageSel(pdtregulator);
        if (iOldSelector < 0) {
            return  (PX_ERROR);
        }
    }

    if (pdtregulatorops->REGOPS_pfuncSetVoltage) {
        iRet = pdtregulatorops->REGOPS_pfuncSetVoltage(pdtregulator, iMin, iMax, &iSelector);
        if (iRet >= 0) {
            if (pdtregulatorops->REGOPS_pfuncListVoltage) {
                ibestVal = pdtregulatorops->REGOPS_pfuncListVoltage(pdtregulator, iSelector);
            } else {
                ibestVal = API_RegulatorVoltageGet(pdtregulator);
            }
        }
    } else if (pdtregulatorops->REGOPS_pfuncGetVoltageSel) {
        iSelector = __regulatorMapVoltage(pdtregulator, iMin, iMax);    /*  查找合适电压并设置          */
        if (iSelector >= 0) {
            ibestVal = pdtregulatorops->REGOPS_pfuncListVoltage(pdtregulator, iSelector);
            if ((ibestVal < iMin) || (ibestVal > iMax)) {
                _ErrorHandle(EINVAL);
                return  (PX_ERROR);
            }

            if (iOldSelector == iSelector) {
                return  (ERROR_NONE);
            } else {
                iRet = pdtregulatorops->REGOPS_pfuncSetVoltageSel(pdtregulator, iSelector);
            }
        }
    } else {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (iRet) {
        return  (iRet);
    }

    if (pdtregulatorops->REGOPS_pfuncSetVoltageTimeSel) {               /*  设置完成后，进行延时处理    */
        if ((iOldSelector >= 0) && (iOldSelector != iSelector))
            idelay = pdtregulatorops->REGOPS_pfuncSetVoltageTimeSel(pdtregulator,
                                                                    iOldSelector,
                                                                    iSelector);
    } else if (pdtregulatorops->REGOPS_pfuncSetVoltageTime && (iOld != ibestVal)) {
        idelay = pdtregulatorops->REGOPS_pfuncSetVoltageTime(pdtregulator,
                                                             iOld,
                                                             ibestVal);
    }

    API_TimeMSleep((idelay / 1000) + 1);

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorIsEnabled
** 功能描述: 检查是否已使能
** 输　入  : pdtregulator               设备指针
** 输　出  : LW_TRUE/LW_FALSE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
BOOL  API_RegulatorIsEnabled (PLW_DT_REGULATOR  pdtregulator)
{
    PLW_DT_REGULATOR_OPS    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;

    if (!pdtregulatorops->REGOPS_pfuncIsEnabled) {
        return  (LW_TRUE);
    }

    return  (pdtregulatorops->REGOPS_pfuncIsEnabled(pdtregulator));
}
/*********************************************************************************************************
** 函数名称: API_RegulatorEnable
** 功能描述: 使能设备
** 输　入  : pdtregulator               设备指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RegulatorEnable (PLW_DT_REGULATOR  pdtregulator)
{
    INT                     iRet;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pdtregulator->DTREGULATOR_iUseCount >= 1) {
        pdtregulator->DTREGULATOR_iUseCount++;
        return  (ERROR_NONE);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;
    if (!pdtregulatorops->REGOPS_pfuncEnable) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iRet = pdtregulatorops->REGOPS_pfuncEnable(pdtregulator);
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    pdtregulator->DTREGULATOR_iUseCount++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorDisable
** 功能描述: 设备禁能
** 输　入  : pdtregulator      设备指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API INT  API_RegulatorDisable (PLW_DT_REGULATOR  pdtregulator)
{
    int                     iRet;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pdtregulator->DTREGULATOR_iUseCount < 1) {
        return  (ERROR_NONE);
    }

    if (pdtregulator->DTREGULATOR_iUseCount > 1) {
        pdtregulator->DTREGULATOR_iUseCount--;
        return  (ERROR_NONE);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;
    if (!pdtregulatorops->REGOPS_pfuncDisabled) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iRet = pdtregulatorops->REGOPS_pfuncDisabled(pdtregulator);         /*  设备禁能                    */
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    pdtregulator->DTREGULATOR_iUseCount = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorCurrentLimitSet
** 功能描述: 设置电流限制
** 输　入  : pdtregulator        设备指针
**           iMin                最小值
**           iMax                最大值
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RegulatorCurrentLimitSet (PLW_DT_REGULATOR  pdtregulator,
                                   INT               iMin,
                                   INT               iMax)
{
    INT                     iRet;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;
    if (!pdtregulatorops->REGOPS_pfuncSetCurrentLimit) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iRet = pdtregulatorops->REGOPS_pfuncSetCurrentLimit(pdtregulator, iMin, iMax);
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_RegulatorCurrentLimitGet
** 功能描述: 获取电流限制
** 输　入  : pdtregulator               设备指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_RegulatorCurrentLimitGet (PLW_DT_REGULATOR  pdtregulator)
{
    INT                     iRet;
    PLW_DT_REGULATOR_OPS    pdtregulatorops;

    if (!pdtregulator) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtregulatorops = pdtregulator->DTREGULATOR_pdtregulatorops;
    if (!pdtregulatorops->REGOPS_pfuncGetCurrentLimit) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iRet = pdtregulatorops->REGOPS_pfuncGetCurrentLimit(pdtregulator);  /* 获取电流限制                 */
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
