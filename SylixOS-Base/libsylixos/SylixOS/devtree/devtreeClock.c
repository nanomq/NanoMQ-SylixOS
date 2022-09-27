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
** ��   ��   ��: devtreeClock.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 09 �� 02 ��
**
** ��        ��: �豸���ӿ�ʱ����ؽӿ�ʵ��
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
#include "driver/fdt/libfdt_env.h"
#include "driver/fdt/libfdt.h"
#include "driver/fdt/fdt.h"
/*********************************************************************************************************
** ��������: API_DeviceTreeClockGetByName
** ��������: ͨ��ʱ������ȡʱ�ӽṹ, ��� pcName Ϊ LW_NULL, �����׸�ʱ��.
** �䡡��  : pdtnDev     �豸���ڵ�
**           pcName      ʱ������
** �䡡��  : ʱ�ӽṹ
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_DeviceTreeClockGetByName (PLW_DEVTREE_NODE  pdtnDev, CPCHAR  pcName)
{
    PLW_CLOCK  pclk   = LW_NULL;
    INT        iIndex;

    while (pdtnDev) {
        iIndex = 0;                                                     /*  ѭ������ÿ����Ҫ��������    */

        if (pcName) {
            iIndex = API_DeviceTreePropertyStringMatch(pdtnDev, "clock-names", pcName);
        }

        pclk = API_DeviceTreeClockGet(pdtnDev, iIndex);
        if (pclk) {
            return  (pclk);
        }

        if (pcName && (iIndex >= 0)) {
            break;
        }

        pdtnDev = pdtnDev->DTN_pdtnparent;
        if (pdtnDev && !API_DeviceTreePropertyGet(pdtnDev, "clock-ranges", LW_NULL)) {
            break;
        }
    }

    return  (pclk);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeParentClockNameGet
** ��������: ��ȡָ����ŵĸ�ʱ������
** �䡡��  : pdtnDev     �豸���ڵ�
**           iIndex      ָ����ʱ�����
** �䡡��  : ��ʱ������
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
CPCHAR  API_DeviceTreeParentClockNameGet (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex)
{
    LW_DEVTREE_PHANDLE_ARGS   pdtpaClkSpec;
    PLW_DEVTREE_PROPERTY      pdtproperty;
    PLW_CLOCK                 pclk;
    CPCHAR                    pcClkName;
    UINT                     *puiVal;
    UINT32                    uiPropVal;
    INT                       iRet;
    INT                       iCount;

    iRet = API_DeviceTreePhandleParseWithArgs(pdtnDev,
                                              "clocks",
                                              "#clock-cells",
                                              iIndex,
                                              &pdtpaClkSpec);
    if (iRet) {
        return  (LW_NULL);
    }

    iIndex = pdtpaClkSpec.DTPH_iArgsCount ? pdtpaClkSpec.DTPH_uiArgs[0] : 0;
    iCount = 0;

    _LIST_EACH_OF_UINT32_PROPERTY(pdtpaClkSpec.DTPH_pdtnDev,
                                  "clock-indices",
                                  pdtproperty,
                                  puiVal,
                                  uiPropVal) {
        if (iIndex == uiPropVal) {
            iIndex = iCount;
            break;
        }
        iCount++;
    }

    if (pdtproperty && !puiVal) {
        return  (LW_NULL);
    }

    if (API_DeviceTreePropertyStringIndexRead(pdtpaClkSpec.DTPH_pdtnDev,
                                              "clock-output-names",
                                              iIndex,
                                              &pcClkName) < 0) {
        pclk = API_ClockGetFromProvider(&pdtpaClkSpec);
        if (!pclk) {
            if (pdtpaClkSpec.DTPH_iArgsCount == 0) {
                pcClkName = pdtpaClkSpec.DTPH_pdtnDev->DTN_pcName;
            } else {
                pcClkName = LW_NULL;
            }
        } else {
            pcClkName = pclk->CLK_pcName;
        }
    }

    return  (pcClkName);
}
/*********************************************************************************************************
** ��������: API_DeviceTreeClockGet
** ��������: ͨ���豸���ڵ��ȡʱ��
** �䡡��  : pdtnDev        �豸���ڵ�
**           iIndex         �豸���
** �䡡��  : ��ȡ��ʱ��
** ȫ�ֱ���:
** ����ģ��:
**                                            API ����
*********************************************************************************************************/
LW_API
PLW_CLOCK  API_DeviceTreeClockGet (PLW_DEVTREE_NODE  pdtnDev, INT  iIndex)
{
    LW_DEVTREE_PHANDLE_ARGS   pdtpaClkSpec;
    PLW_CLOCK                 pclk;
    INT                       iRet;

    iRet = API_DeviceTreePhandleParseWithArgs(pdtnDev,
                                              "clocks",
                                              "#clock-cells",
                                              iIndex,
                                              &pdtpaClkSpec);
    if (iRet) {
        return  (LW_NULL);
    }

    pclk = API_ClockGetFromProvider(&pdtpaClkSpec);

    return  (pclk);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
