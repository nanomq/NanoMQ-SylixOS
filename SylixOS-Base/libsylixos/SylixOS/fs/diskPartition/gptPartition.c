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
** ��   ��   ��: gptPartition.c
**
** ��   ��   ��: Jiang.Taijin (��̫��)
**
** �ļ���������: 2021 �� 06 �� 19 ��
**
** ��        ��: GPT ���������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_internal.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKPART_EN > 0) && (LW_CFG_CPU_WORD_LENGHT > 32)
#include "sys/uuid.h"
#include "endian.h"
#include "gptPartition.h"
/*********************************************************************************************************
  �������������� GPT ͷ�߼����е�ƫ�ƣ���������������Ϊʵ�� SylixOS �Զ����ع�������ӵ�����
*********************************************************************************************************/
#define GPT_PART_TYPE_OFF   128
/*********************************************************************************************************
  GPT ������ǰ׺
*********************************************************************************************************/
static const CHAR  _G_acGptPartName[] = "SylixOSPart";
/*********************************************************************************************************
  �������� UUID��Windows ���ݷ���
*********************************************************************************************************/
static const UINT8  _G_ucGptPartTypeData[16] = {
    0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
    0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};
/*********************************************************************************************************
  �������� EFI��UEFI��������
*********************************************************************************************************/
static const UINT8  _G_ucGptPartTypeEfi[16] = {
    0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11,
    0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b,
};
/*********************************************************************************************************
** ��������: __gptCrcReflect
** ��������: �ߵ�λ�Ի�(i.e. 10101000 <--> 00010101)������ crc32 ����
** �䡡��  : iData              ����������
**           iLen               ����Ϊ��
** �䡡��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __gptCrcReflect (INT  iData, INT  iLen)
{
    INT  iRef = 0;
    INT  i;

    for (i = 0; i < iLen; i++) {
        if (iData & 0x1) {
            iRef |= (1 << ((iLen - 1) - i));
        }
        iData = (iData >> 1);
    }

    return  (iRef);
}
/*********************************************************************************************************
** ��������: __gptCrc32
** ��������: ���� crc
** �䡡��  : pucBuffer          ���ݻ�����
**           iLen               ���ݳ���
** �䡡��  : crc32 ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT32  __gptCrc32 (UINT8  *pucBuffer, INT  iLen)
{
    INT  iByteLen    = 8;
    INT  iMsb        = 0;
    INT  iPolynomial = 0x04c11db7;                                      /*  IEEE 32bit polynomial       */
    UINT uiRegs      = 0xffffffff;
    INT  iRegsMask   = 0xffffffff;
    INT  iRegsMsb    = 0;
    UINT uiReflectedRegs;
    INT  iDataByte;
    INT  i;
    INT  j;

    for (i = 0; i < iLen; i++) {
        iDataByte = pucBuffer[i];
        iDataByte = __gptCrcReflect(iDataByte, 8);

        for (j = 0; j < iByteLen; j++) {
            iMsb      = iDataByte >> (iByteLen - 1);                    /*  get MSB                     */
            iMsb     &= 1;                                              /*  ensure just 1 bit           */
            iRegsMsb  = (uiRegs >> 31) & 1;                             /*  MSB of regs                 */
            uiRegs    = uiRegs << 1;                                    /*  shift regs for CRC-CCITT    */
            if (iRegsMsb ^ iMsb) {                                      /*  MSB is a 1                  */
                uiRegs = uiRegs ^ iPolynomial;                          /*  XOR with generator poly     */
            }

            uiRegs      = uiRegs & iRegsMask;                           /*  Mask off excess upper bits  */
            iDataByte <<= 1;                                            /*  get to next bit             */
        }
    }
    uiRegs = uiRegs & iRegsMask;
    uiReflectedRegs = __gptCrcReflect(uiRegs, 32) ^ 0xffffffff;

    return  (uiReflectedRegs);
}
/*********************************************************************************************************
** ��������: __gptInitMbr
** ��������: ��ʼ�� MBR
** �䡡��  : ucMbr              MBR ���ݻ�����
**           ui64LbaCnt         ���豸�߼�������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __gptInitMbr (UINT8  *ucMbr, UINT64  ui64LbaCnt)
{
    ucMbr[0x1be] = 0x00;
    ucMbr[0x1bf] = 0x00;
    ucMbr[0x1c0] = 0x01;
    ucMbr[0x1c1] = 0x00;

    ucMbr[0x1c2] = 0xee;
    ucMbr[0x1c3] = 0xfe;
    ucMbr[0x1c4] = 0xff;
    ucMbr[0x1c5] = 0xff;

    ucMbr[0x1c6] = 0x01;
    ucMbr[0x1c7] = 0x00;
    ucMbr[0x1c8] = 0x00;
    ucMbr[0x1c9] = 0x00;

    ucMbr[0x1ca] = (ui64LbaCnt & 0xff);
    ucMbr[0x1cb] = ((ui64LbaCnt >> 8) & 0xff);
    ucMbr[0x1cc] = ((ui64LbaCnt >> 16) & 0xff);
    ucMbr[0x1cd] = ((ui64LbaCnt >> 24) & 0xff);

    ucMbr[0x1fe] = 0x55;
    ucMbr[0x1ff] = 0xaa;
}
/*********************************************************************************************************
** ��������: API_GptCreateAndInitWithLba
** ��������: ����ͳ�ʼ�� GPT �ڴ�ṹ
** �䡡��  : ulSecSize          ������С
**           ui64SecCnt         ���豸����������Ϊ 0 ��ֻ�����ڴ治��ʼ���ṹ
**           ui64FirstLba       GPT����FirstLbaƫ��, Ϊ 0 ��ʹ��Ĭ��ƫ��
** �䡡��  : GPT �ڴ�ṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
GPT_TABLE  *API_GptCreateAndInitWithLba (UINT  ulSecSize, UINT64  ui64SecCnt, UINT64 ui64FirstLba)
{
    GPT_TABLE   *pgpt;
    GPT_HEADER  *pgpthdr;

    pgpt = (GPT_TABLE *)__SHEAP_ALLOC(sizeof(GPT_TABLE));
    if (!pgpt) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                          /*  �ڴ治��                    */
        return  (LW_NULL);
    }

    if (ulSecSize > GPT_MAX_SECSIZE) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    lib_bzero((PVOID)pgpt, sizeof(GPT_TABLE));

    pgpt->GPT_uiSecSize = ulSecSize;

    if (ui64SecCnt == 0) {                                              /*  �����ʼ��                  */
        return  (pgpt);
    }

    __gptInitMbr(pgpt->GPT_ucMbr, ui64SecCnt);                          /*  ��ʼ�� MBR                  */

    /*
     *  ��ʼ�� GPT ��ͷ
     */
    pgpthdr = &pgpt->GPT_header;
    lib_memcpy(pgpthdr->HDR_ucMagic, GPT_MAGIC, sizeof(GPT_HEADER));
    pgpthdr->HDR_uiVersion     = GPT_VERSION;
    pgpthdr->HDR_uiHeadSz      = sizeof(GPT_HEADER);
    pgpthdr->HDR_uiCrc32       = 0;
    pgpthdr->HDR_uiReserved    = 0;

    pgpthdr->HDR_ui64HeaderLba = GPT_HEAD_LBA;
    pgpthdr->HDR_ui64BackupLba = ui64SecCnt - GPT_HEAD_LBA_CNT;
    if (ui64FirstLba != 0) {
        pgpthdr->HDR_ui64FirstLba  = ui64FirstLba;                      /*  ʹ�ò���ָ����ƫ��          */
    } else {
        pgpthdr->HDR_ui64FirstLba  = GPT_DATA_LBA;                      /*  ʹ��Ĭ��ƫ��                */
    }
    pgpthdr->HDR_ui64LastLba   = ui64SecCnt - GPT_HEAD_LBA_CNT - GPT_ENT_LBA_CNT;

    uuidgen((uuid_t *)pgpthdr->HDR_ucVolumeUuid, 1);

    pgpthdr->HDR_ui64Entrieslba = 2;
    pgpthdr->HDR_uiEntriesCount = 0;
    pgpthdr->HDR_uiEntriesSize  = sizeof(GPT_ENTRY);
    pgpthdr->HDR_uiEntriesCrc32 = 0;

    return  (pgpt);
}
/*********************************************************************************************************
** ��������: API_GptCreateAndInit
** ��������: ����ͳ�ʼ�� GPT �ڴ�ṹ
** �䡡��  : ulSecSize          ������С
**           ui64SecCnt         ���豸����������Ϊ 0 ��ֻ�����ڴ治��ʼ���ṹ
** �䡡��  : GPT �ڴ�ṹ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
GPT_TABLE  *API_GptCreateAndInit (UINT  ulSecSize, UINT64  ui64SecCnt)
{
    return  API_GptCreateAndInitWithLba(ulSecSize, ui64SecCnt, 0);
}
/*********************************************************************************************************
** ��������: API_GptDestroy
** ��������: ���� GPT �ڴ�ṹ
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  API_GptDestroy (GPT_TABLE  *pgpt)
{
    if (!pgpt) {
        _ErrorHandle(EINVAL);
    }

    __SHEAP_FREE(pgpt);
}
/*********************************************************************************************************
** ��������: API_GptAddEntryWithName
** ��������: ��ӷ�������
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
**           ui64LbaNum         ������ʼ�߼����
**           ui64LbaCnt         �����߼�������
**           ucPartType         ��������
**           cpcName            ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_GptAddEntryWithName (GPT_TABLE  *pgpt,
                              UINT64      ui64LbaNum,
                              UINT64      ui64LbaCnt,
                              UINT8       ucPartType,
                              CPCHAR      cpcName)
{
    size_t  stLen;
    INT     iIndex;
    INT     i;

    if (!pgpt || (pgpt->GPT_header.HDR_uiVersion == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iIndex = pgpt->GPT_header.HDR_uiEntriesCount;

    if (ucPartType == LW_DISK_PART_TYPE_EFI) {
        lib_memcpy(pgpt->GPT_entry[iIndex].ENT_ucTypeUuid,
                   _G_ucGptPartTypeEfi, sizeof(_G_ucGptPartTypeEfi));   /*  EFI����                     */

    } else {
        lib_memcpy(pgpt->GPT_entry[iIndex].ENT_ucTypeUuid,
                   _G_ucGptPartTypeData, sizeof(_G_ucGptPartTypeData)); /*  ���ݷ���                    */
    }

    uuidgen((uuid_t *)pgpt->GPT_entry[iIndex].ENT_ucUniqUuid, 1);
    pgpt->GPT_entry[iIndex].ENT_ui64FirstLba = ui64LbaNum;
    pgpt->GPT_entry[iIndex].ENT_ui64LastLba  = ui64LbaNum + ui64LbaCnt - 1;
    pgpt->GPT_entry[iIndex].ENT_ui64Attr     = 0;

    stLen = lib_strlen(cpcName);
    if (stLen > GPT_MAX_NAMELEN) {
        stLen = GPT_MAX_NAMELEN;
    }

    if (stLen != 0) {
        for (i = 0; i < stLen; i++) {
            pgpt->GPT_entry[iIndex].ENT_usName[i] = cpcName[i];
        }

    } else {
        /*
         *  �������� SylixOSPart*
         */
        stLen = lib_strlen(_G_acGptPartName);
        for (i = 0; i < stLen; i++) {
            pgpt->GPT_entry[iIndex].ENT_usName[i] = _G_acGptPartName[i];
        }
        pgpt->GPT_entry[iIndex].ENT_usName[i] = iIndex;
    }

    /*
     *  ���������ͱ����� GPT ����ͷ�����Ŀ����ֽ���
     */
    if ((GPT_PART_TYPE_OFF + iIndex) >= pgpt->GPT_uiSecSize) {
        return  (PX_ERROR);
    }

    pgpt->GPT_ucBlock[GPT_PART_TYPE_OFF + iIndex] = ucPartType;

    pgpt->GPT_header.HDR_uiEntriesCount++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GptAddEntry
** ��������: ��ӷ�������
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
**           ui64LbaNum         ������ʼ�߼����
**           ui64LbaCnt         �����߼�������
**           ucPartType         ��������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_GptAddEntry (GPT_TABLE  *pgpt,
                      UINT64      ui64LbaNum,
                      UINT64      ui64LbaCnt,
                      UINT8       ucPartType)
{
    return  API_GptAddEntryWithName(pgpt, ui64LbaNum, ui64LbaCnt, ucPartType, LW_NULL);
}
/*********************************************************************************************************
** ��������: API_GptGetEntry
** ��������: ��ȡ��������
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
**           iIndex             ��������
**           pui64LbaNum        ���������ʼ�߼����
**           pui64LbaCnt        ��������߼�������
**           pucPartType        �����������
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_GptGetEntry (GPT_TABLE  *pgpt,
                      INT         iIndex,
                      UINT64     *pui64LbaNum,
                      UINT64     *pui64LbaCnt,
                      UINT8      *pucPartType)
{
    GPT_ENTRY  *pentry;

    if (!pgpt || (pgpt->GPT_header.HDR_uiVersion == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (iIndex >= pgpt->GPT_header.HDR_uiEntriesCount) {
        return  (PX_ERROR);
    }

    pentry       = &pgpt->GPT_entry[iIndex];
    *pui64LbaNum = pentry->ENT_ui64FirstLba;
    *pui64LbaCnt = pentry->ENT_ui64LastLba - pentry->ENT_ui64FirstLba + 1;
    *pucPartType = pgpt->GPT_ucBlock[GPT_PART_TYPE_OFF + iIndex];

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __gptSerial
** ��������: ���л�������
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __gptSerial (GPT_TABLE  *pgpt)
{
    GPT_HEADER  *pgpthdr = &pgpt->GPT_header;
    GPT_ENTRY   *pentry  = pgpt->GPT_entry;
    INT          i;
    INT          j;

    /*
     *  �����л���������Ա���������crc
     */
    pgpthdr->HDR_uiEntriesCount = GPT_MAX_ENTRIES;                      /*  Windows ����������Ϊ���ֵ  */
    for (i = 0; i < pgpthdr->HDR_uiEntriesCount; i++, pentry++) {
        pentry->ENT_ui64FirstLba = htole64(pentry->ENT_ui64FirstLba);
        pentry->ENT_ui64LastLba  = htole64(pentry->ENT_ui64LastLba);
        pentry->ENT_ui64Attr     = htole64(pentry->ENT_ui64Attr);

        for (j = 0; j < GPT_MAX_NAMELEN; j++) {
            pentry->ENT_usName[j] = htole16(pentry->ENT_usName[j]);
        }
    }

    pgpthdr->HDR_uiCrc32        = 0;                                    /*  ����ǰ��ʼ�� crc Ϊ 0       */
    pgpthdr->HDR_uiVersion      = htole32(pgpthdr->HDR_uiVersion);
    pgpthdr->HDR_uiHeadSz       = htole32(pgpthdr->HDR_uiHeadSz);
    pgpthdr->HDR_uiReserved     = htole32(pgpthdr->HDR_uiReserved);
    pgpthdr->HDR_ui64HeaderLba  = htole64(pgpthdr->HDR_ui64HeaderLba);
    pgpthdr->HDR_ui64BackupLba  = htole64(pgpthdr->HDR_ui64BackupLba);
    pgpthdr->HDR_ui64FirstLba   = htole64(pgpthdr->HDR_ui64FirstLba);
    pgpthdr->HDR_ui64LastLba    = htole64(pgpthdr->HDR_ui64LastLba);
    pgpthdr->HDR_ui64Entrieslba = htole64(pgpthdr->HDR_ui64Entrieslba);
    pgpthdr->HDR_uiEntriesCount = htole32(pgpthdr->HDR_uiEntriesCount);
    pgpthdr->HDR_uiEntriesSize  = htole32(pgpthdr->HDR_uiEntriesSize);
    pgpthdr->HDR_uiEntriesCrc32 = __gptCrc32((UINT8 *)pgpt->GPT_entry,
                                             pgpthdr->HDR_uiEntriesCount * sizeof(GPT_ENTRY));

    pgpthdr->HDR_uiCrc32 = __gptCrc32((UINT8 *)pgpthdr, sizeof(GPT_HEADER));
}
/*********************************************************************************************************
** ��������: __gptUnserial
** ��������: �����л�������
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __gptUnserial (GPT_TABLE  *pgpt)
{
    GPT_HEADER  *pgpthdr = &pgpt->GPT_header;
    GPT_ENTRY   *pentry  = pgpt->GPT_entry;
    INT          i;
    INT          j;

    pgpthdr->HDR_uiCrc32        = 0;
    pgpthdr->HDR_uiVersion      = le32toh(pgpthdr->HDR_uiVersion);
    pgpthdr->HDR_uiHeadSz       = le32toh(pgpthdr->HDR_uiHeadSz);
    pgpthdr->HDR_uiReserved     = le32toh(pgpthdr->HDR_uiReserved);
    pgpthdr->HDR_ui64HeaderLba  = le64toh(pgpthdr->HDR_ui64HeaderLba);
    pgpthdr->HDR_ui64BackupLba  = le64toh(pgpthdr->HDR_ui64BackupLba);
    pgpthdr->HDR_ui64FirstLba   = le64toh(pgpthdr->HDR_ui64FirstLba);
    pgpthdr->HDR_ui64LastLba    = le64toh(pgpthdr->HDR_ui64LastLba);
    pgpthdr->HDR_ui64Entrieslba = le64toh(pgpthdr->HDR_ui64Entrieslba);
    pgpthdr->HDR_uiEntriesCount = le32toh(pgpthdr->HDR_uiEntriesCount);
    pgpthdr->HDR_uiEntriesSize  = le32toh(pgpthdr->HDR_uiEntriesSize);

    for (i = 0; i < pgpthdr->HDR_uiEntriesCount; i++, pentry++) {       /*  ����������ֱ���Ƿ�����      */
        pentry->ENT_ui64FirstLba = le64toh(pentry->ENT_ui64FirstLba);
        pentry->ENT_ui64LastLba  = le64toh(pentry->ENT_ui64LastLba);
        pentry->ENT_ui64Attr     = le64toh(pentry->ENT_ui64Attr);

        for (j = 0; j < GPT_MAX_NAMELEN; j++) {
            pentry->ENT_usName[j] = le16toh(pentry->ENT_usName[j]);
        }

        if (pentry->ENT_ui64FirstLba == 0) {
            break;
        }
    }

    pgpthdr->HDR_uiEntriesCount = i;                                    /*  ���÷���������Ϊ��Ч����    */
}
/*********************************************************************************************************
** ��������: __gptVerify
** ��������: У�� GPT crc
** �䡡��  : pgpt               GPT �ڴ�ṹָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __gptVerify (GPT_TABLE  *pgpt)
{
    GPT_HEADER  *pgpthdr = &pgpt->GPT_header;
    UINT32       uiCrc;
    UINT32       uiCrcCalc;
    UINT32       uiCrcEntries;
    UINT32       uiEntriesCount;

    uiCrc                = pgpthdr->HDR_uiCrc32;                        /*  ���� crc                    */
    pgpthdr->HDR_uiCrc32 = 0;                                           /*  ����ǰ�轫��ͷ crc ��0      */

    uiCrcCalc = __gptCrc32((UINT8 *)pgpthdr, sizeof(GPT_HEADER));       /*  �Ƚϱ�ͷ crc                */
    if (uiCrc != uiCrcCalc) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Gpt header crc error : %08x %08x\n",
                     uiCrc, uiCrcCalc);
        return  (PX_ERROR);
    }

    uiEntriesCount = le32toh(pgpthdr->HDR_uiEntriesCount);              /*  ��ȡ�������������ڼ����С  */
    uiCrcEntries   = __gptCrc32((UINT8 *)pgpt->GPT_entry, uiEntriesCount * sizeof(GPT_ENTRY));
    if (pgpthdr->HDR_uiEntriesCrc32 != uiCrcEntries) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Gpt entries crc error : %08x %08x\n",
                     pgpthdr->HDR_uiEntriesCrc32, uiCrcEntries);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GptPartitionSave
** ��������: ���� GPT �����豸
** �䡡��  : iBlkFd             ���豸�ļ�������
**           pgpt               GPT �ڴ�ṹָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_GptPartitionSave (INT  iBlkFd, GPT_TABLE  *pgpt)
{
    GPT_HEADER   gpthdrBakup;
    GPT_HEADER  *pgpthdr;

    if (!pgpt || (pgpt->GPT_header.HDR_uiVersion == 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pgpthdr     = &pgpt->GPT_header;
    gpthdrBakup = (*pgpthdr);

    __gptSerial(pgpt);                                                  /*  ���л�������ṹ            */

    if (pwrite(iBlkFd, pgpt->GPT_ucMbr,
               pgpt->GPT_uiSecSize, 0) != pgpt->GPT_uiSecSize) {        /*  д�� MBR                    */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_ucBlock, pgpt->GPT_uiSecSize,
               (gpthdrBakup.HDR_ui64HeaderLba * pgpt->GPT_uiSecSize)) != pgpt->GPT_uiSecSize) {
                                                                        /*  д�� GPT ������ͷ           */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_ucBlock, pgpt->GPT_uiSecSize,
               (gpthdrBakup.HDR_ui64BackupLba * pgpt->GPT_uiSecSize)) != pgpt->GPT_uiSecSize) {
                                                                        /*  д�� GPT ���ݷ�����ͷ       */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_entry, sizeof(pgpt->GPT_entry),
               (gpthdrBakup.HDR_ui64Entrieslba * pgpt->GPT_uiSecSize)) != sizeof(pgpt->GPT_entry)) {
                                                                        /*  д�� GPT ��������           */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_entry, sizeof(pgpt->GPT_entry),
               (gpthdrBakup.HDR_ui64LastLba * pgpt->GPT_uiSecSize)) != sizeof(pgpt->GPT_entry)) {
                                                                        /*  д�� GPT ���ݷ�����ͷ       */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    /*
     *  һ������� GPT �ڴ�ṹ�ڱ����ᱻ���٣�Ϊ����һ�����������л� GPT �ڴ�ṹ���Ա����ʹ��
     */
    __gptUnserial(pgpt);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GptPartitionLoad
** ��������: ͨ�����豸�ļ�������� GPT �����豸
** �䡡��  : iBlkFd             ���豸�ļ�������
**           pgpt               GPT �ڴ�ṹָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_GptPartitionLoad (INT  iBlkFd, GPT_TABLE  *pgpt)
{
    if (!pgpt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pgpt->GPT_ucMbr,
              pgpt->GPT_uiSecSize, 0) != pgpt->GPT_uiSecSize) {         /*  ��ȡ MBR                    */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pgpt->GPT_ucMbr[0x1c2] != 0xee) {                               /*  ���ͱ�ʶ����                */
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pgpt->GPT_ucBlock, pgpt->GPT_uiSecSize,
              (1 * pgpt->GPT_uiSecSize)) != pgpt->GPT_uiSecSize) {      /*  ��ȡ GPT ������ͷ           */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, (UINT8 *)pgpt->GPT_entry, sizeof(pgpt->GPT_entry),
              (2 * pgpt->GPT_uiSecSize)) != sizeof(pgpt->GPT_entry)) {  /*  ��ȡ��������                */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (__gptVerify(pgpt) != ERROR_NONE) {                              /*  fdisk ������ָ�          */
        return  (PX_ERROR);
    }

    __gptUnserial(pgpt);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_GptPartitionLoad
** ��������: ͨ�����豸�ļ�������� GPT �����豸
** �䡡��  : pblkd              ���豸ָ��
**           pgpt               GPT �ڴ�ṹָ��
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  API_GptPartitionBlkLoad (PLW_BLK_DEV  pblkd, GPT_TABLE  *pgpt)
{
    if (!pgpt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, pgpt->GPT_ucMbr,
                               0, 1) != ERROR_NONE) {                   /*  ��ȡ MBR                    */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pgpt->GPT_ucMbr[0x1c2] != 0xee) {                               /*  ���ͱ�ʶ����                */
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, pgpt->GPT_ucBlock,
                               GPT_HEAD_LBA, GPT_HEAD_LBA_CNT) != ERROR_NONE) {
                                                                        /*  ��ȡ GPT ������ͷ           */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, (UINT8 *)pgpt->GPT_entry,
                               GPT_ENT_LBA, GPT_ENT_LBA_CNT) != ERROR_NONE) {
                                                                        /*  ��ȡ��������                */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (__gptVerify(pgpt) != ERROR_NONE) {
        /*
         *  �ӱ�����������ȡ���ָ� GPT ��������Ϣ
         */
        if (pblkd->BLKD_pfuncBlkRd(pblkd,
                                   pgpt->GPT_ucBlock,
                                   (pblkd->BLKD_ulNSector - GPT_HEAD_LBA_CNT),
                                   GPT_HEAD_LBA_CNT) != ERROR_NONE) {   /*  ��ȡ GPT ���ݷ�����ͷ       */
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (pblkd->BLKD_pfuncBlkWrt(pblkd, pgpt->GPT_ucBlock,
                                    GPT_HEAD_LBA, GPT_HEAD_LBA_CNT) != ERROR_NONE) {
                                                                        /*  �ָ� GPT ������ͷ           */
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (pblkd->BLKD_pfuncBlkRd(pblkd,
                                   (UINT8 *)pgpt->GPT_entry,
                                   (pblkd->BLKD_ulNSector - GPT_ENT_LBA_CNT - GPT_HEAD_LBA_CNT),
                                   GPT_ENT_LBA_CNT) != ERROR_NONE) {    /*  ��ȡ���ݷ�������            */

            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (pblkd->BLKD_pfuncBlkWrt(pblkd, (UINT8 *)pgpt->GPT_entry,
                                    GPT_ENT_LBA, GPT_ENT_LBA_CNT) != ERROR_NONE) {
                                                                        /*  �ָ���������                */
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (__gptVerify(pgpt) != ERROR_NONE) {
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }
    }

    __gptUnserial(pgpt);

    return  (ERROR_NONE);
}

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_DISKPART_EN > 0)    */
                                                                        /*  LW_CFG_CPU_WORD_LENGHT > 32 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
