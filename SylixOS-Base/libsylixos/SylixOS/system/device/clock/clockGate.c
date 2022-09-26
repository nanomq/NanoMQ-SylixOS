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
** ��   ��   ��: clockGate.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 27 ��
**
** ��        ��: �ſ�ʱ��������ֻ�ܿ���ʱ��ʹ�ܺͽ��ܣ�.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "clock.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static INT  __clockGateEnable(PLW_CLOCK  pclk);
static VOID __clockGateDisable(PLW_CLOCK  pclk);
static BOOL __clockGateIsEnabled(PLW_CLOCK  pclk);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsGate = {
    .clockEnable    = __clockGateEnable,
    .clockDisable   = __clockGateDisable,
    .clockIsEnabled = __clockGateIsEnabled,
};
/*********************************************************************************************************
** ��������: __clockGateEnable
** ��������: ʹ��ʱ��
** �䡡��  : pclk           ʱ���豸
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __clockGateEnable (PLW_CLOCK  pclk)
{
    PLW_CLOCK_GATE  pclkgate  = __CLK_TO_CLK_GATE(pclk);

    if (pclkgate->CLKG_pfuncEnable) {
        return  (pclkgate->CLKG_pfuncEnable(pclk));
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __clockGateDisable
** ��������: ����ʱ��
** �䡡��  : pclk           ʱ���豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __clockGateDisable (PLW_CLOCK  pclk)
{
    PLW_CLOCK_GATE  pclkgate  = __CLK_TO_CLK_GATE(pclk);

    if (pclkgate->CLKG_pfuncDisable) {
        pclkgate->CLKG_pfuncDisable(pclk);
    }
}
/*********************************************************************************************************
** ��������: __clockGateIsEnabled
** ��������: �ж�ʱ���Ƿ�ʹ��
** �䡡��  : pclk           ʱ���豸
** �䡡��  : LW_TRUE ʱ��ʹ�ܣ�LW_FALSE ʱ�ӽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL __clockGateIsEnabled (PLW_CLOCK  pclk)
{
    PLW_CLOCK_GATE  pclkgate  = __CLK_TO_CLK_GATE(pclk);

    if (pclkgate->CLKG_pfuncIsEnabled) {
        return  (pclkgate->CLKG_pfuncIsEnabled(pclk));
    }

    return  (LW_TRUE);
}
/*********************************************************************************************************
** ��������: API_ClockGateRegister
** ��������: ע��һ��������ʱ��
** �䡡��  : pcName            ʱ������
**           pcParentName      ��ʱ������
**           ulFlags           ��ʼ����ʶ
**           pfuncEnable       ʹ�ܲ�������
**           pfuncDisable      ���ܲ�������
**           pfuncIsEnabled    �Ƿ�ʹ���жϺ���
** �䡡��  : ע��Ŀ�����ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockGateRegister (CPCHAR       pcName,
                                  PCHAR       *ppcParentName,
                                  ULONG        ulFlags,
                                  FUNCPTR      pfuncEnable,
                                  FUNCPTR      pfuncDisable,
                                  BOOLFUNCPTR  pfuncIsEnabled)
{
    PLW_CLOCK_GATE       pclkgate;
    PLW_CLOCK            pclk;
    INT                  iRet;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkgate = __SHEAP_ZALLOC(sizeof(LW_CLOCK_GATE));
    if (!pclkgate) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkgate->CLKG_clk,
                                pcName,
                                &_G_clkopsGate,
                                ulFlags,
                                ppcParentName,
                                ppcParentName ? 1 : 0);
    if (iRet) {
        __SHEAP_FREE(pclkgate);
        return  (LW_NULL);
    }

    pclk = &pclkgate->CLKG_clk;
    pclkgate->CLKG_pfuncEnable    = pfuncEnable;
    pclkgate->CLKG_pfuncDisable   = pfuncDisable;
    pclkgate->CLKG_pfuncIsEnabled = pfuncIsEnabled;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkgate);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
