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
** ��   ��   ��: cskyL2.h
**
** ��   ��   ��: Zhou.Zhijie (��־��)
**
** �ļ���������: 2020 �� 08 �� 21 ��
**
** ��        ��: C-SKY ��ϵ���� L2 CACHE ����.
*********************************************************************************************************/

#ifndef __CSKYL2_H
#define __CSKYL2_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_CSKY_CACHE_L2 > 0

/*********************************************************************************************************
  L2 cache driver struct
*********************************************************************************************************/

typedef struct {
    CPCHAR          L2CD_pcName;                                        /*  L2 CACHE ����������         */
    size_t          L2CD_stSize;                                        /*  L2 CACHE ��С               */
    
    VOIDFUNCPTR     L2CD_pfuncEnable;
    VOIDFUNCPTR     L2CD_pfuncDisable;
    BOOLFUNCPTR     L2CD_pfuncIsEnable;
    VOIDFUNCPTR     L2CD_pfuncSync;
    FUNCPTR         L2CD_pfuncFlush;
    VOIDFUNCPTR     L2CD_pfuncFlushAll;
    FUNCPTR         L2CD_pfuncInvalidate;
    VOIDFUNCPTR     L2CD_pfuncInvalidateAll;
    FUNCPTR         L2CD_pfuncClear;
    VOIDFUNCPTR     L2CD_pfuncClearAll;
} L2C_DRVIER;

/*********************************************************************************************************
  ��ʼ��
*********************************************************************************************************/

VOID    cskyL2Init(CACHE_MODE   uiInstruction,
                   CACHE_MODE   uiData,
                   CPCHAR       pcMachineName);
CPCHAR  cskyL2Name(VOID);
VOID    cskyL2Enable(VOID);
VOID    cskyL2Disable(VOID);
BOOL    cskyL2IsEnable(VOID);
VOID    cskyL2Sync(VOID);
VOID    cskyL2FlushAll(VOID);
INT     cskyL2Flush(PVOID  pvPdrs, size_t  stBytes);
VOID    cskyL2InvalidateAll(VOID);
INT     cskyL2Invalidate(PVOID  pvPdrs, size_t  stBytes);
VOID    cskyL2ClearAll(VOID);
INT     cskyL2Clear(PVOID  pvPdrs, size_t  stBytes);

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_CSKY_CACHE_L2 > 0    */
#endif                                                                  /*  __CSKYL2_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
