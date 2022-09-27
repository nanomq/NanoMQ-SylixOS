/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: loongarchMmuCommon.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 03 月 04 日
**
** 描        述: LoongArch 体系构架 MMU 通用接口.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHMMUCOMMON_H
#define __ARCH_LOONGARCHMMUCOMMON_H

/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
extern ULONG             _G_ulVmmPgdShift;                              /*  PGD 基址偏移                */
extern ULONG             _G_ulVmmPmdShift;                              /*  PMD 基址偏移                */
extern ULONG             _G_ulVmmPtsShift;                              /*  PTS 基址偏移                */
extern ULONG             _G_ulVmmPteShift;                              /*  PTE 基址偏移                */
extern ULONG             _G_ulVmmPgdBlkSize;                            /*  PGD 内存池块大小            */
extern ULONG             _G_ulVmmPmdBlkSize;                            /*  PMD 内存池块大小            */
extern ULONG             _G_ulVmmPtsBlkSize;                            /*  PTS 内存池块大小            */
extern ULONG             _G_ulVmmPteBlkSize;                            /*  PTE 内存池块大小            */
extern ULONG             _G_ulVmmPgdMask;                               /*  PGD 内存池掩码              */
extern ULONG             _G_ulVmmPmdMask;                               /*  PMD 内存池掩码              */
extern ULONG             _G_ulVmmPtsMask;                               /*  PTS 内存池掩码              */
extern ULONG             _G_ulVmmPteMask;                               /*  PTE 内存池掩码              */
extern ULONG             _G_ulVmmPaLen;                                 /*  处理器物理地址长度          */
extern ULONG             _G_ulEntryLoPenMask;                           /*  处理器页表项物理地址掩码    */
extern ULONG             _G_ulEntryLoPenShift;                          /*  处理器页表项物理地址偏移    */
/*********************************************************************************************************
  PAGE 掩码
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
  函数声明
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
