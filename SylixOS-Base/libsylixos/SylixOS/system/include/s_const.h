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
** 文   件   名: s_const.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2007 年 02 月 13 日
**
** 描        述: 这是系统控制常量定义。

** BUG
2007.10.21  STD_MAP() -> ioTaskStdGet(0, fd) 改为 ioTaskStdGet(API_ThreadIdSelf(), fd).
2007.11.07  加入 __GET_TCB_FROM_INDEX() 以获得 ptcb 指针.
2009.04.22  加入文件异常的检查.
2011.11.11  加入 OPEN_MAX 常量.
*********************************************************************************************************/

#ifndef __S_CONST_H
#define __S_CONST_H

/*********************************************************************************************************
  low-level I/O input, output, error fd's
*********************************************************************************************************/

#define STD_IN                      0
#define STD_OUT                     1
#define STD_ERR                     2

#ifndef STDIN_FILENO
#define STDIN_FILENO                STD_IN
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO               STD_OUT
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO               STD_ERR
#endif

/*********************************************************************************************************
  low-level I/O input, output, error fd's
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define __MAX(x, y)                 (((x) < (y)) ? (y) : (x))
#define __MIN(x, y)                 (((x) < (y)) ? (x) : (y))

#ifndef min
#define min                         __MIN
#endif                                                                  /*  min                         */

#ifndef max
#define max                         __MAX
#endif                                                                  /*  max                         */

#endif                                                                  /*  __SYLIXOS_KERNEL            */
/*********************************************************************************************************
  FILE CFG
*********************************************************************************************************/

#define MAX_FILENAME_LENGTH         (PATH_MAX + 1)
#define MAX_DIRNAMES                LW_CFG_DIR_MAX

#define PATH_MAX                    LW_CFG_PATH_MAX
#define NAME_MAX                    PATH_MAX
#define OPEN_MAX                    LW_CFG_MAX_FILES

/*********************************************************************************************************
  MACRO
*********************************************************************************************************/
#ifdef __SYLIXOS_KERNEL

#define __GET_TCB_FROM_INDEX(usIndex)       _K_ptcbTCBIdTable[usIndex]  /*  通过下标查找线程控制块      */

#define __GET_TCB_FROM_HANDLE(ulId)         __GET_TCB_FROM_INDEX(_ObjectGetIndex(ulId))
                                                                        /*  通过 HANDLE ID 获取控制块   */
/*********************************************************************************************************
  驱动程序文件相关接口参数判断
*********************************************************************************************************/

#define __GET_DRV_ARG(pfdentry)             (PVOID)(((pfdentry)->FDENTRY_iType == LW_DRV_TYPE_NEW_1) ? \
                                            (pfdentry) : ((PVOID)(pfdentry->FDENTRY_lValue)))

/*********************************************************************************************************
  进程相关
*********************************************************************************************************/

#define __PROC_GET_PID_CUR()                ((_S_pfuncGetCurPid) ? (_S_pfuncGetCurPid()) : (0))

#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __S_CONST_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
