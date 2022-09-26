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
** ��   ��   ��: clockFixedRate.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 26 ��
**
** ��        ��: ��׼�̶�Ƶ��ʱ���������� Oscillator �� Crystal��.
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
static ULONG  __clockFixedRateRecalc(PLW_CLOCK  pclk, ULONG  ulParentRate);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsFixedRate = {
    .clockRateRecalc = __clockFixedRateRecalc,
};
/*********************************************************************************************************
** ��������: __clockFixedRateRecalc
** ��������: ��ù̶�Ƶ��ʱ�ӵ�Ƶ��ֵ
** �䡡��  : pclk             ʱ���豸
**           ulParentRate     ��ʱ��Ƶ�ʣ���ʱ����
** �䡡��  : �̶�Ƶ��ʱ�ӵ�Ƶ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __clockFixedRateRecalc (PLW_CLOCK  pclk, ULONG  ulParentRate)
{
    return  (__CLK_TO_CLK_FIXED_RATE(pclk)->CLKFR_ulFixedRate);
}
/*********************************************************************************************************
** ��������: API_ClockFixedRateRegister
** ��������: ע��һ���̶�Ƶ�ʵ�ʱ��
** �䡡��  : pcName            ʱ������
**           ulFlags           ʱ�ӱ�ʶ
**           ulFixedRate       �̶���Ƶ��
** �䡡��  : ע��Ĺ̶�Ƶ��ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockFixedRateRegister (CPCHAR  pcName,
                                       ULONG   ulFlags,
                                       ULONG   ulFixedRate)
{
    PLW_CLOCK_FIXED_RATE    pclkfixedrate;
    PLW_CLOCK               pclk;
    INT                     iRet;

    if (!pcName) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkfixedrate = __SHEAP_ZALLOC(sizeof(LW_CLOCK_FIXED_RATE));
    if (!pclkfixedrate) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkfixedrate->CLKFR_clk,
                                pcName,
                                &_G_clkopsFixedRate,
                                ulFlags,
                                LW_NULL,
                                0);
    if (iRet) {
        __SHEAP_FREE(pclkfixedrate);
        return  (LW_NULL);
    }

    pclk = &pclkfixedrate->CLKFR_clk;
    pclkfixedrate->CLKFR_ulFixedRate = ulFixedRate;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkfixedrate);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
