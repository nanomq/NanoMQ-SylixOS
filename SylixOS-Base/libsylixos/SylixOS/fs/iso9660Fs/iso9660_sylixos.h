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
** 文   件   名: iso9660_sylixos.h
**
** 创   建   人: Tiger.Jiang (蒋太金)
**
** 文件创建日期: 2018 年 09 月 15 日
**
** 描        述: ISO9660 文件系统用户与 IO 系统接口部分.
*********************************************************************************************************/

#ifndef __ISO9660_SYLIOS_H
#define __ISO9660_SYLIOS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_MAX_VOLUMES > 0) && (LW_CFG_ISO9660FS_EN > 0)

/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT      API_Iso9660FsDrvInstall(VOID);
LW_API INT      API_Iso9660FsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
LW_API INT      API_Iso9660FsDevDelete(PCHAR   pcName);

#define iso9660FsDrv                API_Iso9660FsDrvInstall
#define iso9660FsDevCreate          API_Iso9660FsDevCreate
#define iso9660FsDevDelete          API_Iso9660FsDevDelete

#endif                                                                  /*  (LW_CFG_MAX_VOLUMES > 0)    */
                                                                        /*  (LW_CFG_FATFS_EN > 0)       */
#endif                                                                  /*  __FATFS_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
