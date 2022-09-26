/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: baseDrvLib.h
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 10 �� 28 ��
**
** ��        ��: ����������ܿ�ͷ�ļ�
*********************************************************************************************************/

#ifndef __BASEDRVLIB_H
#define __BASEDRVLIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  ������ǰ����
*********************************************************************************************************/

struct lw_bus_type;
struct lw_dev_instance;
struct lw_drv_instance;
struct lw_device_pin_info;

/*********************************************************************************************************
  �豸ʵ��ṹ��
*********************************************************************************************************/

typedef struct lw_dev_instance {
    CPCHAR                        DEVHD_pcName;                         /*  �豸����                    */
    UINT32                        DEVHD_uiId;                           /*  �豸 ID                     */
    struct lw_bus_type           *DEVHD_pbustype;                       /*  �豸���ص�����              */
    struct lw_drv_instance       *DEVHD_pdrvinstance;                   /*  �豸������                  */
    struct lw_dev_instance       *DEVHD_pdevinstanceParent;             /*  �豸�ĸ��豸                */
    PLW_DEVTREE_NODE              DEVHD_pdtnDev;                        /*  �豸���豸���ڵ�            */
    PVOID                         DEVHD_pvPrivData;                     /*  �豸��˽������              */
    struct lw_device_pin_info    *DEVHD_pdevpininfo;                    /*  �豸ʹ�õ�����              */

    LW_OBJECT_HANDLE              DEVHD_hDevLock;                       /*  �豸�Ĳ�����                */
    LW_LIST_LINE                  DEVHD_lineDrv;                        /*  �豸����������Ľڵ�        */
    LW_LIST_LINE                  DEVHD_lineBus;                        /*  �豸����������Ľڵ�        */

    /*
     *  ����Ϊ�豸�����δʹ��
     */
    LW_LIST_LINE                  DEVHD_lineParent;
    LW_LIST_LINE_HEADER           DEVHD_plineChildrenList;
    LW_OBJECT_HANDLE              DEVHD_hChildrenListLock;
} LW_DEV_INSTANCE;
typedef LW_DEV_INSTANCE          *PLW_DEV_INSTANCE;

/*********************************************************************************************************
   ����ʵ��ṹ��
*********************************************************************************************************/

typedef struct lw_drv_instance {
    CPCHAR                        DRVHD_pcName;                        /*  ��������                     */
    struct lw_bus_type           *DRVHD_pbustype;                      /*  ����ʹ�õ�����               */
    PLW_DEVTREE_TABLE             DRVHD_pMatchTable;                   /*  ����ƥ���                   */

    LW_LIST_LINE                  DRVHD_lineBus;                       /*  ����������������Ľڵ�       */
    LW_LIST_LINE_HEADER           DRVHD_plineDevList;                  /*  ƥ����豸����ͷ             */
    LW_OBJECT_HANDLE              DRVHD_hDevListLock;                  /*  ƥ����豸���������         */

    PVOID                         DRVHD_pvPriData;                     /*  ������˽������               */

    /*
     *  ����Ϊ������������
     */
    INT                         (*DRVHD_pfuncProbe   )(PLW_DEV_INSTANCE  pDev);
    INT                         (*DRVHD_pfuncRemove  )(PLW_DEV_INSTANCE  pDev);
    VOID                        (*DRVHD_pfuncShutdown)(PLW_DEV_INSTANCE  pDev);
    INT                         (*DRVHD_pfuncSuspend )(PLW_DEV_INSTANCE  pDev);
    INT                         (*DRVHD_pfuncResume  )(PLW_DEV_INSTANCE  pDev);
} LW_DRV_INSTANCE;
typedef LW_DRV_INSTANCE          *PLW_DRV_INSTANCE;

/*********************************************************************************************************
  ����ʵ��ṹ��
*********************************************************************************************************/

typedef struct lw_bus_type {
    CPCHAR                        BUS_pcName;                           /*  ��������                    */
    CPCHAR                        BUS_pcDevName;                        /*  �������豸��ģ��            */

    UINT                          BUS_uiStatus;                         /*  ����״̬                    */
    UINT                          BUS_uiFlag;                           /*  ���߱�־                    */
#define BUS_INITIALIZED           0x01
#define BUS_AUTO_PROBE            0x02
#define BUS_FORCE_DRV_PROBE       0x03

    LW_OBJECT_HANDLE              BUS_hBusLock;                         /*  ���߲�����                  */
    LW_LIST_LINE_HEADER           BUS_plineDevList;                     /*  �������豸����ͷ            */
    LW_LIST_LINE_HEADER           BUS_plineDrvList;                     /*  ��������������ͷ            */
    LW_OBJECT_HANDLE              BUS_hDevListLock;                     /*  �豸���������              */
    LW_OBJECT_HANDLE              BUS_hDrvListLock;                     /*  �������������              */

    /*
     *  ����Ϊ���߲�������
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
