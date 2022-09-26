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
** ��   ��   ��: mdioLib.h
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 20 ��
**
** ��        ��: mdio ����������ܿ�ͷ�ļ�
*********************************************************************************************************/

#ifndef __MDIO_LIB_H
#define __MDIO_LIB_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  �ṹ������
*********************************************************************************************************/

struct lw_mdio_adapter;
struct lw_mdio_device;
struct lw_mdio_driver;

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

#define MDIO_BIT_SET(a)         (1ull << (a))
#define MDIO_DEVICE_IS_PHY      MDIO_BIT_SET(0)
#define MDIO_DEVICE_IS_C45      MDIO_BIT_SET(1)

#define MDIO_MAX_ADDR           32
#define MDIO_NAME_SIZE          32

#define __mdioDevGet(dev)           \
    _LIST_ENTRY(dev, struct lw_mdio_device, MDIODEV_devinstance)        /*  ͨ�� dev ��ȡ mdiodev       */
    
#define __mdioDrvGet(drv)           \
    _LIST_ENTRY(drv, struct lw_mdio_driver, MDIODRV_drvinstance)        /*  ͨ�� drv ��ȡ mdiodrv       */

#define MdioDevToMdioDrv(mdiodev)   \
    __mdioDrvGet(mdiodev->MDIODEV_devinstance.DEVHD_pdrvinstance)       /*  ͨ�� mdiodev ��ȡ mdiodrv   */

/*********************************************************************************************************
  MDIO �������ṹ��
*********************************************************************************************************/

typedef struct lw_mdio_adapter  {
    CPCHAR                  MDIOADPT_pcName;                            /*  MDIO ����������             */
    PVOID                   MDIOADPT_pvPriv;                            /*  ˽������                    */
    LW_OBJECT_HANDLE        MDIOADPT_hBusLock;                          /*  ������                      */

    struct lw_mdio_device  *MDIOADPT_DevMap[MDIO_MAX_ADDR];             /*  ���ص��豸��                */
    UINT                    MDIOADPT_uiIrqMap[MDIO_MAX_ADDR];           /*  MDIO �豸���жϱ�           */

    /*
     *  ����Ϊ�������������幦�ܼ�����
     */
    INT                    (*MDIOADPT_pfuncRead)(struct lw_mdio_adapter   *pmdioadapter,
                                                 UINT                      uiAddr,
                                                 UINT                      uiRegNum);
    INT                    (*MDIOADPT_pfuncWrite)(struct lw_mdio_adapter  *pmdioadapter,
                                                  UINT                     uiAddr,
                                                  UINT                     uiRegNum,
                                                  UINT16                   usValue);
    INT                    (*MDIOADPT_pfuncReset)(struct lw_mdio_adapter  *pmdioadapter);
} MDIO_ADAPTER;
typedef MDIO_ADAPTER      *PMDIO_ADAPTER;

/*********************************************************************************************************
  MDIO �豸�ṹ��
*********************************************************************************************************/

typedef struct lw_mdio_device  {
    CHAR                     MDIODEV_cName[MDIO_NAME_SIZE];             /*  MDIO �豸����               */
    LW_DEV_INSTANCE          MDIODEV_devinstance;                       /*  ����ģ���е��豸            */
    struct lw_mdio_adapter  *MDIODEV_pmdioadapter;                      /*  ʹ�õĿ�����                */
    PVOID                    MDIODEV_pvPriv;                            /*  ˽������                    */

    UINT                     MDIODEV_uiAddr;                            /*  �豸���ߵ�ַ                */
    UINT                     MDIODEV_uiFlag;                            /*  ��ر�־                    */

    /*
     *  ����ƥ�亯��
     */
    INT                    (*MDIODEV_pfuncBusMatch)(PLW_DEV_INSTANCE  pDev,
                                                    PLW_DRV_INSTANCE  pDrv);
} MDIO_DEVICE;
typedef MDIO_DEVICE      *PMDIO_DEVICE;

/*********************************************************************************************************
  MDIO �����ṹ��
*********************************************************************************************************/

typedef struct lw_mdio_driver  {  
    LW_DRV_INSTANCE          MDIODRV_drvinstance;                       /*  ����ģ���е�����            */
    CPCHAR                   MDIODRV_pcName;                            /*  ��������                    */
    PVOID                    MDIODRV_pvPriv;                            /*  ˽������                    */
    UINT                     MDIODRV_uiFlag;                            /*  ��ر�־                    */

    /* 
     *  ����Ϊ�������������幦�ܼ�����
     */    
    INT     (*MDIODRV_pfuncProbe    )(PMDIO_DEVICE  pmdiodev);
    VOID    (*MDIODRV_pfuncRemove   )(PMDIO_DEVICE  pmdiodev);
    INT     (*MDIODRV_pfuncInit     )(PMDIO_DEVICE  pmdiodev);
    INT     (*MDIODRV_pfuncStatusGet)(PMDIO_DEVICE  pmdiodev);
} MDIO_DRIVER;
typedef MDIO_DRIVER      *PMDIO_DRIVER;

/*********************************************************************************************************
  ����ӿں���
*********************************************************************************************************/

LW_API INT             API_MdioBusInit(VOID);

LW_API INT             API_MdioAdapterRegister(PMDIO_ADAPTER  pmdioadapter);

LW_API INT             API_MdioDrvRegister(PMDIO_DRIVER  pmdiodrv);

LW_API PMDIO_DEVICE    API_MdioDevCreate(PMDIO_ADAPTER   pmdioadapter,
                                         UINT            uiAddr,
                                         UINT            uiFlag);

LW_API VOID            API_MdioDevDelete(PMDIO_DEVICE    pmdiodev);

LW_API INT             API_MdioDevRegister(PMDIO_DEVICE  pmdiodev);

LW_API INT             API_MdioBusRead(PMDIO_ADAPTER     pmdioadapter,
                                       UINT              uiAddr,
                                       UINT              uiRegNum);

LW_API INT             API_MdioBusWrite(PMDIO_ADAPTER    pmdioadapter,
                                        UINT             uiAddr,
                                        UINT             uiRegNum,
                                        UINT16           usValue);

LW_API INT             API_MdioBusReset(PMDIO_ADAPTER    pmdioadapter);

LW_API PLW_BUS_TYPE    API_MdioBusGet(VOID);

LW_API INT             API_MdioDevRead(PMDIO_DEVICE      pmdiodev, UINT  uiRegNum);

LW_API INT             API_MdioDevWrite(PMDIO_DEVICE     pmdiodev,
                                        UINT             uiRegNum,
                                        UINT16           usValue);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /* __MDIO_LIB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
