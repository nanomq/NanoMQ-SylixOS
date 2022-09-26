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
** 文   件   名: gptPartition.c
**
** 创   建   人: Jiang.Taijin (蒋太金)
**
** 文件创建日期: 2021 年 06 月 19 日
**
** 描        述: GPT 分区表分析.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_internal.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_DISKPART_EN > 0) && (LW_CFG_CPU_WORD_LENGHT > 32)
#include "sys/uuid.h"
#include "endian.h"
#include "gptPartition.h"
/*********************************************************************************************************
  分区类型数组在 GPT 头逻辑块中的偏移，分区类型数组是为实现 SylixOS 自动挂载功能所添加的数据
*********************************************************************************************************/
#define GPT_PART_TYPE_OFF   128
/*********************************************************************************************************
  GPT 分区名前缀
*********************************************************************************************************/
static const CHAR  _G_acGptPartName[] = "SylixOSPart";
/*********************************************************************************************************
  分区类型 UUID，Windows 数据分区
*********************************************************************************************************/
static const UINT8  _G_ucGptPartTypeData[16] = {
    0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
    0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};
/*********************************************************************************************************
  分区类型 EFI，UEFI引导分区
*********************************************************************************************************/
static const UINT8  _G_ucGptPartTypeEfi[16] = {
    0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11,
    0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b,
};
/*********************************************************************************************************
** 函数名称: __gptCrcReflect
** 功能描述: 高低位对换(i.e. 10101000 <--> 00010101)，用于 crc32 计算
** 输　入  : iData              待计算数据
**           iLen               数据为宽
** 输　出  : 计算结果
** 全局变量:
** 调用模块:
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
** 函数名称: __gptCrc32
** 功能描述: 计算 crc
** 输　入  : pucBuffer          数据缓冲区
**           iLen               数据长度
** 输　出  : crc32 值
** 全局变量:
** 调用模块:
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
** 函数名称: __gptInitMbr
** 功能描述: 初始化 MBR
** 输　入  : ucMbr              MBR 数据缓冲区
**           ui64LbaCnt         块设备逻辑块数量
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: API_GptCreateAndInitWithLba
** 功能描述: 分配和初始化 GPT 内存结构
** 输　入  : ulSecSize          扇区大小
**           ui64SecCnt         块设备扇区数量，为 0 则只分配内存不初始化结构
**           ui64FirstLba       GPT分区FirstLba偏移, 为 0 则使用默认偏移
** 输　出  : GPT 内存结构
** 全局变量:
** 调用模块:
*********************************************************************************************************/
GPT_TABLE  *API_GptCreateAndInitWithLba (UINT  ulSecSize, UINT64  ui64SecCnt, UINT64 ui64FirstLba)
{
    GPT_TABLE   *pgpt;
    GPT_HEADER  *pgpthdr;

    pgpt = (GPT_TABLE *)__SHEAP_ALLOC(sizeof(GPT_TABLE));
    if (!pgpt) {
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);                          /*  内存不足                    */
        return  (LW_NULL);
    }

    if (ulSecSize > GPT_MAX_SECSIZE) {
        _ErrorHandle(EINVAL);
        return  (LW_NULL);
    }

    lib_bzero((PVOID)pgpt, sizeof(GPT_TABLE));

    pgpt->GPT_uiSecSize = ulSecSize;

    if (ui64SecCnt == 0) {                                              /*  无需初始化                  */
        return  (pgpt);
    }

    __gptInitMbr(pgpt->GPT_ucMbr, ui64SecCnt);                          /*  初始化 MBR                  */

    /*
     *  初始化 GPT 表头
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
        pgpthdr->HDR_ui64FirstLba  = ui64FirstLba;                      /*  使用参数指定的偏移          */
    } else {
        pgpthdr->HDR_ui64FirstLba  = GPT_DATA_LBA;                      /*  使用默认偏移                */
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
** 函数名称: API_GptCreateAndInit
** 功能描述: 分配和初始化 GPT 内存结构
** 输　入  : ulSecSize          扇区大小
**           ui64SecCnt         块设备扇区数量，为 0 则只分配内存不初始化结构
** 输　出  : GPT 内存结构
** 全局变量:
** 调用模块:
*********************************************************************************************************/
GPT_TABLE  *API_GptCreateAndInit (UINT  ulSecSize, UINT64  ui64SecCnt)
{
    return  API_GptCreateAndInitWithLba(ulSecSize, ui64SecCnt, 0);
}
/*********************************************************************************************************
** 函数名称: API_GptDestroy
** 功能描述: 销毁 GPT 内存结构
** 输　入  : pgpt               GPT 内存结构指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  API_GptDestroy (GPT_TABLE  *pgpt)
{
    if (!pgpt) {
        _ErrorHandle(EINVAL);
    }

    __SHEAP_FREE(pgpt);
}
/*********************************************************************************************************
** 函数名称: API_GptAddEntryWithName
** 功能描述: 添加分区表项
** 输　入  : pgpt               GPT 内存结构指针
**           ui64LbaNum         分区起始逻辑块号
**           ui64LbaCnt         分区逻辑块数量
**           ucPartType         分区类型
**           cpcName            分区名称
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
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
                   _G_ucGptPartTypeEfi, sizeof(_G_ucGptPartTypeEfi));   /*  EFI分区                     */

    } else {
        lib_memcpy(pgpt->GPT_entry[iIndex].ENT_ucTypeUuid,
                   _G_ucGptPartTypeData, sizeof(_G_ucGptPartTypeData)); /*  数据分区                    */
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
         *  分区名称 SylixOSPart*
         */
        stLen = lib_strlen(_G_acGptPartName);
        for (i = 0; i < stLen; i++) {
            pgpt->GPT_entry[iIndex].ENT_usName[i] = _G_acGptPartName[i];
        }
        pgpt->GPT_entry[iIndex].ENT_usName[i] = iIndex;
    }

    /*
     *  将分区类型保存在 GPT 分区头扇区的空闲字节中
     */
    if ((GPT_PART_TYPE_OFF + iIndex) >= pgpt->GPT_uiSecSize) {
        return  (PX_ERROR);
    }

    pgpt->GPT_ucBlock[GPT_PART_TYPE_OFF + iIndex] = ucPartType;

    pgpt->GPT_header.HDR_uiEntriesCount++;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GptAddEntry
** 功能描述: 添加分区表项
** 输　入  : pgpt               GPT 内存结构指针
**           ui64LbaNum         分区起始逻辑块号
**           ui64LbaCnt         分区逻辑块数量
**           ucPartType         分区类型
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  API_GptAddEntry (GPT_TABLE  *pgpt,
                      UINT64      ui64LbaNum,
                      UINT64      ui64LbaCnt,
                      UINT8       ucPartType)
{
    return  API_GptAddEntryWithName(pgpt, ui64LbaNum, ui64LbaCnt, ucPartType, LW_NULL);
}
/*********************************************************************************************************
** 函数名称: API_GptGetEntry
** 功能描述: 获取分区表项
** 输　入  : pgpt               GPT 内存结构指针
**           iIndex             分区索引
**           pui64LbaNum        输出分区起始逻辑块号
**           pui64LbaCnt        输出分区逻辑块数量
**           pucPartType        输出分区类型
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
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
** 函数名称: __gptSerial
** 功能描述: 序列化分区表
** 输　入  : pgpt               GPT 内存结构指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __gptSerial (GPT_TABLE  *pgpt)
{
    GPT_HEADER  *pgpthdr = &pgpt->GPT_header;
    GPT_ENTRY   *pentry  = pgpt->GPT_entry;
    INT          i;
    INT          j;

    /*
     *  先序列化分区表项，以便计算分区表crc
     */
    pgpthdr->HDR_uiEntriesCount = GPT_MAX_ENTRIES;                      /*  Windows 需设置项数为最大值  */
    for (i = 0; i < pgpthdr->HDR_uiEntriesCount; i++, pentry++) {
        pentry->ENT_ui64FirstLba = htole64(pentry->ENT_ui64FirstLba);
        pentry->ENT_ui64LastLba  = htole64(pentry->ENT_ui64LastLba);
        pentry->ENT_ui64Attr     = htole64(pentry->ENT_ui64Attr);

        for (j = 0; j < GPT_MAX_NAMELEN; j++) {
            pentry->ENT_usName[j] = htole16(pentry->ENT_usName[j]);
        }
    }

    pgpthdr->HDR_uiCrc32        = 0;                                    /*  计算前初始化 crc 为 0       */
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
** 函数名称: __gptUnserial
** 功能描述: 逆序列化分区表
** 输　入  : pgpt               GPT 内存结构指针
** 输　出  : NONE
** 全局变量:
** 调用模块:
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

    for (i = 0; i < pgpthdr->HDR_uiEntriesCount; i++, pentry++) {       /*  遍历分区表直至非法表项      */
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

    pgpthdr->HDR_uiEntriesCount = i;                                    /*  设置分区表项数为有效项数    */
}
/*********************************************************************************************************
** 函数名称: __gptVerify
** 功能描述: 校验 GPT crc
** 输　入  : pgpt               GPT 内存结构指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __gptVerify (GPT_TABLE  *pgpt)
{
    GPT_HEADER  *pgpthdr = &pgpt->GPT_header;
    UINT32       uiCrc;
    UINT32       uiCrcCalc;
    UINT32       uiCrcEntries;
    UINT32       uiEntriesCount;

    uiCrc                = pgpthdr->HDR_uiCrc32;                        /*  备份 crc                    */
    pgpthdr->HDR_uiCrc32 = 0;                                           /*  计算前需将表头 crc 置0      */

    uiCrcCalc = __gptCrc32((UINT8 *)pgpthdr, sizeof(GPT_HEADER));       /*  比较表头 crc                */
    if (uiCrc != uiCrcCalc) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Gpt header crc error : %08x %08x\n",
                     uiCrc, uiCrcCalc);
        return  (PX_ERROR);
    }

    uiEntriesCount = le32toh(pgpthdr->HDR_uiEntriesCount);              /*  获取分区表项数用于计算大小  */
    uiCrcEntries   = __gptCrc32((UINT8 *)pgpt->GPT_entry, uiEntriesCount * sizeof(GPT_ENTRY));
    if (pgpthdr->HDR_uiEntriesCrc32 != uiCrcEntries) {
        _DebugFormat(__ERRORMESSAGE_LEVEL, "Gpt entries crc error : %08x %08x\n",
                     pgpthdr->HDR_uiEntriesCrc32, uiCrcEntries);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GptPartitionSave
** 功能描述: 保存 GPT 表到块设备
** 输　入  : iBlkFd             块设备文件描述符
**           pgpt               GPT 内存结构指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
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

    __gptSerial(pgpt);                                                  /*  序列化分区表结构            */

    if (pwrite(iBlkFd, pgpt->GPT_ucMbr,
               pgpt->GPT_uiSecSize, 0) != pgpt->GPT_uiSecSize) {        /*  写入 MBR                    */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_ucBlock, pgpt->GPT_uiSecSize,
               (gpthdrBakup.HDR_ui64HeaderLba * pgpt->GPT_uiSecSize)) != pgpt->GPT_uiSecSize) {
                                                                        /*  写入 GPT 分区表头           */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_ucBlock, pgpt->GPT_uiSecSize,
               (gpthdrBakup.HDR_ui64BackupLba * pgpt->GPT_uiSecSize)) != pgpt->GPT_uiSecSize) {
                                                                        /*  写入 GPT 备份分区表头       */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_entry, sizeof(pgpt->GPT_entry),
               (gpthdrBakup.HDR_ui64Entrieslba * pgpt->GPT_uiSecSize)) != sizeof(pgpt->GPT_entry)) {
                                                                        /*  写入 GPT 分区表项           */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    if (pwrite(iBlkFd, pgpt->GPT_entry, sizeof(pgpt->GPT_entry),
               (gpthdrBakup.HDR_ui64LastLba * pgpt->GPT_uiSecSize)) != sizeof(pgpt->GPT_entry)) {
                                                                        /*  写入 GPT 备份分区表头       */
        _ErrorHandle(EIO);
        __gptUnserial(pgpt);
        return  (PX_ERROR);
    }

    /*
     *  一般情况下 GPT 内存结构在保存后会被销毁，为防万一，这里逆序列化 GPT 内存结构，以便后续使用
     */
    __gptUnserial(pgpt);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GptPartitionLoad
** 功能描述: 通过块设备文件句柄加载 GPT 表到块设备
** 输　入  : iBlkFd             块设备文件描述符
**           pgpt               GPT 内存结构指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  API_GptPartitionLoad (INT  iBlkFd, GPT_TABLE  *pgpt)
{
    if (!pgpt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pgpt->GPT_ucMbr,
              pgpt->GPT_uiSecSize, 0) != pgpt->GPT_uiSecSize) {         /*  读取 MBR                    */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pgpt->GPT_ucMbr[0x1c2] != 0xee) {                               /*  类型标识错误                */
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pgpt->GPT_ucBlock, pgpt->GPT_uiSecSize,
              (1 * pgpt->GPT_uiSecSize)) != pgpt->GPT_uiSecSize) {      /*  读取 GPT 分区表头           */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, (UINT8 *)pgpt->GPT_entry, sizeof(pgpt->GPT_entry),
              (2 * pgpt->GPT_uiSecSize)) != sizeof(pgpt->GPT_entry)) {  /*  读取分区表项                */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (__gptVerify(pgpt) != ERROR_NONE) {                              /*  fdisk 命令不做恢复          */
        return  (PX_ERROR);
    }

    __gptUnserial(pgpt);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_GptPartitionLoad
** 功能描述: 通过块设备文件句柄加载 GPT 表到块设备
** 输　入  : pblkd              块设备指针
**           pgpt               GPT 内存结构指针
** 输　出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  API_GptPartitionBlkLoad (PLW_BLK_DEV  pblkd, GPT_TABLE  *pgpt)
{
    if (!pgpt) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, pgpt->GPT_ucMbr,
                               0, 1) != ERROR_NONE) {                   /*  读取 MBR                    */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pgpt->GPT_ucMbr[0x1c2] != 0xee) {                               /*  类型标识错误                */
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, pgpt->GPT_ucBlock,
                               GPT_HEAD_LBA, GPT_HEAD_LBA_CNT) != ERROR_NONE) {
                                                                        /*  读取 GPT 分区表头           */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (pblkd->BLKD_pfuncBlkRd(pblkd, (UINT8 *)pgpt->GPT_entry,
                               GPT_ENT_LBA, GPT_ENT_LBA_CNT) != ERROR_NONE) {
                                                                        /*  读取分区表项                */
        _ErrorHandle(EIO);
        return  (PX_ERROR);
    }

    if (__gptVerify(pgpt) != ERROR_NONE) {
        /*
         *  从备份数据区读取并恢复 GPT 分区表信息
         */
        if (pblkd->BLKD_pfuncBlkRd(pblkd,
                                   pgpt->GPT_ucBlock,
                                   (pblkd->BLKD_ulNSector - GPT_HEAD_LBA_CNT),
                                   GPT_HEAD_LBA_CNT) != ERROR_NONE) {   /*  读取 GPT 备份分区表头       */
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (pblkd->BLKD_pfuncBlkWrt(pblkd, pgpt->GPT_ucBlock,
                                    GPT_HEAD_LBA, GPT_HEAD_LBA_CNT) != ERROR_NONE) {
                                                                        /*  恢复 GPT 分区表头           */
            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (pblkd->BLKD_pfuncBlkRd(pblkd,
                                   (UINT8 *)pgpt->GPT_entry,
                                   (pblkd->BLKD_ulNSector - GPT_ENT_LBA_CNT - GPT_HEAD_LBA_CNT),
                                   GPT_ENT_LBA_CNT) != ERROR_NONE) {    /*  读取备份分区表项            */

            _ErrorHandle(EIO);
            return  (PX_ERROR);
        }

        if (pblkd->BLKD_pfuncBlkWrt(pblkd, (UINT8 *)pgpt->GPT_entry,
                                    GPT_ENT_LBA, GPT_ENT_LBA_CNT) != ERROR_NONE) {
                                                                        /*  恢复分区表项                */
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
