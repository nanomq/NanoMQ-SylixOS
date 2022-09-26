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
** ��   ��   ��: cskyL2CK860.c
**
** ��   ��   ��: Zhou.Zhijie (��־��)
**
** �ļ���������: 2020 �� 08 �� 21 ��
**
** ��        ��: C-SKY CK860 ��ϵ���� L2 CACHE ����������.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_CSKY_CACHE_L2 > 0
#include "cskyL2.h"
#include "../cskyCache.h"
#include "arch/csky/inc/cskyregs.h"
/*********************************************************************************************************
  ��ز���
*********************************************************************************************************/
#define L2C_CACHE_LINE_SIZE     64
#define L1_CACHE_SHIFT          6
#define L1_CACHE_BYTES          (1 << L1_CACHE_SHIFT)
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE ��� pvAdrs �� pvEnd λ��
*********************************************************************************************************/
#define CSKY_SYNC_IS()          __asm__ __volatile__ ("sync.is\nsync.is\nsync.is\n" : : : "memory")
/*********************************************************************************************************
  �ⲿ��������
*********************************************************************************************************/
extern VOID  cskyDCacheClearAll(VOID);
extern VOID  cskyDCacheFlushAll(VOID);
/*********************************************************************************************************
** ��������: cskyL2CacheCfgSet
** ��������: ��ʼ�� L2CACHE �Ĵ��� CR23 ����
** �䡡��  : uiL2Ctl            CR23 Ĭ��ֵ
** �䡡��  : �Ĵ��� CR23 ����
** ȫ�ֱ���:
** ����ģ��:
** ע  ��  : ��ͬ CK860 ��оƬ���ÿ��ܲ�ͬ�����ṩһ�����üĴ����Ľӿ�
*********************************************************************************************************/
LW_WEAK UINT32  cskyL2CacheCfgSet (UINT32  uiL2Ctl)
{
    uiL2Ctl &= ~(M_L2CACHE_CFG_IPRF | M_L2CACHE_CFG_DLTNCY);
    uiL2Ctl |= M_L2CACHE_CFG_L2EN         |                             /*  L2 CACHE ʹ��λ             */
               M_L2CACHE_CFG_TPRF         |                             /*  L2 CACHE TLB Ԥȡʹ��λ     */
               M_L2CACHE_CFG_RFE          |                             /*  ���ݷ��ʶ�����ʹ��λ        */
               (3 << S_L2CACHE_CFG_IPRF)  |                             /*  L2 CACHE ָ��Ԥȡ 3 ��������*/
               (1 << S_L2CACHE_CFG_DLTNCY);                             /*  DATA RAM �������� 2         */

    return  (uiL2Ctl);
}
/*********************************************************************************************************
** ��������: cskyL2CK860Enable
** ��������: ʹ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  cskyL2CK860Enable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl;

    __asm__ __volatile__ ("L2cache.iall\n" : : : "memory");             /*  ��Ч L2 CACHE               */
    __asm__ __volatile__ ("sync.is\n" : : : "memory");

    uiL2Ctl = cskyCCR2Read();

    if (!(uiL2Ctl & M_L2CACHE_CFG_L2EN)) {
        uiL2Ctl = cskyL2CacheCfgSet(uiL2Ctl);
        cskyCCR2Write(uiL2Ctl);
    }
}
/*********************************************************************************************************
** ��������: cskyL2CK860Disable
** ��������: ���� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static VOID  cskyL2CK860Disable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl = cskyCCR2Read();
    
    if (uiL2Ctl & M_L2CACHE_CFG_L2EN) {
        uiL2Ctl &= ~M_L2CACHE_CFG_L2EN;
        cskyCCR2Write(uiL2Ctl);
    }
}
/*********************************************************************************************************
** ��������: cskyL2CK860IsEnable
** ��������: ��� L2 CACHE �������Ƿ�ʹ��
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : �Ƿ�ʹ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static BOOL  cskyL2CK860IsEnable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl = cskyCCR2Read();
    
    return  ((uiL2Ctl & M_L2CACHE_CFG_L2EN) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** ��������: cskyL2CK860Sync
** ��������: L2 CACHE ͬ��
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyL2CK860Sync (L2C_DRVIER  *pl2cdrv)
{
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: cskyL2CK860FlushAll
** ��������: L2 CACHE ��д����������
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyL2CK860FlushAll (L2C_DRVIER  *pl2cdrv)
{
    __asm__ __volatile__ ("L2cache.call\n" : : : "memory");
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: __cskyL2CK860Flush
** ��������: L2 CACHE ��д������
** �䡡��  : ulStart            ��ʼ��ַ
**           ulEnd              ������ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __cskyL2CK860Flush (addr_t  ulStart, addr_t  ulEnd)
{
    addr_t  ulAddr = ulStart & ~(L1_CACHE_BYTES - 1);

    for (; ulAddr < ulEnd; ulAddr += L1_CACHE_BYTES) {
        __asm__ __volatile__ ("dcache.cva %0\n"::"r"(ulAddr):"memory");
    }
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: cskyL2CK860Flush
** ��������: L2 CACHE ��д����������
** �䡡��  : pl2cdrv            �����ṹ
**           pvPdrs             ��ʼ��ַ
**           stBytes            ���ݿ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyL2CK860Flush (L2C_DRVIER  *pl2cdrv, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulPhyEnd;

    if (stBytes >= pl2cdrv->L2CD_stSize) {
        cskyDCacheFlushAll();                                       /*  L1 CACHE                        */
        cskyL2CK860FlushAll(pl2cdrv);                               /*  L2 CACHE                        */
    } else {
        CSKY_CACHE_GET_END(pvPdrs, stBytes, ulPhyEnd, L2C_CACHE_LINE_SIZE);
        __cskyL2CK860Flush((addr_t)pvPdrs, ulPhyEnd);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyL2CK860InvalidateAll
** ��������: L2 CACHE ��Ч
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyL2CK860InvalidateAll (L2C_DRVIER  *pl2cdrv)
{
    __asm__ __volatile__ ("L2cache.iall\n" : : : "memory");
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: __cskyL2CK860Invalidate
** ��������: L2 CACHE ��Ч
** �䡡��  : ulStart            ��ʼ��ַ
**           ulEnd              ������ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __cskyL2CK860Invalidate (addr_t  ulStart, addr_t  ulEnd)
{
    addr_t  ulAddr = ulStart & ~(L1_CACHE_BYTES - 1);

    for (; ulAddr < ulEnd; ulAddr += L1_CACHE_BYTES) {
        __asm__ __volatile__ ("dcache.iva %0\n"::"r"(ulAddr):"memory");
    }
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: cskyL2CK860Invalidate
** ��������: L2 CACHE ��Ч����������
** �䡡��  : pl2cdrv            �����ṹ
**           pvPdrs             ��ʼ��ַ
**           stBytes            ���ݿ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyL2CK860Invalidate (L2C_DRVIER  *pl2cdrv, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulPhyStart = (addr_t)pvPdrs;
    addr_t  ulPhyEnd   = ulPhyStart + stBytes;

    if (ulPhyStart & (L2C_CACHE_LINE_SIZE - 1)) {
        ulPhyStart &= ~(L2C_CACHE_LINE_SIZE - 1);
        __asm__ __volatile__ ("dcache.civa %0\n"::"r"(ulPhyStart):"memory");
        ulPhyStart += L2C_CACHE_LINE_SIZE;
    }

    if (ulPhyEnd & (L2C_CACHE_LINE_SIZE - 1)) {
        ulPhyEnd &= ~(L2C_CACHE_LINE_SIZE - 1);
        __asm__ __volatile__ ("dcache.civa %0\n"::"r"(ulPhyEnd):"memory");
    }

    __cskyL2CK860Invalidate(ulPhyStart, ulPhyEnd);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyL2CK860ClearAll
** ��������: L2 CACHE ��д����Ч
** �䡡��  : pl2cdrv            �����ṹ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  cskyL2CK860ClearAll (L2C_DRVIER  *pl2cdrv)
{
    __asm__ __volatile__ ("L2cache.ciall\n" : : : "memory");
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: __cskyL2CK860Clear
** ��������: L2 CACHE ��д����Ч
** �䡡��  : ulStart            ��ʼ��ַ
**           ulEnd              ������ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __cskyL2CK860Clear (addr_t  ulStart, addr_t  ulEnd)
{
    addr_t  ulAddr = ulStart & ~(L1_CACHE_BYTES - 1);

    for (; ulAddr < ulEnd; ulAddr += L1_CACHE_BYTES) {
        __asm__ __volatile__ ("dcache.civa %0\n"::"r"(ulAddr):"memory");
    }
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** ��������: cskyL2CK860Clear
** ��������: L2 CACHE ��д����Ч
** �䡡��  : pl2cdrv            �����ṹ
**           pvPdrs             ��ʼ��ַ
**           stBytes            ���ݿ��С
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  cskyL2CK860Clear (L2C_DRVIER  *pl2cdrv, PVOID  pvPdrs, size_t  stBytes)
{
    addr_t  ulPhyEnd;

    if (stBytes >= pl2cdrv->L2CD_stSize) {
        cskyDCacheClearAll();
        cskyL2CK860ClearAll(pl2cdrv);

    } else {
        CSKY_CACHE_GET_END(pvPdrs, stBytes, ulPhyEnd, L2C_CACHE_LINE_SIZE);
        __cskyL2CK860Clear((addr_t)pvPdrs, ulPhyEnd);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: cskyL2CK860Init
** ��������: ��ʼ�� L2 CACHE ������
** �䡡��  : pl2cdrv            �����ṹ
**           uiInstruction      ָ�� CACHE ����
**           uiData             ���� CACHE ����
**           pcMachineName      ��������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID  cskyL2CK860Init (L2C_DRVIER  *pl2cdrv,
                       CACHE_MODE   uiInstruction,
                       CACHE_MODE   uiData,
                       CPCHAR       pcMachineName)
{
    pl2cdrv->L2CD_pfuncEnable        = cskyL2CK860Enable;
    pl2cdrv->L2CD_pfuncDisable       = cskyL2CK860Disable;
    pl2cdrv->L2CD_pfuncIsEnable      = cskyL2CK860IsEnable;
    pl2cdrv->L2CD_pfuncSync          = cskyL2CK860Sync;
    pl2cdrv->L2CD_pfuncFlush         = cskyL2CK860Flush;
    pl2cdrv->L2CD_pfuncFlushAll      = cskyL2CK860FlushAll;
    pl2cdrv->L2CD_pfuncInvalidate    = cskyL2CK860Invalidate;
    pl2cdrv->L2CD_pfuncInvalidateAll = cskyL2CK860InvalidateAll;
    pl2cdrv->L2CD_pfuncClear         = cskyL2CK860Clear;
    pl2cdrv->L2CD_pfuncClearAll      = cskyL2CK860ClearAll;
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_ARM_CACHE_L2 > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
