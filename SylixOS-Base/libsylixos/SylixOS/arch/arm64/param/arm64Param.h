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
** 文   件   名: arm64Param.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 06 月 29 日
**
** 描        述: ARM64 体系构架启动参数.
*********************************************************************************************************/

#ifndef __ARM64PARAM_H
#define __ARM64PARAM_H

/*********************************************************************************************************
  启动参数
  
  AP_bSLDepCache: ARM64 spin lock 与 Cache 依赖性配置
*********************************************************************************************************/

typedef struct {
    BOOL        AP_bUnalign;                                            /*  是否支持非对齐访问          */
    BOOL        AP_bSLDepCache;                                         /*  sldefcache                  */
} ARM64_PARAM;

/*********************************************************************************************************
  获取启动参数
*********************************************************************************************************/

ARM64_PARAM  *archKernelParamGet(VOID);

#endif                                                                  /*  __ARM64PARAM_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
