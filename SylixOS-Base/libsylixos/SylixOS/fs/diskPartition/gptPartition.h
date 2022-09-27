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
** ��   ��   ��: gptPartition.h
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2021 �� 06 �� 19 ��
**
** ��        ��: GPT ���������.
*********************************************************************************************************/

#ifndef __GPTPARTITION_H
#define __GPTPARTITION_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKPART_EN > 0) && (LW_CFG_CPU_WORD_LENGHT > 32)
/*********************************************************************************************************
  GPT ������غ궨��
*********************************************************************************************************/
#define GPT_VERSION         0x00010000                                  /*  GPT ��汾��                */
#define GPT_MAGIC           "EFI PART"                                  /*  GPT ���ʶ                  */
#define GPT_MAX_ENTRIES     128                                         /*  ��������                  */
#define GPT_MAX_NAMELEN     36                                          /*  ������������              */
#define GPT_MAX_SECSIZE     4096                                        /*  ֧�ֵ�����߼����С        */
#define GPT_DATA_LBA        34                                          /*  ������ʼ�߼���              */
#define GPT_HEAD_LBA        1                                           /*  GPT ͷ�߼����              */
#define GPT_HEAD_LBA_CNT    1                                           /*  GPT ͷռ�õ��߼�������      */
#define GPT_ENT_LBA         2                                           /*  GPT ��������ʼ�߼���        */
#define GPT_ENT_LBA_CNT     32                                          /*  GPT �������߼�������        */
/*********************************************************************************************************
  GPT ������ͷ
*********************************************************************************************************/
LW_STRUCT_PACK_BEGIN
typedef struct {
    LW_STRUCT_PACK_FIELD(UINT8   HDR_ucMagic[8]);                       /*  GPT ���ʶ                  */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiVersion);                        /*  GPT ��汾��                */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiHeadSz);                         /*  GPT ��ͷ��С                */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiCrc32);                          /*  GPT ��ͷ crc                */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiReserved);                       /*  �����ֶ�                    */

    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64HeaderLba);                    /*  GPT ��ͷ�߼����            */
    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64BackupLba);                    /*  GPT ���ݱ�ͷ�߼����        */
    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64FirstLba);                     /*  GPT ��������ʼ��            */
    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64LastLba);                      /*  GPT ������������            */

    LW_STRUCT_PACK_FIELD(UINT8   HDR_ucVolumeUuid[16]);                 /*  ���豸 UUID                 */

    LW_STRUCT_PACK_FIELD(UINT64  HDR_ui64Entrieslba);                   /*  GPT ��������ʼ�߼���        */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiEntriesCount);                   /*  GPT ����������              */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiEntriesSize);                    /*  GPT ���������С            */
    LW_STRUCT_PACK_FIELD(UINT32  HDR_uiEntriesCrc32);                   /*  GPT ������CRC               */
} LW_STRUCT_PACK_STRUCT GPT_HEADER;
LW_STRUCT_PACK_END
/*********************************************************************************************************
  GPT ��������
*********************************************************************************************************/
LW_STRUCT_PACK_BEGIN
typedef struct {
    LW_STRUCT_PACK_FIELD(UINT8   ENT_ucTypeUuid[16]);                   /*  �������� UUID               */
    LW_STRUCT_PACK_FIELD(UINT8   ENT_ucUniqUuid[16]);                   /*  ������ʶ UUID               */
    LW_STRUCT_PACK_FIELD(UINT64  ENT_ui64FirstLba);                     /*  ������ʼ�߼����            */
    LW_STRUCT_PACK_FIELD(UINT64  ENT_ui64LastLba);                      /*  ���������߼����            */
    LW_STRUCT_PACK_FIELD(UINT64  ENT_ui64Attr);                         /*  ��������                    */
    LW_STRUCT_PACK_FIELD(UINT16  ENT_usName[GPT_MAX_NAMELEN]);          /*  ��������                    */
} LW_STRUCT_PACK_STRUCT GPT_ENTRY;
LW_STRUCT_PACK_END
/*********************************************************************************************************
  GPT �ڴ�ṹ
*********************************************************************************************************/

typedef struct {
    UINT   GPT_uiSecSize;                                               /*  ������С                    */
    UINT8  GPT_ucMbr[GPT_MAX_SECSIZE];                                  /*  MBR ��Ϣ                    */
    union {
        GPT_HEADER  GPT_header;                                         /*  GPT ��ͷ                    */
        UINT8       GPT_ucBlock[GPT_MAX_SECSIZE];                       /*  GPT ��ͷ��Ӧ��������        */
    };
    GPT_ENTRY  GPT_entry[GPT_MAX_ENTRIES];                              /*  GPT ������������            */
} GPT_TABLE;

GPT_TABLE  *API_GptCreateAndInit(UINT    ulSecSize,
                                 UINT64  ui64SecCnt);                   /*  ����ͳ�ʼ�� GPT �ڴ�ṹ   */

GPT_TABLE  *API_GptCreateAndInitWithLba(UINT    ulSecSize,
                                        UINT64  ui64SecCnt,
                                        UINT64  ui64FirstLba);          /*  ����ͳ�ʼ�� GPT �ڴ�ṹ   */

VOID  API_GptDestroy(GPT_TABLE  *pgpt);                                 /*  ���� GPT �ڴ�ṹ           */

INT   API_GptAddEntry(GPT_TABLE  *pgpt,
                      UINT64      ui64LbaNum,
                      UINT64      ui64LbaCnt,
                      UINT8       ucPartType);                          /*  ��� GPT ����               */

INT   API_GptAddEntryWithName(GPT_TABLE  *pgpt,
                              UINT64      ui64LbaNum,
                              UINT64      ui64LbaCnt,
                              UINT8       ucPartType,
                              CPCHAR      cpcName);                     /*  ��� GPT ����               */

INT   API_GptGetEntry(GPT_TABLE  *pgpt,
                      INT         iIndex,
                      UINT64     *ui64LbaNum,
                      UINT64     *pui64LbaCnt,
                      UINT8      *pucPartType);                         /*  ��ȡ GPT ����               */

INT   API_GptPartitionSave(INT  iBlkFd, GPT_TABLE  *pgpt);              /*  ���� GPT �����豸         */

INT   API_GptPartitionLoad(INT  iBlkFd, GPT_TABLE  *pgpt);              /*  �ӿ��豸���� GPT ��         */

INT   API_GptPartitionBlkLoad(PLW_BLK_DEV  pblkd, GPT_TABLE  *pgpt);    /*  �ӿ��豸���� GPT ��         */

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKPART_EN > 0)    */
                                                                        /*  LW_CFG_CPU_WORD_LENGHT > 32 */
#endif                                                                  /*  __GPTPARTITION_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
