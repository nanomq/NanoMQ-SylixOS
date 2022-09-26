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
** ��   ��   ��: clockFixedFactor.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 26 ��
**
** ��        ��: �й̶���Ƶ��Ƶֵ��ʱ��
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
static ULONG  __clockFixedFactorRateRecalc(PLW_CLOCK  pclk, ULONG  ulParentRate);
static LONG   __clockFixedFactorRateRound(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulRate);
static INT    __clockFixedFactorRateSet(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsFixedFactor = {
    .clockRateRecalc = __clockFixedFactorRateRecalc,
    .clockRateRound  = __clockFixedFactorRateRound,
    .clockRateSet    = __clockFixedFactorRateSet,
};
/*********************************************************************************************************
** ��������: __clockFixedFactorRateRecalc
** ��������: ��÷�Ƶ��Ƶʱ�ӵ�Ƶ��ֵ
** �䡡��  : pclk             ʱ���豸
**           ulParentRate     ��ʱ��Ƶ��
** �䡡��  : �̶�Ƶ��ʱ�ӵ�Ƶ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __clockFixedFactorRateRecalc (PLW_CLOCK  pclk, ULONG  ulParentRate)
{
    PLW_CLOCK_FIXED_FACTOR  pclkfixedfactor = __CLK_TO_CLK_FIXED_FACTOR(pclk);
    UINT64                  ullRate;
    lib_lldiv_t             ullDiv;

    ullRate = (UINT64)ulParentRate * pclkfixedfactor->CLKFF_uiFixedMult;/*  ��ʱ�ӱ�Ƶ                  */
    ullDiv  = lib_lldiv(ullRate, pclkfixedfactor->CLKFF_uiFixedDiv);    /*  ��ʱ�ӷ�Ƶ                  */
    ullRate = ullDiv.quot;

    return  ((ULONG)ullRate);
}
/*********************************************************************************************************
** ��������: __clockFixedFactorRateRound
** ��������: �����ӽ�Ԥ��Ƶ�ʵ�Ƶ��ֵ
** �䡡��  : pclk             ʱ���豸
**           ulRate           Ԥ���õ�Ƶ��ֵ
**           pulParentRate    ��ʱ�ӵ�Ƶ��ֵ
** �䡡��  : ��ӽ�Ԥ��Ƶ�ʵ�Ƶ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __clockFixedFactorRateRound (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulParentRate)
{
    PLW_CLOCK_FIXED_FACTOR  pclkfixedfactor = __CLK_TO_CLK_FIXED_FACTOR(pclk);
    ULONG                   ulBestParent;

    if (__HW_FLAGS_GET(pclk) & LW_CLOCK_SET_RATE_PARENT) {              /*  ͬʱ���ø�ʱ�ӵ�Ƶ��        */
        ulBestParent   = (ulRate / pclkfixedfactor->CLKFF_uiFixedMult) *
                         pclkfixedfactor->CLKFF_uiFixedDiv;
        *pulParentRate = API_ClockRateRound(__HW_PARENT_GET(pclk), ulBestParent);
    }

    return  ((*pulParentRate / pclkfixedfactor->CLKFF_uiFixedDiv) *
             pclkfixedfactor->CLKFF_uiFixedMult);
}
/*********************************************************************************************************
** ��������: __clockFixedFactorRateRound
** ��������: ����Ƶ��
** �䡡��  : pclk             ʱ���豸
**           ulRate           Ԥ���õ�Ƶ��ֵ
**           ulParentRate     ��ʱ�ӵ�Ƶ��ֵ
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __clockFixedFactorRateSet (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_ClockFixedFactorRegister
** ��������: ע��̶���Ƶ��Ƶ��ʱ��
** �䡡��  : pcName            ʱ������
**           pcParentName      ��ʱ������
**           ulFlags           ��ʼ����ʶ
**           uiFixedMult       �̶��ı�Ƶֵ
**           uiFixedDiv        �̶��ķ�Ƶֵ
** �䡡��  : �̶���Ƶ��Ƶ��ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockFixedFactorRegister (CPCHAR  pcName,
                                         PCHAR  *ppcParentName,
                                         ULONG   ulFlags,
                                         UINT    uiFixedMult,
                                         UINT    uiFixedDiv)
{
    PLW_CLOCK_FIXED_FACTOR   pclkfixedfactor;
    PLW_CLOCK                pclk;
    INT                      iRet;

    if (!pcName || !ppcParentName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkfixedfactor = __SHEAP_ZALLOC(sizeof(LW_CLOCK_FIXED_FACTOR));
    if (!pclkfixedfactor) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkfixedfactor->CLKFF_clk,
                                pcName,
                                &_G_clkopsFixedFactor,
                                ulFlags,
                                ppcParentName,
                                1);
    if (iRet) {
        __SHEAP_FREE(pclkfixedfactor);
        return  (LW_NULL);
    }

    pclk = &pclkfixedfactor->CLKFF_clk;
    pclkfixedfactor->CLKFF_uiFixedMult = uiFixedMult;
    pclkfixedfactor->CLKFF_uiFixedDiv  = uiFixedDiv;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkfixedfactor);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
