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
** ��   ��   ��: loongarchMmuCommon.h
**
** ��   ��   ��: Wang.Ziyang (������)
**
** �ļ���������: 2022 �� 03 �� 04 ��
**
** ��        ��: LoongArch ��ϵ���� MMU ͨ�ýӿ�.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHMMUCOMMON_H
#define __ARCH_LOONGARCHMMUCOMMON_H

/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern ULONG             _G_ulVmmPgdShift;                              /*  PGD ��ַƫ��                */
extern ULONG             _G_ulVmmPmdShift;                              /*  PMD ��ַƫ��                */
extern ULONG             _G_ulVmmPtsShift;                              /*  PTS ��ַƫ��                */
extern ULONG             _G_ulVmmPteShift;                              /*  PTE ��ַƫ��                */
extern ULONG             _G_ulVmmPgdBlkSize;                            /*  PGD �ڴ�ؿ��С            */
extern ULONG             _G_ulVmmPmdBlkSize;                            /*  PMD �ڴ�ؿ��С            */
extern ULONG             _G_ulVmmPtsBlkSize;                            /*  PTS �ڴ�ؿ��С            */
extern ULONG             _G_ulVmmPteBlkSize;                            /*  PTE �ڴ�ؿ��С            */
extern ULONG             _G_ulVmmPgdMask;                               /*  PGD �ڴ������              */
extern ULONG             _G_ulVmmPmdMask;                               /*  PMD �ڴ������              */
extern ULONG             _G_ulVmmPtsMask;                               /*  PTS �ڴ������              */
extern ULONG             _G_ulVmmPteMask;                               /*  PTE �ڴ������              */
extern ULONG             _G_ulVmmPaLen;                                 /*  �����������ַ����          */
extern ULONG             _G_ulEntryLoPenMask;                           /*  ������ҳ���������ַ����    */
extern ULONG             _G_ulEntryLoPenShift;                          /*  ������ҳ���������ַƫ��    */
/*********************************************************************************************************
  PAGE ����
*********************************************************************************************************/
#if   LW_CFG_VMM_PAGE_SIZE == (4  * LW_CFG_KB_SIZE)
#define LOONGARCH_MMU_PAGE_MASK              PS_4K
#elif LW_CFG_VMM_PAGE_SIZE == (16 * LW_CFG_KB_SIZE)
#define LOONGARCH_MMU_PAGE_MASK              PS_16K
#elif LW_CFG_VMM_PAGE_SIZE == (64 * LW_CFG_KB_SIZE)
#define LOONGARCH_MMU_PAGE_MASK              PS_64K
#else
#error  LW_CFG_VMM_PAGE_SIZE must be (4K, 16K, 64K)!
#endif
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
INT   loongarchTlbSizeGet(VOID);
VOID  loongarchTlbInvAll(VOID);
VOID  loongarchTlbInit(ULONG  ulPgdi, ULONG  ulPgdw,
                       ULONG  ulPmdi, ULONG  ulPmdw,
                       ULONG  ulPtsi, ULONG  ulPtsw,
                       ULONG  ulPtei, ULONG  ulPtew);

VOID  loongarchPgdAddrSet(LW_PGD_TRANSENTRY  *pgdEntry);

VOID  loongarchMmuEnable(VOID);
VOID  loongarchMmuDisable(VOID);

VOID  loongarchMmuInit(LW_MMU_OP  *pmmuop, CPCHAR  pcMachineName);

#endif                                                                  /*  __ARCH_LOONGARCHMMUCOMMON_H */
/*********************************************************************************************************
  END
*********************************************************************************************************/
