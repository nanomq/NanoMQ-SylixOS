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
** ��   ��   ��: cskyL2.c
**
** ��   ��   ��: Zhou.Zhijie (��־��)
**
** �ļ���������: 2020 �� 08 �� 21 ��
**
** ��        ��: C-SKY ��ϵ���� L2 CACHE ����
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_CSKY_CACHE_L2 > 0
#include "cskyL2.h"
/*********************************************************************************************************
  L2 CACHE �� (��˹���һ�� L2 CACHE, ���Բ���ʱ��Ҫ��������, ��������Ѿ����ж�, ����ֻ��������������)
*********************************************************************************************************/
static  LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(l2slca);
#define L2_OP_ENTER()   LW_SPIN_LOCK_IGNIRQ(&l2slca.SLCA_sl)
#define L2_OP_EXIT()    LW_SPIN_UNLOCK_IGNIRQ(&l2slca.SLCA_sl)
/*********************************************************************************************************
  L2 CACHE ����
*********************************************************************************************************/
static L2C_DRVIER       l2cdrv;
/*********************************************************************************************************
  L2 CACHE ��������ʼ������
*********************************************************************************************************/
extern VOID     cskyL2CK860Init(L2C_DRVIER  *pl2cdrv,
                                CACHE_MODE   uiInstruction,
                                CACHE_MODE   uiData,
                                CPCHAR       pcMachineName);
/*********************************************************************************************************
** ��������: cskyL2Enable
** ��������: ʹ�� L2 CACHE 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2Enable (VOID)
{
    if (l2cdrv.L2CD_pfuncEnable) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncEnable(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: cskyL2Disable
** ��������: ���� L2 CACHE 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2Disable (VOID)
{
#if LW_CFG_SMP_EN > 0
    if (l2cdrv.L2CD_pfuncDisable) {
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
    }

#else
    if (l2cdrv.L2CD_pfuncDisable) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
        L2_OP_EXIT();
    }
#endif
}
/*********************************************************************************************************
** ��������: cskyL2IsEnable
** ��������: L2 CACHE �Ƿ��
** �䡡��  : NONE
** �䡡��  : L2 CACHE �Ƿ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL cskyL2IsEnable (VOID)
{
    BOOL    bIsEnable;

    if (l2cdrv.L2CD_pfuncIsEnable) {
        L2_OP_ENTER();
        bIsEnable = l2cdrv.L2CD_pfuncIsEnable(&l2cdrv);
        L2_OP_EXIT();

    } else {
        bIsEnable = LW_FALSE;
    }
    
    return  (bIsEnable);
}
/*********************************************************************************************************
** ��������: cskyL2Sync
** ��������: L2 CACHE ͬ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2Sync (VOID)
{
    if (l2cdrv.L2CD_pfuncSync) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncSync(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: cskyL2FlushAll
** ��������: L2 CACHE ��д����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2FlushAll (VOID)
{
    if (l2cdrv.L2CD_pfuncFlushAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncFlushAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: cskyL2Flush
** ��������: L2 CACHE ��д����������
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT cskyL2Flush (PVOID  pvPdrs, size_t  stBytes)
{
    INT  iRet = ERROR_NONE;

    if (l2cdrv.L2CD_pfuncFlush) {
        L2_OP_ENTER();
        iRet = l2cdrv.L2CD_pfuncFlush(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: cskyL2InvalidateAll
** ��������: L2 CACHE ��Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2InvalidateAll (VOID)
{
    if (l2cdrv.L2CD_pfuncInvalidateAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncInvalidateAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: cskyL2Invalidate
** ��������: L2 CACHE ��Ч
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT cskyL2Invalidate (PVOID  pvPdrs, size_t  stBytes)
{
    INT  iRet = ERROR_NONE;

    if (l2cdrv.L2CD_pfuncInvalidate) {
        L2_OP_ENTER();
        iRet = l2cdrv.L2CD_pfuncInvalidate(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: cskyL2ClearAll
** ��������: L2 CACHE ��д����Ч
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2ClearAll (VOID)
{
    if (l2cdrv.L2CD_pfuncClearAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncClearAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** ��������: cskyL2Clear
** ��������: L2 CACHE ��д����Ч
** �䡡��  : pvPdrs        ��ʼ�����ַ
**           stBytes       ���ݿ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT cskyL2Clear (PVOID  pvPdrs, size_t  stBytes)
{
    INT  iRet = ERROR_NONE;

    if (l2cdrv.L2CD_pfuncClear) {
        L2_OP_ENTER();
        iRet = l2cdrv.L2CD_pfuncClear(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: cskyL2Name
** ��������: ��� L2 CACHE ����������
** �䡡��  : NONE
** �䡡��  : L2 CACHE ����������
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
CPCHAR  cskyL2Name (VOID)
{
    return  (l2cdrv.L2CD_pcName);
}
/*********************************************************************************************************
** ��������: cskyL2Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID cskyL2Init (CACHE_MODE   uiInstruction,
                 CACHE_MODE   uiData,
                 CPCHAR       pcMachineName)
{
    UINT32  uiWays;
    UINT32  uiWaySize;

    LW_SPIN_INIT(&l2slca.SLCA_sl);
    
    if (lib_strcmp(pcMachineName, CSKY_MACHINE_860) == 0) {             /*  CK860 ������ L2 CACHE       */
        uiWays             = 8;
        uiWaySize          = 32;
        
        l2cdrv.L2CD_pcName = CSKY_MACHINE_860;
        l2cdrv.L2CD_stSize = uiWays * uiWaySize * LW_CFG_KB_SIZE;       /* 256KB �� 8 ·������ L2 CACHE */

        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        cskyL2CK860Init(&l2cdrv, uiInstruction, uiData, pcMachineName);
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }


}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_CSKY_CACHE_L2 > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
