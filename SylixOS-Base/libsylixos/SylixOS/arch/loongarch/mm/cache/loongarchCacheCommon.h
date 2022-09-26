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
** ��   ��   ��: loongarchCacheCommon.h
**
** ��   ��   ��: Qin.Fei (�ط�)
**
** �ļ���������: 2022 �� 03 �� 15 ��
**
** ��        ��: LoongArch ��ϵ���� CACHE ����.
*********************************************************************************************************/

#ifndef __LOONGARCHCACHECOMMON_H
#define __LOONGARCHCACHECOMMON_H

/*********************************************************************************************************
  CACHE ��Ϣ
*********************************************************************************************************/
typedef struct {
    BOOL        CACHE_bPresent;                                         /*  �Ƿ���� CACHE              */
    UINT32      CACHE_uiSize;                                           /*  CACHE ��С                  */
    UINT32      CACHE_uiWaySize;                                        /*  ·��С                      */
    UINT32      CACHE_uiSets;                                           /*  ÿһ· CACHE ����           */
    UINT32      CACHE_uiWays;                                           /*  ·��                        */
    UINT32      CACHE_uiLineSize;                                       /*  CACHE ���ֽ���              */
    UINT32      CACHE_uiWayBit;                                         /*  ����ѡ·ƫ��                */
    UINT32      CACHE_uiFlags;                                          /*  CACHE ���Ա�־              */

#define LOONGARCH_CACHE_NOT_PRESENT 0x00000001
#define LOONGARCH_CACHE_VTAG        0x00000002                          /*  Virtually tagged cache      */
#define LOONGARCH_CACHE_ALIASES     0x00000004                          /*  Cache could have aliases    */
#define LOONGARCH_CACHE_IC_F_DC     0x00000008                          /*  Ic can refill from D-cache  */
#define LOONGARCH_IC_SNOOPS_REMOTE  0x00000010                          /*  Ic snoops remote stores     */
#define LOONGARCH_CACHE_PINDEX      0x00000020                          /*  Physically indexed cache    */
} LOONGARCH_CACHE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
extern LOONGARCH_CACHE   _G_ICache, _G_DCache;                          /*  ICACHE �� DCACHE ��Ϣ       */
extern LOONGARCH_CACHE   _G_VCache, _G_SCache;                          /*  VCACHE �� SCACHE ��Ϣ       */

VOID  loongarchCacheInfoShow(VOID);
VOID  loongarchCacheProbe(CPCHAR  pcMachineName);

#endif                                                                  /*  __LOONGARCHCACHECOMMON_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
