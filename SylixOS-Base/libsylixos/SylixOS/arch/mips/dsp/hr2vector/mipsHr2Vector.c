/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: mipsHr2Vector.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 02 �� 24 ��
**
** ��        ��: ��� 2 �Ŵ������������㵥Ԫ֧��.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�֧��
*********************************************************************************************************/
#if (defined(_MIPS_ARCH_HR2) || defined(_MIPS_ARCH_HCW)) && LW_CFG_CPU_DSP_EN > 0
#include "../mipsDsp.h"
#include "arch/mips/common/cp0/mipsCp0.h"
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static MIPS_DSP_OP      _G_dspopHr2Vector;
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  mipsHr2VectorInit(VOID);
extern VOID  mipsHr2VectorEnable(VOID);
extern VOID  mipsHr2VectorDisable(VOID);
extern BOOL  mipsHr2VectorIsEnable(VOID);
extern VOID  mipsHr2VectorSave(ARCH_DSP_CTX  *pdspctx);
extern VOID  mipsHr2VectorRestore(ARCH_DSP_CTX  *pdspctx);
/*********************************************************************************************************
** ��������: mipsHr2VectorCtxShow
** ��������: ��ʾ DSP ������
** �䡡��  : iFd       ����ļ�������
**           pvDspCtx  DSP ������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  mipsHr2VectorCtxShow (INT  iFd, ARCH_DSP_CTX  *pdspctx)
{
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_FIO_LIB_EN > 0)
    HR2_VECTOR_CTX  *phr2VectorCtx = &pdspctx->DSPCTX_hr2VectorCtx;
    INT              i;

    fdprintf(iFd, "VCCR = 0x%08x\n", phr2VectorCtx->HR2VECCTX_uiVccr);

    for (i = 0; i < HR2_VECTOR_REG_NR; i++) {
        fdprintf(iFd, "Z%02d = 0x%08x%08x0x%08x%08x0x%08x%08x0x%08x%08x\n", i,
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[0],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[1],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[2],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[3],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[4],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[5],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[6],
                 phr2VectorCtx->HR2VECCTX_vectorRegs[i].val32[7]);
    }
#endif
}
/*********************************************************************************************************
** ��������: mipsHr2VectorEnableTask
** ��������: ϵͳ���� DSP �������쳣ʱ, ʹ������� DSP
** �䡡��  : ptcbCur    ��ǰ������ƿ�
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  mipsHr2VectorEnableTask (PLW_CLASS_TCB  ptcbCur)
{
    ARCH_REG_CTX  *pregctx;

    pregctx = &ptcbCur->TCB_archRegCtx;
    pregctx->REG_ulCP0Status |= ST0_CU2;
}
/*********************************************************************************************************
** ��������: mipsHr2VectorPrimaryInit
** ��������: ��ȡ DSP ����������������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  : ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
PMIPS_DSP_OP  mipsHr2VectorPrimaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    mipsHr2VectorInit();

    _G_dspopHr2Vector.MDSP_pfuncEnable     = mipsHr2VectorEnable;
    _G_dspopHr2Vector.MDSP_pfuncDisable    = mipsHr2VectorDisable;
    _G_dspopHr2Vector.MDSP_pfuncIsEnable   = mipsHr2VectorIsEnable;
    _G_dspopHr2Vector.MDSP_pfuncSave       = mipsHr2VectorSave;
    _G_dspopHr2Vector.MDSP_pfuncRestore    = mipsHr2VectorRestore;
    _G_dspopHr2Vector.MDSP_pfuncCtxShow    = mipsHr2VectorCtxShow;
    _G_dspopHr2Vector.MDSP_pfuncEnableTask = mipsHr2VectorEnableTask;

    return  (&_G_dspopHr2Vector);
}
/*********************************************************************************************************
** ��������: mipsHr2VectorSecondaryInit
** ��������: ��ʼ�� DSP ������
** �䡡��  : pcMachineName ������
**           pcDspName     DSP ��������
** �䡡��  :
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  mipsHr2VectorSecondaryInit (CPCHAR  pcMachineName, CPCHAR  pcDspName)
{
    mipsHr2VectorInit();
}

#endif                                                                  /*  defined(_MIPS_ARCH_HR2)     */
                                                                        /*  LW_CFG_CPU_DSP_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
