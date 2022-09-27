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
** 文   件   名: loongarchCacheCommon.h
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2022 年 03 月 15 日
**
** 描        述: LoongArch 体系构架 CACHE 驱动.
*********************************************************************************************************/

#ifndef __LOONGARCHCACHECOMMON_H
#define __LOONGARCHCACHECOMMON_H

/*********************************************************************************************************
  CACHE 信息
*********************************************************************************************************/
typedef struct {
    BOOL        CACHE_bPresent;                                         /*  是否存在 CACHE              */
    UINT32      CACHE_uiSize;                                           /*  CACHE 大小                  */
    UINT32      CACHE_uiWaySize;                                        /*  路大小                      */
    UINT32      CACHE_uiSets;                                           /*  每一路 CACHE 行数           */
    UINT32      CACHE_uiWays;                                           /*  路数                        */
    UINT32      CACHE_uiLineSize;                                       /*  CACHE 行字节数              */
    UINT32      CACHE_uiWayBit;                                         /*  索引选路偏移                */
    UINT32      CACHE_uiFlags;                                          /*  CACHE 属性标志              */

#define LOONGARCH_CACHE_NOT_PRESENT 0x00000001
#define LOONGARCH_CACHE_VTAG        0x00000002                          /*  Virtually tagged cache      */
#define LOONGARCH_CACHE_ALIASES     0x00000004                          /*  Cache could have aliases    */
#define LOONGARCH_CACHE_IC_F_DC     0x00000008                          /*  Ic can refill from D-cache  */
#define LOONGARCH_IC_SNOOPS_REMOTE  0x00000010                          /*  Ic snoops remote stores     */
#define LOONGARCH_CACHE_PINDEX      0x00000020                          /*  Physically indexed cache    */
} LOONGARCH_CACHE;
/*********************************************************************************************************
  全局变量声明
*********************************************************************************************************/
extern LOONGARCH_CACHE   _G_ICache, _G_DCache;                          /*  ICACHE 和 DCACHE 信息       */
extern LOONGARCH_CACHE   _G_VCache, _G_SCache;                          /*  VCACHE 和 SCACHE 信息       */

VOID  loongarchCacheInfoShow(VOID);
VOID  loongarchCacheProbe(CPCHAR  pcMachineName);

#endif                                                                  /*  __LOONGARCHCACHECOMMON_H    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
