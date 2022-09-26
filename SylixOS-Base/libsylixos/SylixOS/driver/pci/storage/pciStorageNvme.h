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
** 文   件   名: pciStorageNvme.h
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2017 年 7 月 17 日
**
** 描        述: NVMe 驱动.
*********************************************************************************************************/

#ifndef __PCISTORAGENVME_H
#define __PCISTORAGENVME_H

/*********************************************************************************************************
  驱动参数 (版本号 0x01000000 为 v1.0.0)
*********************************************************************************************************/
#define NVME_PCI_DRV_NAME                   "nvme_pci"                  /* PCI 类型                     */
#define NVME_PCI_DRV_VER_NUM                0x01000000                  /* 驱动版本数值                 */

/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
INT  pciStorageNvmeInit(VOID);

#endif                                                                  /*  __PCISTORAGENVME_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
