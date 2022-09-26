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
** ��   ��   ��: clockDivider.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 26 ��
**
** ��        ��: �ɱ��Ƶֵ��ʱ��
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
  �궨��
*********************************************************************************************************/
#define DIV_ROUND_DOWN_ULL(ll, d)     \
    ({                                \
         lib_lldiv_t     ullDiv;      \
         ULONG           ullTmp;      \
         ullDiv  = lib_lldiv(ll, d);  \
         ullTmp  = ullDiv.quot;       \
         ullTmp;                      \
    })
#define DIV_ROUND_UP_ULL(ll, d)     DIV_ROUND_DOWN_ULL((ll) + (d) - 1, (d))
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static ULONG  __clockDividerRateRecalc(PLW_CLOCK  pclk, ULONG  ulParentRate);
static LONG   __clockDividerRateRound(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulRate);
static INT    __clockDividerRateSet(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate);
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static LW_CLOCK_OPS  _G_clkopsDivider = {
    .clockRateRecalc = __clockDividerRateRecalc,
    .clockRateRound  = __clockDividerRateRound,
    .clockRateSet    = __clockDividerRateSet,
};
/*********************************************************************************************************
** ��������: __clockIsBestDiv
** ��������: �ж��Ƿ�������ʵķ�Ƶֵ
** �䡡��  : ulRate           Ԥ�ڷ�Ƶ��ֵ
**           ulNow            ��ǰ��Ƶ��ֵ
**           ulBest           ��ǰ��ѷ�Ƶ��ֵ
**           ulFlags          ʱ�ӱ�־
** �䡡��  : LW_TRUE Ϊ��ѷ�Ƶֵ��LW_FALSE ��Ϊ��ѷ�Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL __clockIsBestDiv (ULONG  ulRate, ULONG  ulNow, ULONG  ulBest, ULONG  ulFlags)
{
    if (ulFlags & LW_CLOCK_DIVIDER_ROUND_CLOSEST) {                     /*  ���������ӽ��ķ�Ƶֵ      */
        return  (lib_labs(ulRate - ulNow) < lib_labs(ulRate - ulBest));
    }

    return  (ulNow <= ulRate) && (ulNow > ulBest);
}
/*********************************************************************************************************
** ��������: __clockIsValidDiv
** ��������: �ж��Ƿ�����Ч�ķ�Ƶֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           uiDiv            ��Ƶֵ
** �䡡��  : LW_TRUE Ϊ��Ч��Ƶֵ��LW_FALSE ��Ϊ��Ч��Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL __clockIsValidDiv (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {
            return  (LW_TRUE);
        }
    }

    return  (LW_FALSE);
}
/*********************************************************************************************************
** ��������: __clockTableMinDivGet
** ��������: ��ȡ������С��Ƶֵ
** �䡡��  : pclkdivtable        ��Ƶ��
** �䡡��  : ����Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableMinDivGet (PLW_CLOCK_DIV_TABLE  pclkdivtable)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiMinDiv = UINT_MAX;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv < uiMinDiv) {
            uiMinDiv = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiMinDiv);
}
/*********************************************************************************************************
** ��������: __clockTableDivRoundUpGet
** ��������: ��ñ�����ӽ�Ԥ�ڷ�Ƶֵ�Ҵ���Ԥ�ڷ�Ƶֵ�ķ�Ƶֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           uiDiv            Ԥ�ڷ�Ƶֵ
** �䡡��  : ��ӽ�Ԥ�ڷ�Ƶֵ�Ҵ���Ԥ�ڷ�Ƶֵ�ķ�Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableDivRoundUpGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiUp = UINT_MAX;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {                        /*  �ҵ���ƥ��ķ�Ƶֵ          */
            return  (pclkdivitem->CLKDT_uiDiv);

        } else if (pclkdivitem->CLKDT_uiDiv < uiDiv) {                  /*  �����Ƶֵ��Ԥ�ڷ�ƵֵС    */
            continue;
        }

        if ((pclkdivitem->CLKDT_uiDiv - uiDiv) < (uiUp - uiDiv)) {      /*  ����ֵ��Ԥ��ֵ����С      */
            uiUp = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiUp);
}
/*********************************************************************************************************
** ��������: __clockTableDivRoundDownGet
** ��������: ��ñ�����ӽ�Ԥ�ڷ�Ƶֵ��С��Ԥ�ڷ�Ƶֵ�ķ�Ƶֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           uiDiv            Ԥ�ڷ�Ƶֵ
** �䡡��  : ��ӽ�Ԥ�ڷ�Ƶֵ��С��Ԥ�ڷ�Ƶֵ�ķ�Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableDivRoundDownGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiDown = __clockTableMinDivGet(pclkdivtable);

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {                        /*  �ҵ���ƥ��ķ�Ƶֵ          */
            return  (pclkdivitem->CLKDT_uiDiv);

        } else if (pclkdivitem->CLKDT_uiDiv > uiDiv) {                  /*  �����Ƶֵ��Ԥ�ڷ�Ƶֵ��    */
            continue;
        }

        if ((uiDiv - pclkdivitem->CLKDT_uiDiv) < (uiDiv - uiDown)) {    /*  ����ֵ��Ԥ��ֵ����С      */
            uiDown = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiDown);
}
/*********************************************************************************************************
** ��������: __clockDivRoundClosest
** ��������: ��ñ�����ӽ�Ԥ�ڷ�Ƶֵ�ķ�Ƶֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           ulParentRate     ��ʱ��Ƶ��
**           ulRate           Ԥ�ڷ�Ƶֵ
** �䡡��  : ��ӽ�Ԥ�ڷ�Ƶֵ�ķ�Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockDivRoundClosest (PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                     ULONG                ulParentRate,
                                     ULONG                ulRate)
{
    ULONG  ulUpRate;
    ULONG  ulDownRate;
    UINT   uiDivUp;
    UINT   uiDivDown;

    uiDivUp    = DIV_ROUND_UP_ULL((UINT64)ulParentRate, ulRate);        /*  ��Ƶֵ���Ͻ�                */
    uiDivDown  = ulParentRate / ulRate;                                 /*  ��Ƶֵ���½�                */

    uiDivUp    = __clockTableDivRoundUpGet(pclkdivtable, uiDivUp);
    uiDivDown  = __clockTableDivRoundDownGet(pclkdivtable, uiDivDown);

    ulUpRate   = DIV_ROUND_UP_ULL((UINT64)ulParentRate, uiDivUp);
    ulDownRate = DIV_ROUND_UP_ULL((UINT64)ulParentRate, uiDivDown);

    return  ((ulRate - ulUpRate) <= (ulDownRate - ulRate) ? uiDivUp : uiDivDown);
}
/*********************************************************************************************************
** ��������: __clockDivRound
** ��������: �����ӽ��ķ�Ƶֵ
** �䡡��  : pclkdivtable        ��Ƶ��
**           ulParentRate     ��ʱ��Ƶ��
**           ulRate           ��ʱ��Ƶ��
**           ulFlags          ʱ�ӱ�־
** �䡡��  : ��ӽ��ķ�Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockDivRound (PLW_CLOCK_DIV_TABLE  pclkdivtable,
                              ULONG                ulParentRate,
                              ULONG                ulRate,
                              ULONG                ulFlags)
{
    UINT  uiDiv;

    if (ulFlags & LW_CLOCK_DIVIDER_ROUND_CLOSEST) {                     /*  ���������ӽ��ķ�Ƶֵ      */
        return  (__clockDivRoundClosest(pclkdivtable, ulParentRate, ulRate));
    }

    uiDiv = DIV_ROUND_UP_ULL((UINT64)ulParentRate, ulRate);

    return  (__clockTableDivRoundUpGet(pclkdivtable, uiDiv));           /*  �����Դ��ڵķ�Ƶֵ          */
}
/*********************************************************************************************************
** ��������: __clockTableValGet
** ��������: ͨ����Ƶֵ��ȡ�Ĵ�����ֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           uiDiv            ��Ƶֵ
** �䡡��  : �Ĵ�����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableValGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiDiv)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv == uiDiv) {
            return  (pclkdivitem->CLKDT_uiVal);
        }
    }

    return  (0);
}
/*********************************************************************************************************
** ��������: __clockTableDivGet
** ��������: ͨ���Ĵ�����ֵ��ȡ��Ƶֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           uiVal            �Ĵ�����ֵ
** �䡡��  : ��Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableDivGet (PLW_CLOCK_DIV_TABLE  pclkdivtable, UINT  uiVal)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiVal == uiVal) {
            return  (pclkdivitem->CLKDT_uiDiv);
        }
    }

    return  (1);
}
/*********************************************************************************************************
** ��������: __clockTableMaxDivGet
** ��������: ��ȡ��������Ƶֵ
** �䡡��  : pclkdivtable        ��Ƶ��
** �䡡��  : ����Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableMaxDivGet (PLW_CLOCK_DIV_TABLE  pclkdivtable)
{
    PLW_CLOCK_DIV_TABLE  pclkdivitem;
    UINT                 uiMaxDiv = 0;

    for (pclkdivitem = pclkdivtable; pclkdivitem->CLKDT_uiDiv; pclkdivitem++) {
        if (pclkdivitem->CLKDT_uiDiv > uiMaxDiv) {
            uiMaxDiv = pclkdivitem->CLKDT_uiDiv;
        }
    }

    return  (uiMaxDiv);
}
/*********************************************************************************************************
** ��������: __clockTableBestDivGet
** ��������: �������ʵķ�Ƶֵ
** �䡡��  : pclk               ʱ���豸
**           pclkParent         ��ʱ���豸
**           ulRate             ʱ��Ƶ��
**           pulBestParentRate  ���ʺϵĸ�ʱ��Ƶ��
**           pclkdivtable       ��Ƶ��
** �䡡��  : ����ʵķ�Ƶֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __clockTableBestDivGet (PLW_CLOCK            pclk,
                                     PLW_CLOCK            pclkParent,
                                     ULONG                ulRate,
                                     ULONG               *pulBestParentRate,
                                     PLW_CLOCK_DIV_TABLE  pclkdivtable)
{
    ULONG  ulParentRate;
    ULONG  ulParentRateSaved = *pulBestParentRate;
    ULONG  ulBest            = 0;
    ULONG  ulNow;
    UINT   uiMaxDiv;
    UINT   uiBestDiv         = 0;
    INT    i;

    if (!ulRate) {
        ulRate = 1;
    }

    uiMaxDiv = __clockTableMaxDivGet(pclkdivtable);

    if (!(__HW_FLAGS_GET(pclk) & LW_CLOCK_SET_RATE_PARENT)) {
        ulParentRate = *pulBestParentRate;
        uiBestDiv    = __clockDivRound(pclkdivtable,
                                       ulParentRate,
                                       ulRate,
                                       __HW_FLAGS_GET(pclk));
        uiBestDiv    = uiBestDiv > uiMaxDiv ? uiMaxDiv : uiBestDiv;
        return  (uiBestDiv);
    }

    uiMaxDiv = __MIN(ULONG_MAX / ulRate, uiMaxDiv);

    for (i  = __clockTableDivRoundUpGet(pclkdivtable, 1);
         i <= uiMaxDiv;
         i  = __clockTableDivRoundUpGet(pclkdivtable, i)) {

         if (ulRate * i == ulParentRateSaved) {
             *pulBestParentRate = ulParentRateSaved;
             return  (i);
         }

         ulParentRate = API_ClockRateRound(pclk, ulRate * i);
         ulNow        = DIV_ROUND_UP_ULL((UINT64)ulParentRate, i);
         if (__clockIsBestDiv(ulRate, ulNow, ulBest, __HW_FLAGS_GET(pclk))) {
             uiBestDiv         = i;
             ulBest            = ulNow;
            *pulBestParentRate = ulParentRate;
         }
    }

    if (!uiBestDiv) {
        uiBestDiv         = __clockTableMaxDivGet(pclkdivtable);
       *pulBestParentRate = API_ClockRateRound(pclkParent, ulRate);
    }

    return  (uiBestDiv);
}
/*********************************************************************************************************
** ��������: __clockDividerValGet
** ��������: ��ö�ӦƵ�ʵļĴ�����ֵ
** �䡡��  : pclkdivtable     ��Ƶ��
**           ulParentRate     ��ʱ��Ƶ��
**           ulRate           ��ʱ��Ƶ��
** �䡡��  : �Ĵ�����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __clockDividerValGet (PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                  ULONG                ulParentRate,
                                  ULONG                ulRate)
{
    UINT  uiDiv;
    UINT  uiValue;

    uiDiv = DIV_ROUND_UP_ULL((UINT64)ulParentRate, ulRate);

    if (!__clockIsValidDiv(pclkdivtable, uiDiv)) {                      /*  �ж��Ƿ�����Ч��Ƶֵ        */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    uiValue = __clockTableValGet(pclkdivtable, uiDiv);

    return  (uiValue);
}
/*********************************************************************************************************
** ��������: __clockDividerRateRecalc
** ��������: ��÷�Ƶ��Ƶʱ�ӵ�Ƶ��ֵ
** �䡡��  : pclk             ʱ���豸
**           ulParentRate     ��ʱ��Ƶ��
** �䡡��  : ʱ�ӵ�Ƶ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ULONG  __clockDividerRateRecalc (PLW_CLOCK  pclk, ULONG  ulParentRate)
{
    PLW_CLOCK_DIVIDER  pclkdivider = __CLK_TO_CLK_DIVIDER(pclk);
    UINT32             uiRegVal;
    UINT32             uiDiv;

    uiRegVal  = pclkdivider->CLKD_pfuncValGet(pclk);
    uiRegVal  = uiRegVal >> pclkdivider->CLKD_uiShift;
    uiRegVal &= pclkdivider->CLKD_uiMask;
    uiDiv     = __clockTableDivGet(pclkdivider->CLKD_pclkdivtable, uiRegVal);

    return  (DIV_ROUND_UP_ULL((UINT64)ulParentRate, uiDiv));
}
/*********************************************************************************************************
** ��������: __clockDividerRateSet
** ��������: ����Ƶ��
** �䡡��  : pclk             ʱ���豸
**           ulRate           Ԥ���õ�Ƶ��ֵ
**           ulParentRate     ��ʱ�ӵ�Ƶ��ֵ
** �䡡��  : ERROR_NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __clockDividerRateSet (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate)
{
    PLW_CLOCK_DIVIDER  pclkdivider  = __CLK_TO_CLK_DIVIDER(pclk);
    UINT               uiRegVal;
    INT                iVal;

    iVal = __clockDividerValGet(pclkdivider->CLKD_pclkdivtable, ulParentRate, ulRate);
    if (iVal < 0) {
        return  (iVal);
    }

    uiRegVal  = pclkdivider->CLKD_pfuncValGet(pclk);
    uiRegVal &= ~((pclkdivider->CLKD_uiMask) << (pclkdivider->CLKD_uiShift));
    uiRegVal |= iVal << pclkdivider->CLKD_uiShift;

    return  (pclkdivider->CLKD_pfuncValSet(pclk, uiRegVal));
}
/*********************************************************************************************************
** ��������: __clockDividerRateRound
** ��������: �����ӽ�Ԥ��Ƶ�ʵ�Ƶ��ֵ
** �䡡��  : pclk             ʱ���豸
**           ulRate           Ԥ���õ�Ƶ��ֵ
**           pulParentRate    ��ʱ�ӵ�Ƶ��ֵ
** �䡡��  : ��ӽ�Ԥ��Ƶ�ʵ�Ƶ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __clockDividerRateRound (PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulParentRate)
{
    PLW_CLOCK_DIVIDER  pclkdivider = __CLK_TO_CLK_DIVIDER(pclk);
    PLW_CLOCK          pclkParent  = pclk->CLK_clkparent;
    UINT32             uiDiv;

    uiDiv = __clockTableBestDivGet(pclk,
                                   pclkParent,
                                   ulRate,
                                   pulParentRate,
                                   pclkdivider->CLKD_pclkdivtable);

    return  (DIV_ROUND_UP_ULL((UINT64)*pulParentRate, uiDiv));
}
/*********************************************************************************************************
** ��������: API_ClockDividerRegister
** ��������: ע��ɱ��Ƶ��ʱ��
** �䡡��  : pcName            ʱ������
**           ppcParentName     ��ʱ������
**           ulFlags           ��ʼ����ʶ
**           pclkdivtable      ��Ƶ��
**           uiMask            �Ĵ�������
**           uiShift           �Ĵ�����λ
**           pfuncValGet       �Ĵ���ֵ��ȡ����
**           pfuncValSet       �Ĵ���ֵ���ú���
** �䡡��  : �ɱ��Ƶ��ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_ClockDividerRegister (CPCHAR               pcName,
                                     CHAR               **ppcParentName,
                                     ULONG                ulFlags,
                                     PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                     UINT                 uiMask,
                                     UINT                 uiShift,
                                     UINTFUNCPTR          pfuncValGet,
                                     FUNCPTR              pfuncValSet)
{
    PLW_CLOCK_DIVIDER   pclkdivider;
    PLW_CLOCK           pclk;
    INT                 iRet;

    if (!pcName || !ppcParentName || !pclkdivtable || !pfuncValGet || !pfuncValSet) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    pclkdivider = __SHEAP_ZALLOC(sizeof(LW_CLOCK_DIVIDER));
    if (!pclkdivider) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    iRet = API_ClockInitDataSet(&pclkdivider->CLKD_clk,
                                pcName,
                                &_G_clkopsDivider,
                                ulFlags,
                                ppcParentName,
                                1);
    if (iRet) {
        __SHEAP_FREE(pclkdivider);
        return  (LW_NULL);
    }

    pclk = &pclkdivider->CLKD_clk;
    pclkdivider->CLKD_pclkdivtable = pclkdivtable;
    pclkdivider->CLKD_uiMask       = uiMask;
    pclkdivider->CLKD_uiShift      = uiShift;
    pclkdivider->CLKD_pfuncValGet  = pfuncValGet;
    pclkdivider->CLKD_pfuncValSet  = pfuncValSet;

    iRet = API_ClockRegister(pclk);
    if (iRet) {
        __SHEAP_FREE(pclkdivider);
        return  (LW_NULL);
    }

    return  (pclk);
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
