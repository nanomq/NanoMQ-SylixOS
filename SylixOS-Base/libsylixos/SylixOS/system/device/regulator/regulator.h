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
** 文   件   名: regulator.h
**
** 创   建   人: Li.Zhi (李植)
**
** 文件创建日期: 2022 年 05 月 13 日
**
** 描        述: regulator 设备驱动框架
*********************************************************************************************************/

#ifndef __REGULATOR_H
#define __REGULATOR_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

struct lw_dt_regulator;
struct lw_dt_regulator_ops;

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

#define REGULATOR_VOLTAGE           (0)                                 /* regulator 类型               */
#define REGULATOR_CURRENT           (1)

#define REGULATOR_CHANGE_VOLTAGE    (0x1)                               /* regulator 可操作种类         */
#define REGULATOR_CHANGE_CURRENT    (0x2)
#define REGULATOR_CHANGE_MODE       (0x4)
#define REGULATOR_CHANGE_STATUS     (0x8)
#define REGULATOR_CHANGE_DRMS       (0x10)
#define REGULATOR_CHANGE_BYPASS     (0x20)

/*********************************************************************************************************
  regulator 操作结构体
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

    ULONG       REGOPS_ulPad[8];                                        /*  保留未来扩展                */
} LW_DT_REGULATOR_OPS;

typedef LW_DT_REGULATOR_OPS  *PLW_DT_REGULATOR_OPS;

/*********************************************************************************************************
  regulator 描述结构体
*********************************************************************************************************/

typedef struct lw_dt_regulator {
    LW_LIST_LINE                DTREGULATOR_lineManage;
    CHAR                        DTREGULATOR_cName[LW_CFG_OBJECT_NAME_SIZE];
    PCHAR                       DTREGULATOR_pcDtRegulatorName;          /* regulator 名称               */
    INT                         DTREGULATOR_iType;                      /* regulator 类型               */
    PLW_DT_REGULATOR_OPS        DTREGULATOR_pdtregulatorops;            /* regulator 操作集合           */

    PLW_DEV_INSTANCE            DTREGULATOR_pdev;
    PVOID                       DTREGULATOR_pvPrivData;                 /* regulator 私有数据           */

    struct lw_dt_regulator     *DTREGULATOR_pSupply;                    /* regulator 上层设备           */
    INT                         DTREGULATOR_iFixedVoltage;              /* regulator 固定电压           */
    INT                         DTREGULATOR_iVoltagesNum;
    INT                         DTREGULATOR_iUseCount;                  /* regulator 使用次数           */

    INT                         DTREGULATOR_iMinVoltage;                /* regulator 电压下限           */
    INT                         DTREGULATOR_iMaxVoltage;                /* regulator 电压上限           */
    INT                         DTREGULATOR_iVoltagesOffset;            /* regulator 电压补偿           */
    UINT                        DTREGULATOR_uiApplyVoltage;
    INT                         DTREGULATOR_iMinCurrent;                /* regulator 电压下限           */
    INT                         DTREGULATOR_iMaxCurrent;                /* regulator 电流上限           */
    INT                         DTREGULATOR_iLimCurrent;
    UINT                        DTREGULATOR_uiAlwaysOn;
    UINT                        DTREGULATOR_uiValidOpsMask;             /* regulator 可操作掩码         */

    ULONG                       DTREGULATOR_ulPad[16];                  /* 保留未来扩展                 */
} LW_DT_REGULATOR;

typedef LW_DT_REGULATOR  *PLW_DT_REGULATOR;

/*********************************************************************************************************
  对外接口函数
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
