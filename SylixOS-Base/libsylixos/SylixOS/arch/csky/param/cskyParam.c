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
** ��   ��   ��: cskyParam.c
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2018 �� 05 �� 11 ��
**
** ��        ��: C-SKY ��ϵ�ܹ���������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "cskyParam.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static CSKY_PARAM    cskyParam = { LW_TRUE, LW_TRUE, LW_TRUE, LW_TRUE };
/*********************************************************************************************************
** ��������: archKernelParam
** ��������: C-SKY ��ϵ�ܹ�������������.
** �䡡��  : pcParam       ��������
**                         unalign=yes      �Ƿ�֧�ַǶ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  archKernelParam (CPCHAR  pcParam)
{
    if (lib_strncmp(pcParam, "unalign=", 8) == 0) {                     /*  �Ƿ�֧�ַǶ������          */
        if (pcParam[8] == 'n') {
            cskyParam.CP_bUnalign = LW_FALSE;
        } else {
            cskyParam.CP_bUnalign = LW_TRUE;
        }

    } else if (lib_strncmp(pcParam, "mmuenbyboot=", 12) == 0) {         /*  BOOT �Ƿ��Ѿ������� MMU     */
        if (pcParam[12] == 'n') {
            cskyParam.CP_bMmuEnByBoot = LW_FALSE;
        } else {
            cskyParam.CP_bMmuEnByBoot = LW_TRUE;
        }
#if LW_CFG_SMP_EN > 0
    } else if (lib_strncmp(pcParam, "sldepcache=", 11) == 0) {          /*  �������Ƿ����� CACHE        */
        if (pcParam[11] == 'n') {
            cskyParam.CP_bSLDepCache = LW_FALSE;
        } else {
            cskyParam.CP_bSLDepCache = LW_TRUE;
            __ARCH_SPIN_BYPASS();
        }
#endif                                                                  /*  LW_CFG_SMP_EN > 0           */
    } else if (lib_strncmp(pcParam, "agpr=", 5) == 0) {                 /*  Ӳ���Ƿ���ڿ�ѡͨ�üĴ���  */
        if (pcParam[5] == 'n') {
            cskyParam.CP_bAPGR = LW_FALSE;
            KN_FIQ_AUTO_ENTRY_SET();
        } else {
            cskyParam.CP_bAPGR = LW_TRUE;
        }
    }
}
/*********************************************************************************************************
** ��������: archKernelParamGet
** ��������: ��ȡ��������.
** �䡡��  : NONE
** �䡡��  : ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
CSKY_PARAM  *archKernelParamGet (VOID)
{
    return  (&cskyParam);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
