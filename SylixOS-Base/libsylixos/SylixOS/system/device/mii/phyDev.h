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
** 文   件   名: phyDev.h
**
** 创   建   人: Zhang.jian (张健)
**
** 文件创建日期: 2019 年 11 月 22 日
**
** 描        述: phy 设备驱动框架头文件
*********************************************************************************************************/

#ifndef __PHY_DEV_H
#define __PHY_DEV_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

#define PHY_LINK_DOWN           0
#define PHY_LINK_HALF_DUPLIX    1
#define PHY_LINK_FULL_DUPLIX    2

#define PHY_LINK_POS            0
#define PHY_DUPLEX_POS          1
#define PHY_SPEED_POS           16

#define PHY_STATUS_COMBIN(link, duplix, speed)         \
                  (((link)   << PHY_LINK_POS)      |   \
                   ((duplix) << PHY_DUPLEX_POS)    |   \
                   ((speed)  << PHY_SPEED_POS))                         /*  将 phy 的状态组合在一起     */

#define PHY_STATUS_LINK_GET(a)      ((a) >> PHY_LINK_POS   & 0x01)      /*  获取 phy 的连接状态         */
#define PHY_STATUS_DUPLEX_GET(a)    ((a) >> PHY_DUPLEX_POS & 0x01)      /*  获取 phy 的双工状态         */
#define PHY_STATUS_SPEED_GET(a)     ((a) >> PHY_SPEED_POS  & 0xffff)    /*  获取 phy 的速度             */

/*********************************************************************************************************
  phy 设备描述结构体
*********************************************************************************************************/

typedef struct lw_phy_device  {
    PVOID               PHYDEV_pvMacDrv;                                /*  Mother Mac Driver Control   */
    MDIO_DEVICE        *PHYDEV_pmdiodev;

    /*
     *  phy 设备属性，待补充和修改
     */
    UINT32              PHYDEV_uiPhyFlags;                              /*  PHY flag bits               */
    UINT32              PHYDEV_uiPhyAbilityFlags;                       /*  PHY flag bits               */
    UINT32              PHYDEV_uiPhyANFlags;                            /*  Auto Negotiation flags      */
    UINT32              PHYDEV_uiPhyLinkMethod;                         /*  Whether to force link mode  */
    UINT32              PHYDEV_uiLinkDelay;                             /*  Delay time to wait for Link */
    UINT32              PHYDEV_uiSpinDelay;                             /*  Delay time of Spinning Reg  */
    UINT32              PHYDEV_uiTryMax;                                /*  Max Try times               */
    UINT16              PHYDEV_usPhyStatus;                             /*  Record Status of PHY        */
    UINT32              PHYDEV_uiPhyID;                                 /*  Phy ID                      */
    UINT32              PHYDEV_uiPhySpeed;
    CHAR                PHYDEV_pcPhyMode[16];                           /*  Link Mode description       */  
} PHY_DEVICE;
typedef PHY_DEVICE      *PPHY_DEVICE;

/*********************************************************************************************************
  phy 驱动描述结构体
*********************************************************************************************************/

typedef struct lw_phy_driver  {
    CPCHAR              PHYDRV_pcName;                                  /*  phy 驱动名称                */
    UINT32              PHYDRV_uiPhyID;                                 /*  phy ID                      */
    UINT32              PHYDRV_uiPhyIDMask;                             /*  phy mask                    */
    PMDIO_DRIVER        PHYDRV_pmdiodrv;                                /*  phy 驱动所在的 mdio 驱动    */
    
    /*
     *  phy 操作函数，待补充完善
     */
    INT     (*PHYDRV_pfuncSoftReset )(PPHY_DEVICE  pphydev);
    INT     (*PHYDRV_pfuncReadStatus)(PPHY_DEVICE  pphydev);
    INT     (*PHYDRV_pfuncConfigAneg)(PPHY_DEVICE  pphydev);
} PHY_DRIVER;
typedef PHY_DRIVER      *PPHY_DRIVER;

/*********************************************************************************************************
  对外接口函数
*********************************************************************************************************/

LW_API PPHY_DEVICE   API_PhyDevCreate(PMDIO_DEVICE  pmdiodev);

LW_API INT           API_PhyRead(PPHY_DEVICE  pphydev, UINT  uiRegNum);

LW_API INT           API_PhyWrite(PPHY_DEVICE   pphydev,
                                  UINT          uiRegNum,
                                  UINT16        usValue);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /* __PHY_DEV_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
