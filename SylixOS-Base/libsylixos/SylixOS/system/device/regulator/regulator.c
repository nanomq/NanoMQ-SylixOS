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
** ��   ��   ��: regulator.c
**
** ��   ��   ��: Li.Zhi (��ֲ)
**
** �ļ���������: 2022 �� 05 �� 13 ��
**
** ��        ��: regulator �豸�������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
#include <limits.h>
#include <string.h>
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "devtree/devtree.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE             _G_hRegulatorListLock = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER          _G_plineRegulatorHeader;
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __REGULATOR_LIST_LOCK()     API_SemaphoreBPend(_G_hRegulatorListLock, LW_OPTION_WAIT_INFINITE)
#define __REGULATOR_LIST_UNLOCK()   API_SemaphoreBPost(_G_hRegulatorListLock)
/*********************************************************************************************************
** ��������: __regulatorMapVoltageIterate
** ��������: ���Һ��ʵĵ�ѹ����
** �䡡��  : pdtregulator      �豸ָ��
**           iMin              ��С��ѹֵ
**           iMax              ����ѹֵ
** �䡡��  : ��ѹ����, ����ʱ���� PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
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

    for (i = 0; i < pdtregulator->DTREGULATOR_iVoltagesNum; i++) {      /*  ����ȫ����ѹֵ              */
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
** ��������: __regulatorMapVoltage
** ��������: ���Һ��ʵĵ�ѹ����
** �䡡��  : pdtregulator      �豸ָ��
**           iMin              ��С��ѹֵ
**           iMax              ����ѹֵ
** �䡡��  : ��ѹ����, ����ʱ���� PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
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

    return  (__regulatorMapVoltageIterate(pdtregulator, iMin, iMax));   /*  ��������ʵĵ�ѹ            */
}
/*********************************************************************************************************
** ��������: __regulatorEnable
** ��������: ʹ�� Regulator
** �䡡��  : pdtregulator      �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __regulatorVoltageRecordUpdate
** ��������: ���� Regulator ����¼�ĵ�ѹ��Ϣ
** �䡡��  : pdtregulator      �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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

        if (pdtregulator->DTREGULATOR_iMinVoltage < iMinVoltage) {      /*  �޸ĵ�ѹ��Ϣ                */
            pdtregulator->DTREGULATOR_iMinVoltage = iMinVoltage;
        }
        if (pdtregulator->DTREGULATOR_iMaxVoltage > iMaxVoltage) {
            pdtregulator->DTREGULATOR_iMaxVoltage = iMaxVoltage;
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __regulatorConstraintsVoltageSet
** ��������: �ο� Regulator ������Ϣ���õ�ѹ
** �䡡��  : pdtregulator      �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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

        iCurrentVoltage = API_RegulatorVoltageGet(pdtregulator);        /*  ��ȡ��ǰ��ѹֵ              */
        if ((iCurrentVoltage < 0) && (errno == ENOTRECOVERABLE)) {      /*  ����δ��ʼ�豸��ѹ          */
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

    if (__regulatorVoltageRecordUpdate(pdtregulator)) {                 /*  �����豸��ѹ��Ϣ            */
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __regulatorConstraintsSet
** ��������: ����������Ϣ���� Regulator
** �䡡��  : pdtregulator      �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __regulatorConstraintsSet (PLW_DT_REGULATOR  pdtregulator)
{
    INT     iRet;

    iRet = __regulatorConstraintsVoltageSet(pdtregulator);
    if (iRet != 0) {
        return iRet;
    }

    if (pdtregulator->DTREGULATOR_uiAlwaysOn) {
        if (pdtregulator->DTREGULATOR_pSupply) {                        /*  ��������ϲ��豸����ʹ��    */
            iRet = __regulatorEnable(pdtregulator->DTREGULATOR_pSupply);
            if (iRet < 0) {
                pdtregulator->DTREGULATOR_pSupply = LW_NULL;
                return  (PX_ERROR);
            }
        }

        iRet = API_RegulatorEnable(pdtregulator);                       /*  �豸ʹ��                    */
        if (!iRet) {
            _DebugFormat(__ERRORMESSAGE_LEVEL, "failed to enable.\r\n");
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RegulatorInit
** ��������: ��ʼ�� Regulator �����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
VOID  API_RegulatorInit (VOID)
{
    if (_G_hRegulatorListLock == LW_OBJECT_HANDLE_INVALID) {            /*  ��ʼ regulator ��           */
        _G_hRegulatorListLock = API_SemaphoreBCreate("Regulator_listlock",
                                                     LW_TRUE, LW_OPTION_WAIT_FIFO |
                                                     LW_OPTION_OBJECT_GLOBAL,
                                                     LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_RegulatorRegister
** ��������: ע���豸
** �䡡��  : pdtregulator     �豸ָ��
**           pcName           �豸����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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
    if ((pdtregulator->DTREGULATOR_iType != REGULATOR_VOLTAGE) &&       /*  ��� regulator ��������     */
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

    iRet = __regulatorConstraintsSet(pdtregulator);                     /*  ��ȡ regulator ������Ϣ     */
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    __REGULATOR_LIST_LOCK();                                            /*  �� regulator ��ӵ�������   */
    _List_Line_Add_Ahead(&pdtregulator->DTREGULATOR_lineManage, &_G_plineRegulatorHeader);
    __REGULATOR_LIST_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RegulatorGet
** ��������: ��ȡ�豸
** �䡡��  : pdtnDev               �豸�ڵ�
** �䡡��  : pdtregulator          �豸ָ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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

    __REGULATOR_LIST_LOCK();                                            /*  ���� regulator ����         */
    for (plineTemp  = _G_plineRegulatorHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {                 /*  ���� regulator ����         */

        pdtregulator = _LIST_ENTRY(plineTemp, LW_DT_REGULATOR, DTREGULATOR_lineManage);
        if (pdtregulator->DTREGULATOR_pdev->DEVHD_pdtnDev == pdtnDev) {
            break;
        }
    }
    __REGULATOR_LIST_UNLOCK();                                          /*  ���� BUS ����               */

    if (plineTemp) {
        return  (pdtregulator);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: API_RegulatorVoltageGet
** ��������: ��ȡ��ѹֵ
** �䡡��  : pdtregulator               �豸ָ��
** �䡡��  : ��ѹֵ, ����ʱ���� PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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

    if (pdtregulatorops->REGOPS_pfuncGetBypass) {                       /*  ��������Դ�ṩ              */
        iRet = pdtregulatorops->REGOPS_pfuncGetBypass(pdtregulator, &bBypassed);
        if (iRet < 0) {
            return  (PX_ERROR);
        }

        if (bBypassed) {
            if (!pdtregulator->DTREGULATOR_pSupply) {                   /*  ��Ҫ��������Դ��Ϊ�ϲ��豸  */
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

    } else if (pdtregulator->DTREGULATOR_pSupply) {                     /*  ��ȡ�ϲ�ṹ���ṩ�ĵ�ѹ    */
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
** ��������: API_RegulatorVoltageSet
** ��������: ���õ�ѹֵ
** �䡡��  : pdtregulator      �豸ָ��
**           iMin              ��Сֵ
**           iMax              ���ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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

    if (API_RegulatorIsEnabled(pdtregulator) &&                         /*  ��� regulator �Ƿ���ʹ��   */
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
        iSelector = __regulatorMapVoltage(pdtregulator, iMin, iMax);    /*  ���Һ��ʵ�ѹ������          */
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

    if (pdtregulatorops->REGOPS_pfuncSetVoltageTimeSel) {               /*  ������ɺ󣬽�����ʱ����    */
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
** ��������: API_RegulatorIsEnabled
** ��������: ����Ƿ���ʹ��
** �䡡��  : pdtregulator               �豸ָ��
** �䡡��  : LW_TRUE/LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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
** ��������: API_RegulatorEnable
** ��������: ʹ���豸
** �䡡��  : pdtregulator               �豸ָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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
** ��������: API_RegulatorDisable
** ��������: �豸����
** �䡡��  : pdtregulator      �豸ָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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

    iRet = pdtregulatorops->REGOPS_pfuncDisabled(pdtregulator);         /*  �豸����                    */
    if (iRet < 0) {
        return  (PX_ERROR);
    }

    pdtregulator->DTREGULATOR_iUseCount = 0;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_RegulatorCurrentLimitSet
** ��������: ���õ�������
** �䡡��  : pdtregulator        �豸ָ��
**           iMin                ��Сֵ
**           iMax                ���ֵ
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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
** ��������: API_RegulatorCurrentLimitGet
** ��������: ��ȡ��������
** �䡡��  : pdtregulator               �豸ָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
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

    iRet = pdtregulatorops->REGOPS_pfuncGetCurrentLimit(pdtregulator);  /* ��ȡ��������                 */
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
