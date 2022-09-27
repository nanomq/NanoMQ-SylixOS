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
** ��   ��   ��: oemFdisk.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2015 �� 09 �� 25 ��
**
** ��        ��: OEM ���̷�������.
**
** ע        ��: ��������Ŀ��Ϊ /dev/blk/xxx ���豸�ļ�.
**
** BUG:
2017.07.17  __oemFdisk() ����Թؼ��������������.
2017.09.22  ����������С�����������.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../SylixOS/fs/include/fs_internal.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_OEMDISK_EN > 0
/*********************************************************************************************************
  ����������Ҫ��Ϣ�ص�
*********************************************************************************************************/
#define __DISK_PART_TYPE                    0x4
#define __DISK_PART_STARTSECTOR             0x8
#define __DISK_PART_NSECTOR                 0xc
#define __DISK_PART_OFFSEC                  2048
#define __DISK_PART_MINSEC                  (__DISK_PART_OFFSEC + 128)
/*********************************************************************************************************
** ��������: __oemFdisk
** ��������: �� OEM �����豸���з�������
** �䡡��  : iBlkFd             ���豸�ļ�������
**           ulSecSize          ������С
**           ulTotalSec         ��������
**           fdpPart            ������Ϣ
**           uiNPart            ��������
**           stAlign            �������� (����: SSD ��Ҫ 4K ����)
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __oemFdisk (INT                     iBlkFd,
                        ULONG                   ulSecSize,
                        ULONG                   ulTotalSec,
                        const LW_OEMFDISK_PART  fdpPart[],
                        UINT                    uiNPart,
                        size_t                  stAlign)
{
#ifndef MBR_Table
#define MBR_Table           446                                         /* MBR: Partition table offset  */
#endif
    
    UINT              i;
    UINT8            *pucSecBuf, *pucFillBuf;
    UINT8            *pucPartEntry;
    BOOL              bMaster = LW_FALSE;
    ULONG             ulLeftSec, ulInvSec = ulTotalSec;
    ULONG             ulNSecPerMB = LW_CFG_MB_SIZE / ulSecSize;
    UINT64            u64Temp;
    UINT32            uiPSecNext;
    UINT32            uiPStartSec[4], uiPNSec[4];
    
    UINT              uiHdStart,  uiHdEnd;
    UINT              uiCylStart, uiCylEnd;
    UINT              uiSecStart, uiSecEnd;
    
    ulTotalSec -= __DISK_PART_OFFSEC;                                   /*  ��Ч����������              */
    uiPSecNext  = __DISK_PART_OFFSEC;
    ulLeftSec   = ulTotalSec;
    
    for (i = 0; i < uiNPart; i++) {                                     /*  ȷ������ LBA ��Ϣ           */
        uiPStartSec[i] = uiPSecNext;
        if (fdpPart[i].FDP_ucSzPct == 0) {                              /*  ʣ������                    */
            uiPNSec[i] = (UINT32)ulLeftSec;
            break;
            
        } else if (fdpPart[i].FDP_ucSzPct > 100) {                      /*  ��������Ϣ����              */
            uiPNSec[i] = fdpPart[i].FDP_ulMBytes * ulNSecPerMB;

        } else {                                                        /*  ���ٷֱȷ���                */
            u64Temp    = (UINT64)ulTotalSec;
            u64Temp    = (u64Temp * fdpPart[i].FDP_ucSzPct) / 100;
            uiPNSec[i] = (UINT32)u64Temp;
        }

        uiPNSec[i] = ROUND_DOWN(uiPNSec[i], (stAlign / ulSecSize));     /*  �������������              */
        if (ulLeftSec < uiPNSec[i]) {                                   /*  ����Խ��                    */
            _ErrorHandle(ENOSPC);
            return  (PX_ERROR);
        }

        ulLeftSec  -= uiPNSec[i];
        uiPSecNext += uiPNSec[i];
    }

    pucSecBuf = (UINT8 *)__SHEAP_ALLOC((size_t)ulSecSize << 1);
    if (pucSecBuf == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    pucFillBuf = pucSecBuf + ulSecSize;
    lib_memset(pucFillBuf, 0xff, (size_t)ulSecSize);                    /*  �� 0xff �����������        */
    if (ulInvSec > 2050) {
        pwrite(iBlkFd, pucFillBuf,
               (size_t)ulSecSize, (2050 * (off_t)ulSecSize));           /*  ȥ�� Linux �ļ�ϵͳ��Ϣ     */
    }

    if (pread(iBlkFd, pucSecBuf, (size_t)ulSecSize, 0) != (ssize_t)ulSecSize) {
        __SHEAP_FREE(pucSecBuf);
        return  (PX_ERROR);
    }
    
    pucPartEntry = &pucSecBuf[MBR_Table];
    lib_bzero(pucPartEntry, 16 * 4);
    
    for (i = 0; i < uiNPart; i++, pucPartEntry += 16) {
        if (fdpPart[i].FDP_bIsActive && (bMaster == LW_FALSE)) {
            bMaster         = LW_TRUE;
            pucPartEntry[0] = 0x80;                                     /*  �����                    */
        
        } else {
            pucPartEntry[0] = 0x00;
        }

        pucPartEntry[4] = fdpPart[i].FDP_ucPartType;                    /*  �����ļ�ϵͳ                */
        
        /*
         *  CS = 0, HS = 0, SS = 1, PS = 63, PH = 255
         *
         *  H = (LBA DIV PS) MOD PH + HS
         *  C = LBA DIV (PH*PS) + CS
         *  S = LBA MOD PS + SS
         */
        uiHdStart  = (uiPStartSec[i] / 63) % 255 + 0;
        uiCylStart = uiPStartSec[i] / (255 * 63) + 0;
        uiSecStart = (uiPStartSec[i] % 63) + 1;
        
        uiHdEnd  = ((uiPStartSec[i] + uiPNSec[i] - 1) / 63) % 255 + 0;
        uiCylEnd = (uiPStartSec[i] + uiPNSec[i] - 1) / (255 * 63) + 0;
        uiSecEnd = ((uiPStartSec[i] + uiPNSec[i] - 1) % 63) + 1;
        
        pucPartEntry[1] = (BYTE)uiHdStart;                              /*  Start head                  */
        pucPartEntry[2] = (BYTE)((uiCylStart >> 2) + uiSecStart);       /*  Start sector                */
        pucPartEntry[3] = (BYTE)uiCylStart;                             /*  Start cylinder              */

        pucPartEntry[5] = (BYTE)uiHdEnd;                                /*  End head                    */
        pucPartEntry[6] = (BYTE)((uiCylEnd >> 2) + uiSecEnd);           /*  End sector                  */
        pucPartEntry[7] = (BYTE)uiCylEnd;                               /*  End cylinder                */

        BLK_ST_DWORD(pucPartEntry +  8, uiPStartSec[i]);                /*  Start sector in LBA         */
        BLK_ST_DWORD(pucPartEntry + 12, uiPNSec[i]);                    /*  Partition size              */

        pwrite(iBlkFd, pucFillBuf, 
               (size_t)ulSecSize, (uiPStartSec[i] * (off_t)ulSecSize)); /*  ȥ���ļ�ϵͳ��Ϣ            */

        if (fdpPart[i].FDP_ucSzPct == 0) {
            i++;
            break;
        }
    }
    
    pucSecBuf[ulSecSize - 2] = 0x55;
    pucSecBuf[ulSecSize - 1] = 0xaa;

    if (pwrite(iBlkFd, pucSecBuf, (size_t)ulSecSize, 0) != (ssize_t)ulSecSize) {
        __SHEAP_FREE(pucSecBuf);
        return  (PX_ERROR);
    }

    __SHEAP_FREE(pucSecBuf);
    return  ((INT)i);
}
/*********************************************************************************************************
** ��������: __oemGptFdisk
** ��������: �� OEM �����豸���� GPT ��������
** �䡡��  : iBlkFd             ���豸�ļ�������
**           ulSecSize          ������С
**           ulTotalSec         ��������
**           ui64FirstLba       FirstLbaƫ�ƣ� Ϊ 0 ��ʹ��Ĭ��ƫ��
**           fdpPart            ������Ϣ
**           uiNPart            ��������
**           stAlign            �������� (����: SSD ��Ҫ 4K ����)
** �䡡��  : ��������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT > 32

static INT  __oemGptFdisk (INT                     iBlkFd,
                           ULONG                   ulSecSize,
                           ULONG                   ulTotalSec,
                           UINT64                  ui64FirstLba,
                           const LW_OEMFDISK_PART  fdpPart[],
                           UINT                    uiNPart,
                           size_t                  stAlign)
{
    UINT64 ui64PSecNext;
    UINT64 ui64PStartSec;
    UINT64 ui64lPNSec;
    UINT64 ui64LeftSec;
    INT    i;
    ULONG  ulNSecPerMB = LW_CFG_MB_SIZE / ulSecSize;

    GPT_TABLE *pgpt;

    if (ui64FirstLba == 0) {
        ui64FirstLba = ROUND_UP(GPT_DATA_LBA, (stAlign / ulSecSize));
    }

    pgpt = API_GptCreateAndInitWithLba(ulSecSize, ulTotalSec, ui64FirstLba);
    if (!pgpt) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    ui64PSecNext  = pgpt->GPT_header.HDR_ui64FirstLba;
    ui64LeftSec   = pgpt->GPT_header.HDR_ui64LastLba - pgpt->GPT_header.HDR_ui64FirstLba;

    for (i = 0; i < uiNPart; i++) {                                     /*  ȷ������ LBA ��Ϣ           */
        ui64PStartSec = ui64PSecNext;
        if (ui64PStartSec > pgpt->GPT_header.HDR_ui64LastLba) {
            API_GptDestroy(pgpt);
            return  (PX_ERROR);
        }

        if (fdpPart[i].FDP_ucSzPct == 0) {                              /*  ʣ������                    */
            ui64lPNSec = ui64LeftSec;

        } else if (fdpPart[i].FDP_ucSzPct > 100) {                      /*  ��������Ϣ����              */
            ui64lPNSec = fdpPart[i].FDP_ulMBytes * ulNSecPerMB;

        } else {                                                        /*  ���ٷֱȷ���                */
            ui64lPNSec = ulTotalSec;
            ui64lPNSec = (ui64lPNSec * fdpPart[i].FDP_ucSzPct) / 100;
        }

        if ((ui64PStartSec + ui64lPNSec) > pgpt->GPT_header.HDR_ui64LastLba) {
            ui64lPNSec = pgpt->GPT_header.HDR_ui64LastLba - ui64PStartSec;
        }

        ui64lPNSec    = ROUND_DOWN(ui64lPNSec, (stAlign / ulSecSize));  /*  �������������              */
        ui64LeftSec  -= ui64lPNSec;
        ui64PSecNext += ui64lPNSec;

        if (API_GptAddEntryWithName(pgpt, ui64PStartSec, ui64lPNSec,    /*  ��ӵ� GPT ������           */
                                    fdpPart[i].FDP_ucPartType,
                                    (CPCHAR)fdpPart[i].FDP_pcName) != ERROR_NONE) {
            API_GptDestroy(pgpt);
            return  (PX_ERROR);
        }

        if (fdpPart[i].FDP_ucSzPct == 0) {
            i++;
            break;
        }
    }

    if (API_GptPartitionSave(iBlkFd, pgpt) != ERROR_NONE) {             /*  д����������豸          */
        API_GptDestroy(pgpt);
        return  (PX_ERROR);
    }

    API_GptDestroy(pgpt);

    return  ((INT)i);
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT > 32 */
/*********************************************************************************************************
** ��������: API_OemFdiskEx
** ��������: �� OEM �����豸���з�������
** �䡡��  : pcBlkDev           ���豸�ļ� ����: /dev/blk/sata0
**           fdpPart            ��������
**           uiNPart            ������������
**           stAlign            �������� (����: SSD ��Ҫ 4K ����)
**           bGpt               �Ƿ� GPT ����
** �䡡��  : �����ķ�������, PX_ERROR ��ʾ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemFdiskEx (CPCHAR                  pcBlkDev,
                     const LW_OEMFDISK_PART  fdpPart[],
                     UINT                    uiNPart,
                     size_t                  stAlign,
                     BOOL                    bGpt)
{
    return API_OemFdiskExWithLba(pcBlkDev, fdpPart, uiNPart, stAlign, bGpt, 0);
}
/*********************************************************************************************************
** ��������: API_OemFdiskExWithLba
** ��������: �� OEM �����豸���з�������
** �䡡��  : pcBlkDev           ���豸�ļ� ����: /dev/blk/sata0
**           fdpPart            ��������
**           uiNPart            ������������
**           stAlign            �������� (����: SSD ��Ҫ 4K ����)
**           bGpt               �Ƿ� GPT ����
**           ui64FirstLba       GPT ���� FirstLba ƫ�ƣ� Ϊ 0 ��ʹ��Ĭ��ƫ��
** �䡡��  : �����ķ�������, PX_ERROR ��ʾ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemFdiskExWithLba (CPCHAR                  pcBlkDev,
                            const LW_OEMFDISK_PART  fdpPart[],
                            UINT                    uiNPart,
                            size_t                  stAlign,
                            BOOL                    bGpt,
                            UINT64                  ui64FirstLba)
{
    INT         i;
    INT         iBlkFd;
    INT         iNCnt;
    struct stat statGet;
    ULONG       ulSecSize;
    ULONG       ulTotalSec;

    if (!pcBlkDev || !fdpPart || !uiNPart) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (!bGpt && (uiNPart > 4)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
#if LW_CFG_CPU_WORD_LENGHT == 32
    if (bGpt) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "32bits CPU no support GPT partition.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT == 32*/

    if ((stAlign < 4096) || (stAlign & (stAlign - 1))) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "stAlign error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (geteuid()) {
        _ErrorHandle(EACCES);
        return  (PX_ERROR);
    }

    iBlkFd = open(pcBlkDev, O_RDWR);
    if (iBlkFd < 0) {
        return  (PX_ERROR);
    }

    if (fstat(iBlkFd, &statGet) ||
        !S_ISBLK(statGet.st_mode)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (ioctl(iBlkFd, LW_BLKD_GET_SECNUM, &ulTotalSec)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (!bGpt && (ulTotalSec < __DISK_PART_OFFSEC)) {
        close(iBlkFd);
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    if (ioctl(iBlkFd, LW_BLKD_GET_SECSIZE, &ulSecSize)) {
        ulSecSize = 512;

    } else if ((ulSecSize < 512) || !ALIGNED(ulSecSize, ulSecSize - 1)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    for (i = 0; i < uiNPart; i++) {
        if (fdpPart[i].FDP_ucSzPct == 0) {
            i++;
            break;
        }
    }

    uiNPart = i;
#if LW_CFG_CPU_WORD_LENGHT > 32
    if (bGpt) {                                                         /*  �� GPT ��ʽ�Կ��豸����     */
        iNCnt = __oemGptFdisk(iBlkFd, ulSecSize, ulTotalSec, ui64FirstLba, fdpPart, uiNPart, stAlign);
    } else
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT > 32 */
    {
        iNCnt = __oemFdisk(iBlkFd, ulSecSize, ulTotalSec, fdpPart, uiNPart, stAlign);
    }
    if (iNCnt > 0) {
        fsync(iBlkFd);
    }
    close(iBlkFd);

    return  (iNCnt);
}
/*********************************************************************************************************
** ��������: API_OemFdisk
** ��������: �� OEM �����豸���з�������
** �䡡��  : pcBlkDev           ���豸�ļ� ����: /dev/blk/sata0
**           fdpPart            ��������
**           uiNPart            ������������
**           stAlign            �������� (����: SSD ��Ҫ 4K ����)
** �䡡��  : �����ķ�������, PX_ERROR ��ʾ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemFdisk (CPCHAR                  pcBlkDev,
                   const LW_OEMFDISK_PART  fdpPart[],
                   UINT                    uiNPart,
                   size_t                  stAlign)
{
    return  (API_OemFdiskEx(pcBlkDev, fdpPart, uiNPart, stAlign, LW_FALSE));
}
/*********************************************************************************************************
** ��������: __oemFdiskGet
** ��������: ��ȡ OEM �����豸�������
** �䡡��  : iBlkFd             ���豸�ļ�������
**           ulSecStart         ��ʼ����
**           ulSecExtStart      ��չ��ʼ����
**           ulSecSize          ������С
**           uiNCnt             ��������
**           fdpInfo            ��ȡ��Ϣ����
**           uiNPart            �������ɱ��������Ϣ����
** �䡡��  : ��ȡ�ķ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __oemFdiskGet (INT                  iBlkFd,
                           ULONG                ulSecStart,
                           ULONG                ulSecExtStart,
                           ULONG                ulSecSize,
                           UINT                 uiNCnt,
                           LW_OEMFDISK_PINFO    fdpInfo[],
                           UINT                 uiNPart)
{
    INT                 i;
    UINT8              *pucSecBuf;
    off_t               oft = ((off_t)ulSecSize * ulSecStart);
    PLW_OEMFDISK_PINFO  pfdpInfo;
    BYTE                ucActiveFlag;
    BYTE                ucPartType;
    INT                 iPartInfoStart;
    INT                 iRet;
    ULONG               ulSSec;
    ULONG               ulNSec;

    pucSecBuf = (UINT8 *)__SHEAP_ALLOC((size_t)ulSecSize);
    if (pucSecBuf == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    if (pread(iBlkFd, pucSecBuf, (size_t)ulSecSize, oft) != (ssize_t)ulSecSize) {
        __SHEAP_FREE(pucSecBuf);
        return  (PX_ERROR);
    }

    if ((pucSecBuf[ulSecSize - 2] != 0x55) ||
        (pucSecBuf[ulSecSize - 1] != 0xaa)) {                           /*  ����������־�Ƿ���ȷ        */
        __SHEAP_FREE(pucSecBuf);
        _ErrorHandle(EFORMAT);
        return  (PX_ERROR);
    }

    for (i = 0; i < 4; i++) {                                           /*  �鿴����������Ϣ            */
        iPartInfoStart = MBR_Table + (i * 16);                          /*  ��ȡ������Ϣ��ʼ��          */

        ucActiveFlag = pucSecBuf[iPartInfoStart];                       /*  �����־                    */
        ucPartType   = pucSecBuf[iPartInfoStart + __DISK_PART_TYPE];    /*  �����ļ�ϵͳ����            */

        if (LW_DISK_PART_IS_VALID(ucPartType)) {
            pfdpInfo = &fdpInfo[uiNCnt];                                /*  �߼�������Ϣ                */
            ulSSec   = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_STARTSECTOR]);
            ulNSec   = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_NSECTOR]);

            if (ucActiveFlag == 0x80) {
                pfdpInfo->FDP_bIsActive = LW_TRUE;
            } else {
                pfdpInfo->FDP_bIsActive = LW_FALSE;
            }

            pfdpInfo->FDP_ucPartType = ucPartType;
            pfdpInfo->FDP_u64Size    = ((UINT64)ulNSec * ulSecSize);
            pfdpInfo->FDP_u64Oft     = ((UINT64)ulSSec * ulSecSize);

            uiNCnt++;

        } else if (LW_DISK_PART_IS_EXTENDED(ucPartType)) {
            if (ulSecStart == 0ul) {                                    /*  �Ƿ�λ����������            */
                ulSecExtStart = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_STARTSECTOR]);
                ulSecStart    = ulSecExtStart;
            } else {                                                    /*  λ����չ����������          */
                ulSecStart  = BLK_LD_DWORD(&pucSecBuf[iPartInfoStart + __DISK_PART_STARTSECTOR]);
                ulSecStart += ulSecExtStart;
            }

            iRet = __oemFdiskGet(iBlkFd, ulSecStart, ulSecExtStart,
                                 ulSecSize, uiNCnt, fdpInfo, uiNPart);
            if (iRet < 0) {
                __SHEAP_FREE(pucSecBuf);
                return  ((INT)uiNCnt);
            } else {
                uiNCnt = (UINT)iRet;
            }
        }

        if (uiNCnt >= uiNPart) {                                        /*  �����ٱ�����������Ϣ      */
            __SHEAP_FREE(pucSecBuf);
            return  ((INT)uiNCnt);
        }
    }

    __SHEAP_FREE(pucSecBuf);
    if (uiNCnt == 0) {                                                  /*  û�м�⵽�κη���          */
        return  (PX_ERROR);
    } else {
        return  ((INT)uiNCnt);                                          /*  ��⵽�ķ�������            */
    }
}
/*********************************************************************************************************
** ��������: __oemGptFdiskGet
** ��������: ��ȡ OEM �����豸 GPT �������
** �䡡��  : iBlkFd             ���豸�ļ�������
**           fdpInfo            ��ȡ��Ϣ����
**           uiNPart            �������ɱ��������Ϣ����
**           ulSecSize          ������С
** �䡡��  : ��ȡ�ķ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_WORD_LENGHT > 32

static INT  __oemGptFdiskGet (INT                  iBlkFd,
                              LW_OEMFDISK_PINFO    fdpInfo[],
                              UINT                 uiNPart,
                              ULONG                ulSecSize)
{
    UINT64 ui64PStartSec;
    UINT64 ui64lPNSec;
    INT    i;

    GPT_TABLE *pgpt;

    pgpt = API_GptCreateAndInit(ulSecSize, 0);
    if (!pgpt) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    if (API_GptPartitionLoad(iBlkFd, pgpt) != ERROR_NONE) {             /*  ���� GPT ������Ϣ           */
        API_GptDestroy(pgpt);
        return  (PX_ERROR);
    }

    if (uiNPart > pgpt->GPT_header.HDR_uiEntriesCount) {
        uiNPart = pgpt->GPT_header.HDR_uiEntriesCount;
    }

    for (i = 0; i < uiNPart; i++) {                                     /*  ת��������                  */
        if (API_GptGetEntry(pgpt, i, &ui64PStartSec,
                            &ui64lPNSec, &fdpInfo[i].FDP_ucPartType) != ERROR_NONE) {
            API_GptDestroy(pgpt);
            return  (PX_ERROR);
        }

        if (fdpInfo[i].FDP_ucPartType == LW_DISK_PART_TYPE_EFI) {
            fdpInfo[i].FDP_bIsActive = LW_TRUE;
        } else {
            fdpInfo[i].FDP_bIsActive = LW_FALSE;
        }
        fdpInfo[i].FDP_u64Size   = ((UINT64)ui64lPNSec * ulSecSize);
        fdpInfo[i].FDP_u64Oft    = ((UINT64)ui64PStartSec * ulSecSize);
    }

    API_GptDestroy(pgpt);

    return  (i);
}

#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT > 32 */
/*********************************************************************************************************
** ��������: API_OemFdiskGet
** ��������: ��ȡ OEM �����豸�������
** �䡡��  : pcBlkDev           ���豸�ļ� ����: /dev/blk/sata0
**           fdpInfo            ��ȡ��Ϣ����
**           uiNPart            �������ɱ��������Ϣ����
**           pbGpt              �����Ƿ� GPT ����
** �䡡��  : ��ȡ�ķ�������, PX_ERROR ��ʾ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemFdiskGetEx (CPCHAR  pcBlkDev, LW_OEMFDISK_PINFO  fdpInfo[], UINT  uiNPart, BOOL  *pbGpt)
{
    INT         iBlkFd;
    INT         iNCnt;
    struct stat statGet;
    ULONG       ulSecSize;

    if (!pcBlkDev || !fdpInfo || !uiNPart) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    iBlkFd = open(pcBlkDev, O_RDONLY);
    if (iBlkFd < 0) {
        return  (PX_ERROR);
    }

    if (fstat(iBlkFd, &statGet) ||
        !S_ISBLK(statGet.st_mode)) {
        close(iBlkFd);
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (ioctl(iBlkFd, LW_BLKD_GET_SECSIZE, &ulSecSize)) {
        ulSecSize = 512;
    }

#if LW_CFG_CPU_WORD_LENGHT > 32
    iNCnt = __oemGptFdiskGet(iBlkFd, fdpInfo, uiNPart, ulSecSize);      /*  ���ȳ��� GPT ������ʽɨ��   */
    if (iNCnt < 0) {
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT > 32 */
        iNCnt  = __oemFdiskGet(iBlkFd, 0, 0, ulSecSize, 0, fdpInfo, uiNPart);
        if (pbGpt) {
            *pbGpt = LW_FALSE;
        }
#if LW_CFG_CPU_WORD_LENGHT > 32
    } else {
        if (pbGpt) {
            *pbGpt = LW_TRUE;
        }
    }
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT > 32 */

    close(iBlkFd);

    return  (iNCnt);
}
/*********************************************************************************************************
** ��������: API_OemFdiskGet
** ��������: ��ȡ OEM �����豸�������
** �䡡��  : pcBlkDev           ���豸�ļ� ����: /dev/blk/sata0
**           fdpInfo            ��ȡ��Ϣ����
**           uiNPart            �������ɱ��������Ϣ����
** �䡡��  : ��ȡ�ķ�������, PX_ERROR ��ʾ����.
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemFdiskGet (CPCHAR  pcBlkDev, LW_OEMFDISK_PINFO  fdpInfo[], UINT  uiNPart)
{
    return  (API_OemFdiskGetEx(pcBlkDev, fdpInfo, uiNPart, LW_NULL));
}
/*********************************************************************************************************
** ��������: __oemFdiskGetType
** ��������: ��ô��̷��������ֶ�
** �䡡��  : ucType             ��������
** �䡡��  : ���������ִ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static CPCHAR  __oemFdiskGetType (UINT8  ucType)
{
    switch (ucType) {

    case 0x00:  return  ("Empty (NULL) Partition");
    case 0x01:  return  ("MSDOS Partition 12-bit FAT");
    case 0x02:  return  ("XENIX / (slash) Partition");
    case 0x03:  return  ("XENIX /usr Partition");
    case 0x04:  return  ("MSDOS 16-bit FAT <32M Partition");
    case 0x05:  return  ("MSDOS Extended Partition");
    case 0x06:  return  ("MSDOS 16-bit FAT >=32M Partition");
    case 0x07:  return  ("HPFS / NTFS Partition");
    case 0x08:  return  ("AIX boot or SplitDrive Partition");
    case 0x09:  return  ("AIX data or Coherent Partition");
    case 0x0a:  return  ("OS/2 Boot Manager Partition");
    case 0x0b:  return  ("Win95 FAT32 Partition");
    case 0x0c:  return  ("Win95 FAT32 (LBA) Partition");
    case 0x0e:  return  ("Win95 FAT16 (LBA) Partition");
    case 0x0f:  return  ("Win95 Extended (LBA) Partition");
    case 0x10:  return  ("OPUS Partition");
    case 0x11:  return  ("Hidden DOS FAT12 Partition");
    case 0x12:  return  ("Compaq diagnostics Partition");
    case 0x14:  return  ("Hidden DOS FAT16 Partition");
    case 0x16:  return  ("Hidden DOS FAT16 (big) Partition");
    case 0x17:  return  ("Hidden HPFS/NTFS Partition");
    case 0x18:  return  ("AST Windows swapfile Partition");
    case 0x24:  return  ("NEC DOS Partition");
    case 0x3c:  return  ("PartitionMagic recovery Partition");
    case 0x40:  return  ("Venix 80286 Partition");
    case 0x41:  return  ("Linux/MINIX (shared with DRDOS) Partition");
    case 0x42:  return  ("SFS or Linux swap part (shared with DRDOS)");
    case 0x43:  return  ("Linux native (shared with DRDOS) Partition");
    case 0x50:  return  ("DM (disk manager) Partition");
    case 0x51:  return  ("DM6 Aux1 (or Novell) Partition");
    case 0x52:  return  ("CP/M or Microport SysV/AT Partition");
    case 0x53:  return  ("DM6 Aux3 Partition");
    case 0x54:  return  ("DM6 Partition");
    case 0x55:  return  ("EZ-Drive (disk manager) Partition");
    case 0x56:  return  ("Golden Bow (disk manager) Partition");
    case 0x5c:  return  ("Priam Edisk (disk manager) Partition");
    case 0x61:  return  ("SpeedStor Partition");
    case 0x63:  return  ("GNU HURD or Mach or Sys V/386 (ISC UNIX)");
    case 0x64:  return  ("Novell Netware 286 Partition");
    case 0x65:  return  ("Novell Netware 386 Partition");
    case 0x70:  return  ("DiskSecure Multi-Boot Partition");
    case 0x75:  return  ("PC/IX Partition");
    case 0x77:  return  ("QNX4.x Partition");
    case 0x78:  return  ("QNX4.x 2nd part Partition");
    case 0x79:  return  ("QNX4.x 3rd part Partition");
    case 0x80:  return  ("MINIX until 1.4a Partition");
    case 0x81:  return  ("MINIX / old Linux Partition");
    case 0x82:  return  ("Linux swap Partition");
    case 0x83:  return  ("Linux native Partition");
    case 0x84:  return  ("OS/2 hidden C: drive Partition");
    case 0x85:  return  ("Linux extended Partition");
    case 0x86:  return  ("NTFS volume set Partition");
    case 0x87:  return  ("NTFS volume set Partition");
    case 0x93:  return  ("Amoeba Partition");
    case 0x94:  return  ("Amoeba bad block table Partition");
    case 0x96:  return  ("ISO-9660 file system");
    case 0x9c:  return  ("SylixOS True Power Safe Partition");
    case 0x9d:  return  ("EFI Partition");
    case 0xa0:  return  ("IBM Thinkpad hibernation Partition");
    case 0xa5:  return  ("BSD/386 Partition");
    case 0xa7:  return  ("NeXTSTEP 486 Partition");
    case 0xb1:  return  ("QNX6.x Partition");
    case 0xb2:  return  ("QNX6.x 2nd part Partition");
    case 0xb3:  return  ("QNX6.x 3rd part Partition");
    case 0xb7:  return  ("BSDI fs Partition");
    case 0xb8:  return  ("BSDI swap Partition");
    case 0xbe:  return  ("Solaris boot partition");
    case 0xbf:  return  ("Reserved partition");
    case 0xc0:  return  ("DRDOS/Novell DOS secured partition");
    case 0xc1:  return  ("DRDOS/sec (FAT-12) Partition");
    case 0xc4:  return  ("DRDOS/sec (FAT-16, < 32M) Partition");
    case 0xc6:  return  ("DRDOS/sec (FAT-16, >= 32M) Partition");
    case 0xc7:  return  ("Syrinx Partition");
    case 0xdb:  return  ("CP/M-Concurrent CP/M-Concurrent DOS-CTOS");
    case 0xe1:  return  ("DOS access-SpeedStor 12-bit FAT ext.");
    case 0xe3:  return  ("DOS R/O or SpeedStor Partition");
    case 0xe4:  return  ("SpeedStor 16-bit FAT Ext Part. < 1024 cyl.");
    case 0xee:  return  ("GPT Partition");
    case 0xf1:  return  ("SpeedStor Partition");
    case 0xf2:  return  ("DOS 3.3+ secondary Partition");
    case 0xf4:  return  ("SpeedStor large partition Partition");
    case 0xfe:  return  ("SpeedStor >1024 cyl. or LANstep Partition");
    case 0xff:  return  ("Xenix Bad Block Table Partition");

    default:    break;
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: API_OemFdiskShow
** ��������: ��ӡ OEM �����豸�������
** �䡡��  : pcBlkDev           ���豸�ļ� ����: /dev/blk/sata0
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_OemFdiskShow (CPCHAR  pcBlkDev)
{
    static PCHAR         pcPartInfo = "\n"\
    "PART ACT  SIZE(KB)  OFFSET(KB)             TYPE\n"
    "---- --- ---------- ---------- -------------------------------------\n";

    PLW_OEMFDISK_PINFO   fdpInfo;
    LW_BLK_INFO          blkinfo;
    INT                  iRet;
    INT                  i, iBlkFd;
    PCHAR                pcAct;
    CPCHAR               pcType;
    BOOL                 bGpt;

    fdpInfo = (PLW_OEMFDISK_PINFO)__SHEAP_ALLOC(sizeof(LW_OEMFDISK_PINFO) *
                                                LW_CFG_MAX_DISKPARTS);
    if (fdpInfo == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    
    iBlkFd = open(pcBlkDev, O_RDONLY);
    if (iBlkFd < ERROR_NONE) {
        fprintf(stderr, "can not open %s: %s\n", pcBlkDev, lib_strerror(errno));
        __SHEAP_FREE(fdpInfo);
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iBlkFd, LW_BLKD_CTRL_INFO, &blkinfo);
    if (iRet < ERROR_NONE) {
        blkinfo.BLKI_uiType = LW_BLKD_CTRL_INFO_TYPE_UNKOWN;
        lib_strcpy(blkinfo.BLKI_cSerial,   "N/A");
        lib_strcpy(blkinfo.BLKI_cFirmware, "N/A");
        lib_strcpy(blkinfo.BLKI_cProduct,  "N/A");
        lib_strcpy(blkinfo.BLKI_cMedia,    "N/A");
    
    } else {
        if (blkinfo.BLKI_cSerial[0] == PX_EOS) {
            lib_strcpy(blkinfo.BLKI_cSerial, "N/A");
        }
        if (blkinfo.BLKI_cFirmware[0] == PX_EOS) {
            lib_strcpy(blkinfo.BLKI_cFirmware, "N/A");
        }
        if (blkinfo.BLKI_cProduct[0] == PX_EOS) {
            lib_strcpy(blkinfo.BLKI_cProduct, "N/A");
        }
        if (blkinfo.BLKI_cMedia[0] == PX_EOS) {
            lib_strcpy(blkinfo.BLKI_cMedia, "N/A");
        }
    }
    close(iBlkFd);

    printf("block device  : %s\n", pcBlkDev);
    printf("block type    : ");
    switch (blkinfo.BLKI_uiType) {
    
    case LW_BLKD_CTRL_INFO_TYPE_RAMDISK:        printf("RAMDISK\n");        break;
    case LW_BLKD_CTRL_INFO_TYPE_BLKRAW:         printf("BLKRAW\n");         break;
    case LW_BLKD_CTRL_INFO_TYPE_ATA:            printf("ATA\n");            break;
    case LW_BLKD_CTRL_INFO_TYPE_SATA:           printf("SATA\n");           break;
    case LW_BLKD_CTRL_INFO_TYPE_SCSI:           printf("SCSI\n");           break;
    case LW_BLKD_CTRL_INFO_TYPE_SAS:            printf("SAS(SCSI)\n");      break;
    case LW_BLKD_CTRL_INFO_TYPE_UFS:            printf("UFS(SCSI)\n");      break;
    case LW_BLKD_CTRL_INFO_TYPE_NVME:           printf("NVMe\n");           break;
    case LW_BLKD_CTRL_INFO_TYPE_SDMMC:          printf("SD/MMC\n");         break;
    case LW_BLKD_CTRL_INFO_TYPE_MSTICK:         printf("MEMORY-STICK\n");   break;
    case LW_BLKD_CTRL_INFO_TYPE_USB:            printf("USB\n");            break;
    default:                                    printf("<unkown>\n");       break;
    }
    
    printf("block serial  : %s\n", blkinfo.BLKI_cSerial);
    printf("block firmware: %s\n", blkinfo.BLKI_cFirmware);
    printf("block product : %s\n", blkinfo.BLKI_cProduct);
    printf("block media   : %s\n", blkinfo.BLKI_cMedia);

    iRet = API_OemFdiskGetEx(pcBlkDev, fdpInfo, LW_CFG_MAX_DISKPARTS, &bGpt);
    if (iRet < ERROR_NONE) {
        fprintf(stderr, "\ncan not analysis partition table: %s\n", lib_strerror(errno));
        __SHEAP_FREE(fdpInfo);
        return  (iRet);

    } else if (iRet == 0) {
        __SHEAP_FREE(fdpInfo);
        fprintf(stderr, "\nblock device: %s do not have MBR partition table.\n", pcBlkDev);
        return  (iRet);
    }

    printf("\n%s partition >>\n%s", (bGpt ? "GPT" : "MBR"), pcPartInfo);
    for (i = 0; i < iRet; i++) {
        pcAct  = (fdpInfo[i].FDP_bIsActive) ? "*" : "";
        pcType = __oemFdiskGetType(fdpInfo[i].FDP_ucPartType);
        if (pcType) {
            printf("%4d  %1s  %10llu %10llu %s\n",
                   i, pcAct,
                   (fdpInfo[i].FDP_u64Size >> 10),
                   (fdpInfo[i].FDP_u64Oft >> 10),
                   pcType);
        } else {
            printf("%4d  %1s  %10llu %10llu 0x%02x\n",
                   i, pcAct,
                   (fdpInfo[i].FDP_u64Size >> 10),
                   (fdpInfo[i].FDP_u64Oft >> 10),
                   fdpInfo[i].FDP_ucPartType);
        }
    }

    printf("\ntotal partition %d\n", iRet);

    __SHEAP_FREE(fdpInfo);

    return  (ERROR_NONE);
}

#endif                                                                  /*  LW_CFG_OEMDISK_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
