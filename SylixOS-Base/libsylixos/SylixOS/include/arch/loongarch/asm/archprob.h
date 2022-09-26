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
** 文   件   名: archprob.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2021 年 10 月 13 日
**
** 描        述: LoongArch 平台编译探测.
*********************************************************************************************************/

#ifndef __ARCHPROB_H
#define __ARCHPROB_H

/*********************************************************************************************************
  LoongArch architecture detect
*********************************************************************************************************/

#define __SYLIXOS_LOONGARCH_ARCH_LOONGARCH32      32
#define __SYLIXOS_LOONGARCH_ARCH_LOONGARCH64      64

#ifdef __GNUC__
#  if defined(_LOONGARCH_ARCH_LOONGARCH32)
#    define __SYLIXOS_LOONGARCH_ARCH__   __SYLIXOS_LOONGARCH_ARCH_LOONGARCH32

#  elif defined(_LOONGARCH_ARCH_LOONGARCH64)
#    define __SYLIXOS_LOONGARCH_ARCH__   __SYLIXOS_LOONGARCH_ARCH_LOONGARCH64

#  endif                                                                /*  user define only            */

#else
#  define __SYLIXOS_LOONGARCH_ARCH__     __SYLIXOS_LOONGARCH_ARCH_LOONGARCH32
                                                                        /*  default LOONGARCH32         */
#endif

#endif                                                                  /*  __ARCHPROB_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
