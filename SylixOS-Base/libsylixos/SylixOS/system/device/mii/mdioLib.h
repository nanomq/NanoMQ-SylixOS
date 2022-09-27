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
** 文   件   名: mdioLib.h
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 20 日
**
** 描        述: mdio 总线驱动框架库头文件
*********************************************************************************************************/

#ifndef __MDIO_LIB_H
#define __MDIO_LIB_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  结构体声明
*********************************************************************************************************/

struct lw_mdio_adapter;
struct lw_mdio_device;
struct lw_mdio_driver;

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

#define MDIO_BIT_SET(a)         (1ull << (a))
#define MDIO_DEVICE_IS_PHY      MDIO_BIT_SET(0)
#define MDIO_DEVICE_IS_C45      MDIO_BIT_SET(1)

#define MDIO_MAX_ADDR           32
#define MDIO_NAME_SIZE          32

#define __mdioDevGet(dev)           \
    _LIST_ENTRY(dev, struct lw_mdio_device, MDIODEV_devinstance)        /*  通过 dev 获取 mdiodev       */
    
#define __mdioDrvGet(drv)           \
    _LIST_ENTRY(drv, struct lw_mdio_driver, MDIODRV_drvinstance)        /*  通过 drv 获取 mdiodrv       */

#define MdioDevToMdioDrv(mdiodev)   \
    __mdioDrvGet(mdiodev->MDIODEV_devinstance.DEVHD_pdrvinstance)       /*  通过 mdiodev 获取 mdiodrv   */

/*********************************************************************************************************
  MDIO 控制器结构体
*********************************************************************************************************/

typedef struct lw_mdio_adapter  {
    CPCHAR                  MDIOADPT_pcName;                            /*  MDIO 控制器名称             */
    PVOID                   MDIOADPT_pvPriv;                            /*  私有数据                    */
    LW_OBJECT_HANDLE        MDIOADPT_hBusLock;                          /*  操作锁                      */

    struct lw_mdio_device  *MDIOADPT_DevMap[MDIO_MAX_ADDR];             /*  挂载的设备表                */
    UINT                    MDIOADPT_uiIrqMap[MDIO_MAX_ADDR];           /*  MDIO 设备的中断表           */

    /*
     *  以下为操作函数，具体功能见名称
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
  MDIO 设备结构体
*********************************************************************************************************/

typedef struct lw_mdio_device  {
    CHAR                     MDIODEV_cName[MDIO_NAME_SIZE];             /*  MDIO 设备名称               */
    LW_DEV_INSTANCE          MDIODEV_devinstance;                       /*  驱动模型中的设备            */
    struct lw_mdio_adapter  *MDIODEV_pmdioadapter;                      /*  使用的控制器                */
    PVOID                    MDIODEV_pvPriv;                            /*  私有数据                    */

    UINT                     MDIODEV_uiAddr;                            /*  设备总线地址                */
    UINT                     MDIODEV_uiFlag;                            /*  相关标志                    */

    /*
     *  总线匹配函数
     */
    INT                    (*MDIODEV_pfuncBusMatch)(PLW_DEV_INSTANCE  pDev,
                                                    PLW_DRV_INSTANCE  pDrv);
} MDIO_DEVICE;
typedef MDIO_DEVICE      *PMDIO_DEVICE;

/*********************************************************************************************************
  MDIO 驱动结构体
*********************************************************************************************************/

typedef struct lw_mdio_driver  {  
    LW_DRV_INSTANCE          MDIODRV_drvinstance;                       /*  驱动模型中的驱动            */
    CPCHAR                   MDIODRV_pcName;                            /*  驱动名称                    */
    PVOID                    MDIODRV_pvPriv;                            /*  私有数据                    */
    UINT                     MDIODRV_uiFlag;                            /*  相关标志                    */

    /* 
     *  以下为操作函数，具体功能见名称
     */    
    INT     (*MDIODRV_pfuncProbe    )(PMDIO_DEVICE  pmdiodev);
    VOID    (*MDIODRV_pfuncRemove   )(PMDIO_DEVICE  pmdiodev);
    INT     (*MDIODRV_pfuncInit     )(PMDIO_DEVICE  pmdiodev);
    INT     (*MDIODRV_pfuncStatusGet)(PMDIO_DEVICE  pmdiodev);
} MDIO_DRIVER;
typedef MDIO_DRIVER      *PMDIO_DRIVER;

/*********************************************************************************************************
  对外接口函数
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
