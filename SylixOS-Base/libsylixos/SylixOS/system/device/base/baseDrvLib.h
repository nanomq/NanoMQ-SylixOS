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
** 文   件   名: baseDrvLib.h
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 10 月 28 日
**
** 描        述: 基础驱动框架库头文件
*********************************************************************************************************/

#ifndef __BASEDRVLIB_H
#define __BASEDRVLIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  类型提前声明
*********************************************************************************************************/

struct lw_bus_type;
struct lw_dev_instance;
struct lw_drv_instance;
struct lw_device_pin_info;

/*********************************************************************************************************
  设备实体结构体
*********************************************************************************************************/

typedef struct lw_dev_instance {
    CPCHAR                        DEVHD_pcName;                         /*  设备名称                    */
    UINT32                        DEVHD_uiId;                           /*  设备 ID                     */
    struct lw_bus_type           *DEVHD_pbustype;                       /*  设备挂载的总线              */
    struct lw_drv_instance       *DEVHD_pdrvinstance;                   /*  设备的驱动                  */
    struct lw_dev_instance       *DEVHD_pdevinstanceParent;             /*  设备的父设备                */
    PLW_DEVTREE_NODE              DEVHD_pdtnDev;                        /*  设备的设备树节点            */
    PVOID                         DEVHD_pvPrivData;                     /*  设备的私有数据              */
    struct lw_device_pin_info    *DEVHD_pdevpininfo;                    /*  设备使用的引脚              */

    LW_OBJECT_HANDLE              DEVHD_hDevLock;                       /*  设备的操作锁                */
    LW_LIST_LINE                  DEVHD_lineDrv;                        /*  设备在驱动链表的节点        */
    LW_LIST_LINE                  DEVHD_lineBus;                        /*  设备在总线链表的节点        */

    /*
     *  以下为设备子项，暂未使用
     */
    LW_LIST_LINE                  DEVHD_lineParent;
    LW_LIST_LINE_HEADER           DEVHD_plineChildrenList;
    LW_OBJECT_HANDLE              DEVHD_hChildrenListLock;
} LW_DEV_INSTANCE;
typedef LW_DEV_INSTANCE          *PLW_DEV_INSTANCE;

/*********************************************************************************************************
   驱动实体结构体
*********************************************************************************************************/

typedef struct lw_drv_instance {
    CPCHAR                        DRVHD_pcName;                        /*  驱动名称                     */
    struct lw_bus_type           *DRVHD_pbustype;                      /*  驱动使用的总线               */
    PLW_DEVTREE_TABLE             DRVHD_pMatchTable;                   /*  驱动匹配表                   */

    LW_LIST_LINE                  DRVHD_lineBus;                       /*  驱动所在总线链表的节点       */
    LW_LIST_LINE_HEADER           DRVHD_plineDevList;                  /*  匹配的设备链表头             */
    LW_OBJECT_HANDLE              DRVHD_hDevListLock;                  /*  匹配的设备链表操作锁         */

    PVOID                         DRVHD_pvPriData;                     /*  驱动的私有数据               */

    /*
     *  以下为驱动操作函数
     */
    INT                         (*DRVHD_pfuncProbe   )(PLW_DEV_INSTANCE  pDev);
    INT                         (*DRVHD_pfuncRemove  )(PLW_DEV_INSTANCE  pDev);
    VOID                        (*DRVHD_pfuncShutdown)(PLW_DEV_INSTANCE  pDev);
    INT                         (*DRVHD_pfuncSuspend )(PLW_DEV_INSTANCE  pDev);
    INT                         (*DRVHD_pfuncResume  )(PLW_DEV_INSTANCE  pDev);
} LW_DRV_INSTANCE;
typedef LW_DRV_INSTANCE          *PLW_DRV_INSTANCE;

/*********************************************************************************************************
  总线实体结构体
*********************************************************************************************************/

typedef struct lw_bus_type {
    CPCHAR                        BUS_pcName;                           /*  总线名称                    */
    CPCHAR                        BUS_pcDevName;                        /*  总线上设备名模板            */

    UINT                          BUS_uiStatus;                         /*  总线状态                    */
    UINT                          BUS_uiFlag;                           /*  总线标志                    */
#define BUS_INITIALIZED           0x01
#define BUS_AUTO_PROBE            0x02
#define BUS_FORCE_DRV_PROBE       0x03

    LW_OBJECT_HANDLE              BUS_hBusLock;                         /*  总线操作锁                  */
    LW_LIST_LINE_HEADER           BUS_plineDevList;                     /*  总线上设备链表头            */
    LW_LIST_LINE_HEADER           BUS_plineDrvList;                     /*  总线上驱动链表头            */
    LW_OBJECT_HANDLE              BUS_hDevListLock;                     /*  设备链表操作锁              */
    LW_OBJECT_HANDLE              BUS_hDrvListLock;                     /*  驱动链表操作锁              */

    /*
     *  以下为总线操作函数
     */
    INT                         (*BUS_pfuncMatch   )(PLW_DEV_INSTANCE  pDev, PLW_DRV_INSTANCE  pDrv);
    INT                         (*BUS_pfuncProbe   )(PLW_DEV_INSTANCE  pDev);
    INT                         (*BUS_pfuncRemove  )(PLW_DEV_INSTANCE  pDev);
    VOID                        (*BUS_pfuncShutdown)(PLW_DEV_INSTANCE  pDev);
    INT                         (*BUS_pfuncSuspend )(PLW_DEV_INSTANCE  pDev);
    INT                         (*BUS_pfuncResume  )(PLW_DEV_INSTANCE  pDev);
} LW_BUS_TYPE;
typedef LW_BUS_TYPE            *PLW_BUS_TYPE;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT               API_BusInit(PLW_BUS_TYPE               pBus);

LW_API PLW_DEV_INSTANCE  API_BusFindDevice(PLW_BUS_TYPE         pvBus,
                                           PLW_DEVTREE_NODE     pdtNode);

LW_API INT               API_DeviceRegister(PLW_DEV_INSTANCE    pDev);

LW_API VOID              API_DeviceUnregister(PLW_DEV_INSTANCE  pDev);

LW_API INT               API_DriverRegister(PLW_DRV_INSTANCE    pDrv);

LW_API VOID              API_DriverUnregister(PLW_DRV_INSTANCE  pDrv);

LW_API VOID              API_DevSetDrvdata(PLW_DEV_INSTANCE     pDev, PVOID  pvData);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /* __BASEDRVLIB_H               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
