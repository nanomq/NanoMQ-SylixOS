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
** 文   件   名: cskyL2.c
**
** 创   建   人: Zhou.Zhijie (周志杰)
**
** 文件创建日期: 2020 年 08 月 21 日
**
** 描        述: C-SKY 体系构架 L2 CACHE 驱动
*********************************************************************************************************/
#define  __SYLIXOS_IO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁减配置
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0 && LW_CFG_CSKY_CACHE_L2 > 0
#include "cskyL2.h"
/*********************************************************************************************************
  L2 CACHE 锁 (多核共享一个 L2 CACHE, 所以操作时需要加自旋锁, 由于外层已经关中断, 这里只需锁自旋锁即可)
*********************************************************************************************************/
static  LW_SPINLOCK_CA_DEFINE_CACHE_ALIGN(l2slca);
#define L2_OP_ENTER()   LW_SPIN_LOCK_IGNIRQ(&l2slca.SLCA_sl)
#define L2_OP_EXIT()    LW_SPIN_UNLOCK_IGNIRQ(&l2slca.SLCA_sl)
/*********************************************************************************************************
  L2 CACHE 驱动
*********************************************************************************************************/
static L2C_DRVIER       l2cdrv;
/*********************************************************************************************************
  L2 CACHE 控制器初始化函数
*********************************************************************************************************/
extern VOID     cskyL2CK860Init(L2C_DRVIER  *pl2cdrv,
                                CACHE_MODE   uiInstruction,
                                CACHE_MODE   uiData,
                                CPCHAR       pcMachineName);
/*********************************************************************************************************
** 函数名称: cskyL2Enable
** 功能描述: 使能 L2 CACHE 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2Enable (VOID)
{
    if (l2cdrv.L2CD_pfuncEnable) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncEnable(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** 函数名称: cskyL2Disable
** 功能描述: 禁能 L2 CACHE 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2Disable (VOID)
{
#if LW_CFG_SMP_EN > 0
    if (l2cdrv.L2CD_pfuncDisable) {
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
    }

#else
    if (l2cdrv.L2CD_pfuncDisable) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncDisable(&l2cdrv);
        L2_OP_EXIT();
    }
#endif
}
/*********************************************************************************************************
** 函数名称: cskyL2IsEnable
** 功能描述: L2 CACHE 是否打开
** 输　入  : NONE
** 输　出  : L2 CACHE 是否打开
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
BOOL cskyL2IsEnable (VOID)
{
    BOOL    bIsEnable;

    if (l2cdrv.L2CD_pfuncIsEnable) {
        L2_OP_ENTER();
        bIsEnable = l2cdrv.L2CD_pfuncIsEnable(&l2cdrv);
        L2_OP_EXIT();

    } else {
        bIsEnable = LW_FALSE;
    }
    
    return  (bIsEnable);
}
/*********************************************************************************************************
** 函数名称: cskyL2Sync
** 功能描述: L2 CACHE 同步
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2Sync (VOID)
{
    if (l2cdrv.L2CD_pfuncSync) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncSync(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** 函数名称: cskyL2FlushAll
** 功能描述: L2 CACHE 回写所有脏数据
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2FlushAll (VOID)
{
    if (l2cdrv.L2CD_pfuncFlushAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncFlushAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** 函数名称: cskyL2Flush
** 功能描述: L2 CACHE 回写部分脏数据
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT cskyL2Flush (PVOID  pvPdrs, size_t  stBytes)
{
    INT  iRet = ERROR_NONE;

    if (l2cdrv.L2CD_pfuncFlush) {
        L2_OP_ENTER();
        iRet = l2cdrv.L2CD_pfuncFlush(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: cskyL2InvalidateAll
** 功能描述: L2 CACHE 无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2InvalidateAll (VOID)
{
    if (l2cdrv.L2CD_pfuncInvalidateAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncInvalidateAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** 函数名称: cskyL2Invalidate
** 功能描述: L2 CACHE 无效
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT cskyL2Invalidate (PVOID  pvPdrs, size_t  stBytes)
{
    INT  iRet = ERROR_NONE;

    if (l2cdrv.L2CD_pfuncInvalidate) {
        L2_OP_ENTER();
        iRet = l2cdrv.L2CD_pfuncInvalidate(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: cskyL2ClearAll
** 功能描述: L2 CACHE 回写并无效
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2ClearAll (VOID)
{
    if (l2cdrv.L2CD_pfuncClearAll) {
        L2_OP_ENTER();
        l2cdrv.L2CD_pfuncClearAll(&l2cdrv);
        L2_OP_EXIT();
    }
}
/*********************************************************************************************************
** 函数名称: cskyL2Clear
** 功能描述: L2 CACHE 回写并无效
** 输　入  : pvPdrs        起始虚拟地址
**           stBytes       数据块大小
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT cskyL2Clear (PVOID  pvPdrs, size_t  stBytes)
{
    INT  iRet = ERROR_NONE;

    if (l2cdrv.L2CD_pfuncClear) {
        L2_OP_ENTER();
        iRet = l2cdrv.L2CD_pfuncClear(&l2cdrv, pvPdrs, stBytes);
        L2_OP_EXIT();
    }

    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: cskyL2Name
** 功能描述: 获得 L2 CACHE 控制器名称
** 输　入  : NONE
** 输　出  : L2 CACHE 控制器名称
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
CPCHAR  cskyL2Name (VOID)
{
    return  (l2cdrv.L2CD_pcName);
}
/*********************************************************************************************************
** 函数名称: cskyL2Init
** 功能描述: 初始化 L2 CACHE 控制器
** 输　入  : uiInstruction      指令 CACHE 类型
**           uiData             数据 CACHE 类型
**           pcMachineName      机器名称
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID cskyL2Init (CACHE_MODE   uiInstruction,
                 CACHE_MODE   uiData,
                 CPCHAR       pcMachineName)
{
    UINT32  uiWays;
    UINT32  uiWaySize;

    LW_SPIN_INIT(&l2slca.SLCA_sl);
    
    if (lib_strcmp(pcMachineName, CSKY_MACHINE_860) == 0) {             /*  CK860 处理器 L2 CACHE       */
        uiWays             = 8;
        uiWaySize          = 32;
        
        l2cdrv.L2CD_pcName = CSKY_MACHINE_860;
        l2cdrv.L2CD_stSize = uiWays * uiWaySize * LW_CFG_KB_SIZE;       /* 256KB 的 8 路组相联 L2 CACHE */

        _DebugFormat(__LOGMESSAGE_LEVEL, "%s %s L2 cache controller initialization.\r\n", 
                     LW_CFG_CPU_ARCH_FAMILY, l2cdrv.L2CD_pcName);
        
        cskyL2CK860Init(&l2cdrv, uiInstruction, uiData, pcMachineName);
    } else {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "unknown machine name.\r\n");
    }


}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
                                                                        /*  LW_CFG_CSKY_CACHE_L2 > 0    */
/*********************************************************************************************************
  END
*********************************************************************************************************/
