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
** ��   ��   ��: regulator.h
**
** ��   ��   ��: Li.Zhi (��ֲ)
**
** �ļ���������: 2022 �� 05 �� 13 ��
**
** ��        ��: regulator �豸�������
*********************************************************************************************************/

#ifndef __REGULATOR_H
#define __REGULATOR_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

struct lw_dt_regulator;
struct lw_dt_regulator_ops;

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

#define REGULATOR_VOLTAGE           (0)                                 /* regulator ����               */
#define REGULATOR_CURRENT           (1)

#define REGULATOR_CHANGE_VOLTAGE    (0x1)                               /* regulator �ɲ�������         */
#define REGULATOR_CHANGE_CURRENT    (0x2)
#define REGULATOR_CHANGE_MODE       (0x4)
#define REGULATOR_CHANGE_STATUS     (0x8)
#define REGULATOR_CHANGE_DRMS       (0x10)
#define REGULATOR_CHANGE_BYPASS     (0x20)

/*********************************************************************************************************
  regulator �����ṹ��
*********************************************************************************************************/

typedef struct lw_dt_regulator_ops {
    INT       (*REGOPS_pfuncEnable)            (struct lw_dt_regulator    *pdtregulator);
    BOOL      (*REGOPS_pfuncIsEnabled)         (struct lw_dt_regulator    *pdtregulator);
    INT       (*REGOPS_pfuncDisabled)          (struct lw_dt_regulator    *pdtregulator);
    INT       (*REGOPS_pfuncGetBypass)         (struct lw_dt_regulator    *pdtregulator,
                                                BOOL                      *bEnable);
    INT       (*REGOPS_pfuncSetVoltage)        (struct lw_dt_regulator    *pdtregulator,
                                                INT                        iMin,
                                                INT                        iMax,
                                                UINT                      *uiSelector);
    INT       (*REGOPS_pfuncGetVoltage)        (struct lw_dt_regulator    *pdtregulator);
    INT       (*REGOPS_pfuncMapVoltage)        (struct lw_dt_regulator    *pdtregulator,
                                                INT                        iMin,
                                                INT                        iMax);
    INT       (*REGOPS_pfuncListVoltage)       (struct lw_dt_regulator    *pdtregulator,
                                                UINT                       uiSelector);
    INT       (*REGOPS_pfuncSetVoltageSel)     (struct lw_dt_regulator    *pdtregulator,
                                                UINT                       uiSelector);
    INT       (*REGOPS_pfuncGetVoltageSel)     (struct lw_dt_regulator    *pdtregulator);
    INT       (*REGOPS_pfuncSetVoltageTime)    (struct lw_dt_regulator    *pdtregulator,
                                                INT                        iOld,
                                                INT                        iNew);
    INT       (*REGOPS_pfuncSetVoltageTimeSel) (struct lw_dt_regulator    *pdtregulator,
                                                UINT                       uiOldSelector,
                                                UINT                       uiNewSelector);
    INT       (*REGOPS_pfuncSetCurrentLimit)   (struct lw_dt_regulator    *pdtregulator,
                                                INT                        iMin,
                                                INT                        iMax);
    INT       (*REGOPS_pfuncGetCurrentLimit)   (struct lw_dt_regulator    *pdtregulator);

    ULONG       REGOPS_ulPad[8];                                        /*  ����δ����չ                */
} LW_DT_REGULATOR_OPS;

typedef LW_DT_REGULATOR_OPS  *PLW_DT_REGULATOR_OPS;

/*********************************************************************************************************
  regulator �����ṹ��
*********************************************************************************************************/

typedef struct lw_dt_regulator {
    LW_LIST_LINE                DTREGULATOR_lineManage;
    CHAR                        DTREGULATOR_cName[LW_CFG_OBJECT_NAME_SIZE];
    PCHAR                       DTREGULATOR_pcDtRegulatorName;          /* regulator ����               */
    INT                         DTREGULATOR_iType;                      /* regulator ����               */
    PLW_DT_REGULATOR_OPS        DTREGULATOR_pdtregulatorops;            /* regulator ��������           */

    PLW_DEV_INSTANCE            DTREGULATOR_pdev;
    PVOID                       DTREGULATOR_pvPrivData;                 /* regulator ˽������           */

    struct lw_dt_regulator     *DTREGULATOR_pSupply;                    /* regulator �ϲ��豸           */
    INT                         DTREGULATOR_iFixedVoltage;              /* regulator �̶���ѹ           */
    INT                         DTREGULATOR_iVoltagesNum;
    INT                         DTREGULATOR_iUseCount;                  /* regulator ʹ�ô���           */

    INT                         DTREGULATOR_iMinVoltage;                /* regulator ��ѹ����           */
    INT                         DTREGULATOR_iMaxVoltage;                /* regulator ��ѹ����           */
    INT                         DTREGULATOR_iVoltagesOffset;            /* regulator ��ѹ����           */
    UINT                        DTREGULATOR_uiApplyVoltage;
    INT                         DTREGULATOR_iMinCurrent;                /* regulator ��ѹ����           */
    INT                         DTREGULATOR_iMaxCurrent;                /* regulator ��������           */
    INT                         DTREGULATOR_iLimCurrent;
    UINT                        DTREGULATOR_uiAlwaysOn;
    UINT                        DTREGULATOR_uiValidOpsMask;             /* regulator �ɲ�������         */

    ULONG                       DTREGULATOR_ulPad[16];                  /* ����δ����չ                 */
} LW_DT_REGULATOR;

typedef LW_DT_REGULATOR  *PLW_DT_REGULATOR;

/*********************************************************************************************************
  ����ӿں���
*********************************************************************************************************/

LW_API VOID              API_RegulatorInit(VOID);

LW_API INT               API_RegulatorRegister(PLW_DT_REGULATOR     pdtregulator,
                                               CPCHAR               pcName);

LW_API PLW_DT_REGULATOR  API_RegulatorGet(PLW_DEVTREE_NODE     pdtnDev);

LW_API INT               API_RegulatorVoltageGet(PLW_DT_REGULATOR     pdtregulator);

LW_API INT               API_RegulatorVoltageSet(PLW_DT_REGULATOR     pdtregulator,
                                                 INT                  iMin,
                                                 INT                  iMax);

LW_API BOOL              API_RegulatorIsEnabled(PLW_DT_REGULATOR     pdtregulator);

LW_API INT               API_RegulatorEnable(PLW_DT_REGULATOR     pdtregulator);

LW_API INT               API_RegulatorDisable(PLW_DT_REGULATOR     pdtregulator);

LW_API INT               API_RegulatorCurrentLimitSet(PLW_DT_REGULATOR     pdtregulator,
                                                      INT                  iMin,
                                                      INT                  iMax);

LW_API INT               API_RegulatorCurrentLimitGet(PLW_DT_REGULATOR     pdtregulator);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /* __REGULATOR_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
