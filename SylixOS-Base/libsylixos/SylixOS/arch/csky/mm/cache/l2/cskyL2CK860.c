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
** 文   件   名: cskyL2CK860.c
**
** 创   建   人: Zhou.Zhijie (周志杰)
**
** 文件创建日期: 2020 年 08 月 21 日
**
** 描        述: C-SKY CK860 体系构架 L2 CACHE 控制器驱动.
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_CSKY_CACHE_L2 > 0
#include "cskyL2.h"
#include "../cskyCache.h"
#include "arch/csky/inc/cskyregs.h"
/*********************************************************************************************************
  相关参数
*********************************************************************************************************/
#define L2C_CACHE_LINE_SIZE     64
#define L1_CACHE_SHIFT          6
#define L1_CACHE_BYTES          (1 << L1_CACHE_SHIFT)
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define CSKY_CACHE_GET_END(pvAdrs, stBytes, ulEnd, uiLineSize)              \
        do {                                                                \
            ulEnd  = (addr_t)((size_t)pvAdrs + stBytes);                    \
            pvAdrs = (PVOID)((addr_t)pvAdrs & ~((addr_t)uiLineSize - 1));   \
        } while (0)
/*********************************************************************************************************
  CACHE 获得 pvAdrs 与 pvEnd 位置
*********************************************************************************************************/
#define CSKY_SYNC_IS()          __asm__ __volatile__ ("sync.is\nsync.is\nsync.is\n" : : : "memory")
/*********************************************************************************************************
  外部函数声明
*********************************************************************************************************/
extern VOID  cskyDCacheClearAll(VOID);
extern VOID  cskyDCacheFlushAll(VOID);
/*********************************************************************************************************
** 函数名称: cskyL2CacheCfgSet
** 功能描述: 初始化 L2CACHE 寄存器 CR23 设置
** 输　入  : uiL2Ctl            CR23 默认值
** 输　出  : 寄存器 CR23 设置
** 全局变量:
** 调用模块:
** 注  意  : 不同 CK860 的芯片配置可能不同，故提供一个配置寄存器的接口
*********************************************************************************************************/
LW_WEAK UINT32  cskyL2CacheCfgSet (UINT32  uiL2Ctl)
{
    uiL2Ctl &= ~(M_L2CACHE_CFG_IPRF | M_L2CACHE_CFG_DLTNCY);
    uiL2Ctl |= M_L2CACHE_CFG_L2EN         |                             /*  L2 CACHE 使能位             */
               M_L2CACHE_CFG_TPRF         |                             /*  L2 CACHE TLB 预取使能位     */
               M_L2CACHE_CFG_RFE          |                             /*  数据访问读分配使能位        */
               (3 << S_L2CACHE_CFG_IPRF)  |                             /*  L2 CACHE 指令预取 3 条缓存行*/
               (1 << S_L2CACHE_CFG_DLTNCY);                             /*  DATA RAM 访问周期 2         */

    return  (uiL2Ctl);
}
/*********************************************************************************************************
** 函数名称: cskyL2CK860Enable
** 功能描述: 使能 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  cskyL2CK860Enable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl;

    __asm__ __volatile__ ("L2cache.iall\n" : : : "memory");             /*  无效 L2 CACHE               */
    __asm__ __volatile__ ("sync.is\n" : : : "memory");

    uiL2Ctl = cskyCCR2Read();

    if (!(uiL2Ctl & M_L2CACHE_CFG_L2EN)) {
        uiL2Ctl = cskyL2CacheCfgSet(uiL2Ctl);
        cskyCCR2Write(uiL2Ctl);
    }
}
/*********************************************************************************************************
** 函数名称: cskyL2CK860Disable
** 功能描述: 禁能 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
** 函数名称: cskyL2CK860IsEnable
** 功能描述: 检查 L2 CACHE 控制器是否使能
** 输　入  : pl2cdrv            驱动结构
** 输　出  : 是否使能
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static BOOL  cskyL2CK860IsEnable (L2C_DRVIER  *pl2cdrv)
{
    UINT32  uiL2Ctl = cskyCCR2Read();
    
    return  ((uiL2Ctl & M_L2CACHE_CFG_L2EN) ? LW_TRUE : LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: cskyL2CK860Sync
** 功能描述: L2 CACHE 同步
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyL2CK860Sync (L2C_DRVIER  *pl2cdrv)
{
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** 函数名称: cskyL2CK860FlushAll
** 功能描述: L2 CACHE 回写所有脏数据
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyL2CK860FlushAll (L2C_DRVIER  *pl2cdrv)
{
    __asm__ __volatile__ ("L2cache.call\n" : : : "memory");
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** 函数名称: __cskyL2CK860Flush
** 功能描述: L2 CACHE 回写脏数据
** 输　入  : ulStart            起始地址
**           ulEnd              结束地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: cskyL2CK860Flush
** 功能描述: L2 CACHE 回写部分脏数据
** 输　入  : pl2cdrv            驱动结构
**           pvPdrs             起始地址
**           stBytes            数据块大小
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: cskyL2CK860InvalidateAll
** 功能描述: L2 CACHE 无效
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyL2CK860InvalidateAll (L2C_DRVIER  *pl2cdrv)
{
    __asm__ __volatile__ ("L2cache.iall\n" : : : "memory");
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** 函数名称: __cskyL2CK860Invalidate
** 功能描述: L2 CACHE 无效
** 输　入  : ulStart            起始地址
**           ulEnd              结束地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: cskyL2CK860Invalidate
** 功能描述: L2 CACHE 无效部分脏数据
** 输　入  : pl2cdrv            驱动结构
**           pvPdrs             起始地址
**           stBytes            数据块大小
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: cskyL2CK860ClearAll
** 功能描述: L2 CACHE 回写并无效
** 输　入  : pl2cdrv            驱动结构
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  cskyL2CK860ClearAll (L2C_DRVIER  *pl2cdrv)
{
    __asm__ __volatile__ ("L2cache.ciall\n" : : : "memory");
    CSKY_SYNC_IS();
}
/*********************************************************************************************************
** 函数名称: __cskyL2CK860Clear
** 功能描述: L2 CACHE 回写并无效
** 输　入  : ulStart            起始地址
**           ulEnd              结束地址
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: cskyL2CK860Clear
** 功能描述: L2 CACHE 回写并无效
** 输　入  : pl2cdrv            驱动结构
**           pvPdrs             起始地址
**           stBytes            数据块大小
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: cskyL2CK860Init
** 功能描述: 初始化 L2 CACHE 控制器
** 输　入  : pl2cdrv            驱动结构
**           uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
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
