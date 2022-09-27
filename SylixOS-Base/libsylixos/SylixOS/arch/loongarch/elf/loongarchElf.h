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
** 文   件   名: loongarchElf.h
**
** 创   建   人: Wang.Ziyang (王子阳)
**
** 文件创建日期: 2022 年 03 月 10 日
**
** 描        述: 实现 LoongArch 体系结构的 ELF 文件重定位.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCHELF_H
#define __ARCH_LOONGARCHELF_H

#if defined(LW_CFG_CPU_ARCH_LOONGARCH)                                  /*  LoongArch 体系结构          */

#if LW_CFG_CPU_WORD_LENGHT == 32
#define ELF_CLASS              ELFCLASS32
#else
#define ELF_CLASS              ELFCLASS64
#endif
#define ELF_ARCH               EM_LOONGARCH

#define LA_RELA_STACK_DEPTH    16

#endif                                                                  /*  LW_CFG_CPU_ARCH_LOONGARCH   */
#endif                                                                  /*  __ARCH_LOONGARCHELF_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
