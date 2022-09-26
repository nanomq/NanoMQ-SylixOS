/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: gptPartition.h
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2021 年 06 月 19 日
**
** 描        述: GPT 分区表分析.
*********************************************************************************************************/

#ifndef __GPTPARTITION_H
#define __GPTPARTITION_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKPART_EN > 0) && (LW_CFG_CPU_WORD_LENGHT > 32)
/*********************************************************************************************************
  GPT 分区相关宏定义
*********************************************************************************************************/
#define GPT_VERSION         0x00010000                                  /*  GPT 表版本号                */
#define GPT_MAGIC           "EFI PART"                                  /*  GPT 表标识                  */
#define GPT_MAX_ENTRIES     128                                         /*  最大分区数                  */
#define GPT_MAX_NAMELEN     36                                          /*  最大分区名长度              */
#define GPT_MAX_SECSIZE     4096                                        /*  支持的最大逻辑块大小        */
#define GPT_DATA_LBA        34                                          /*  数据起始逻辑块              */
#define GPT_HEAD_LBA        1                                           /*  GPT 头逻辑块号              */
#define GPT_HEAD_LBA_CNT    1                                           /*  GPT 头占用的逻辑块数量      */
#define GPT_ENT_LBA         2                                           /*  GPT 分区表起始逻辑块        */
#define GPT_ENT_LBA_CNT     32                                          /*  GPT 分区表逻辑块数量        */
/*********************************************************************************************************
  GPT 分区表头
*********************************************************************************************************/
LW_STRUCT_PACK_BEGIN
typedef struct {
    LW_STRUCT_PACK_FIELD(UINT8   HDR_ucMagic[8]);                       /*  GPT 表标识                  */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiVersion);                        /*  GPT 表版本号                */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiHeadSz);                         /*  GPT 表头大小                */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiCrc32);                          /*  GPT 表头 crc                */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiReserved);                       /*  保留字段                    */

    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64HeaderLba);                    /*  GPT 表头逻辑块号            */
    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64BackupLba);                    /*  GPT 备份表头逻辑块号        */
    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64FirstLba);                     /*  GPT 数据区起始块            */
    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64LastLba);                      /*  GPT 数据区结束块            */

    LW_STRUCT_PACK_FIELD(UINT8   HDR_ucVolumeUuid[16]);                 /*  块设备 UUID                 */

    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64Entrieslba);                   /*  GPT 分区表起始逻辑块        */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiEntriesCount);                   /*  GPT 分区表项数              */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiEntriesSize);                    /*  GPT 分区表项大小            */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiEntriesCrc32);                   /*  GPT 分区表CRC               */
} LW_STRUCT_PACK_STRUCT GPT_HEADER;
LW_STRUCT_PACK_END
/*********************************************************************************************************
  GPT 分区表项
*********************************************************************************************************/
LW_STRUCT_PACK_BEGIN
typedef struct {
    LW_STRUCT_PACK_FIELD(UINT8   ENT_ucTypeUuid[16]);                   /*  分区类型 UUID               */
    LW_STRUCT_PACK_FIELD(UINT8   ENT_ucUniqUuid[16]);                   /*  分区标识 UUID               */
    LW_STRUCT_PACK_FIELD(UINT64  ENT_ui64FirstLba);                     /*  分区起始逻辑块号            */
    LW_STRUCT_PACK_FIELD(UINT64  ENT_ui64LastLba);                      /*  分区结束逻辑块号            */
    LW_STRUCT_PACK_FIELD(UINT64  ENT_ui64Attr);                         /*  分区属性                    */
    LW_STRUCT_PACK_FIELD(UINT16  ENT_usName[GPT_MAX_NAMELEN]);          /*  分区名称                    */
} LW_STRUCT_PACK_STRUCT GPT_ENTRY;
LW_STRUCT_PACK_END
/*********************************************************************************************************
  GPT 内存结构
*********************************************************************************************************/

typedef struct {
    UINT   GPT_uiSecSize;                                               /*  扇区大小                    */
    UINT8  GPT_ucMbr[GPT_MAX_SECSIZE];                                  /*  MBR 信息                    */
    union {
        GPT_HEADER  GPT_header;                                         /*  GPT 表头                    */
        UINT8       GPT_ucBlock[GPT_MAX_SECSIZE];                       /*  GPT 表头对应扇区数据        */
    };
    GPT_ENTRY  GPT_entry[GPT_MAX_ENTRIES];                              /*  GPT 分区表项数组            */
} GPT_TABLE;

GPT_TABLE  *API_GptCreateAndInit(UINT    ulSecSize,
                                 UINT64  ui64SecCnt);                   /*  分配和初始化 GPT 内存结构   */

GPT_TABLE  *API_GptCreateAndInitWithLba(UINT    ulSecSize,
                                        UINT64  ui64SecCnt,
                                        UINT64  ui64FirstLba);          /*  分配和初始化 GPT 内存结构   */

VOID  API_GptDestroy(GPT_TABLE  *pgpt);                                 /*  销毁 GPT 内存结构           */

INT   API_GptAddEntry(GPT_TABLE  *pgpt,
                      UINT64      ui64LbaNum,
                      UINT64      ui64LbaCnt,
                      UINT8       ucPartType);                          /*  添加 GPT 表项               */

INT   API_GptAddEntryWithName(GPT_TABLE  *pgpt,
                              UINT64      ui64LbaNum,
                              UINT64      ui64LbaCnt,
                              UINT8       ucPartType,
                              CPCHAR      cpcName);                     /*  添加 GPT 表项               */

INT   API_GptGetEntry(GPT_TABLE  *pgpt,
                      INT         iIndex,
                      UINT64     *ui64LbaNum,
                      UINT64     *pui64LbaCnt,
                      UINT8      *pucPartType);                         /*  获取 GPT 表项               */

INT   API_GptPartitionSave(INT  iBlkFd, GPT_TABLE  *pgpt);              /*  保存 GPT 表到块设备         */

INT   API_GptPartitionLoad(INT  iBlkFd, GPT_TABLE  *pgpt);              /*  从块设备加载 GPT 表         */

INT   API_GptPartitionBlkLoad(PLW_BLK_DEV  pblkd, GPT_TABLE  *pgpt);    /*  从块设备加载 GPT 表         */

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKPART_EN > 0)    */
                                                                        /*  LW_CFG_CPU_WORD_LENGHT > 32 */
#endif                                                                  /*  __GPTPARTITION_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
