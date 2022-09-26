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
** 文   件   名: i2cLibDevTree.h
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2020 年 02 月 6 日
**
** 描        述: I2C 总线驱动框架库头文件
*********************************************************************************************************/

#ifndef __I2C_LIB_DEVTREE_H
#define __I2C_LIB_DEVTREE_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  结构体声明
*********************************************************************************************************/

struct lw_dt_i2c_msg;
struct lw_dt_i2c_adapter;
struct lw_dt_i2c_device;
struct lw_dt_i2c_driver;
struct lw_dt_i2c_funcs;

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

#define LW_I2C_NAME_SIZE            32
#define LW_I2C_TIME_OUT_DEFAULT     100
#define LW_I2C_RETRY_DEFAULT        1

#define __i2cDevGet(dev)            _LIST_ENTRY(dev, struct lw_dt_i2c_device, DTI2CDEV_devinstance)
#define __i2cDrvGet(drv)            _LIST_ENTRY(drv, struct lw_dt_i2c_driver, DTI2CDRV_drvinstance)

#define LW_I2C_TEN_BIT_ADDRESS      (1 << 31)
#define LW_I2C_OWN_SLAVE_ADDRESS    (1 << 30)

/*********************************************************************************************************
  I2C 总线传输控制消息
*********************************************************************************************************/

typedef struct lw_dt_i2c_msg {
    UINT16                      DTI2CMSG_usAddr;                        /*  器件地址                    */
    UINT16                      DTI2CMSG_usFlag;                        /*  传输控制参数                */

#define LW_I2C_M_RD             0x0001                                  /*  为读操作, 否则为写          */
#define LW_I2C_M_TEN            0x0010                                  /*  使用 10bit 设备地址         */
#define LW_I2C_M_RECV_LEN       0x0400                                  /*  !目前不支持!                */
#define LW_I2C_M_NO_RD_ACK      0x0800                                  /*  读操作时不发送 ACK          */
#define LW_I2C_M_IGNORE_NAK     0x1000                                  /*  忽略 ACK NACK               */
#define LW_I2C_M_REV_DIR_ADDR   0x2000                                  /*  读写标志位反转              */
#define LW_I2C_M_NOSTART        0x4000                                  /*  不发送 start 标志           */
    
    UINT16                      DTI2CMSG_usLen;                         /*  长度(缓冲区大小)            */
    UINT8                      *DTI2CMSG_pucBuffer;                     /*  缓冲区                      */
} LW_DT_I2C_MSG;
typedef LW_DT_I2C_MSG          *PLW_DT_I2C_MSG;

/*********************************************************************************************************
  I2C 控制器结构体
*********************************************************************************************************/

typedef struct lw_dt_i2c_adapter {
    PLW_I2C_ADAPTER             DTI2CADPT_pi2cadapter;                  /*  适配器指针                  */
    PLW_DEV_INSTANCE            DTI2CADPT_pdevinstance;                 /*  驱动模型中的设备            */
    LW_OBJECT_HANDLE            DTI2CADPT_hBusLock;                     /*  总线操作锁                  */
    ULONG                       DTI2CADPT_ulTimeout;                    /*  操作超时时间                */
    INT                         DTI2CADPT_iRetry;                       /*  重试次数                    */
    PVOID                       DTI2CADPT_pvPriv;                       /*  私有数据                    */
    struct lw_dt_i2c_funcs     *DTI2CADPT_pi2cfuncs;                    /*  总线适配器操作函数          */

    ULONG                       DTI2CADPT_ulPad[16];                    /*  保留未来扩展                */
} LW_DT_I2C_ADAPTER;
typedef LW_DT_I2C_ADAPTER      *PLW_DT_I2C_ADAPTER;

/*********************************************************************************************************
  I2C 总线传输函数集
*********************************************************************************************************/

typedef struct lw_dt_i2c_funcs {
    INT     (*DTI2CFUNC_pfuncMasterXfer)(PLW_DT_I2C_ADAPTER  pdti2cadapter,
                                         PLW_DT_I2C_MSG      pdti2cmsg,
                                         INT                 iNum);
                                                                        /*  I2C 传输函数                */
    UINT32  (*DTI2CFUNC_pfuncFunction)(PLW_DT_I2C_ADAPTER    pdti2cadapter);
                                                                        /*  I2C 控制器支持的功能        */
} LW_DT_I2C_FUNCS;
typedef LW_DT_I2C_FUNCS       *PLW_DT_I2C_FUNCS;

/*********************************************************************************************************
  I2C 设备结构体
*********************************************************************************************************/

typedef struct lw_dt_i2c_device {
    UINT16                      DTI2CDEV_usAddr;                        /*  设备地址                    */
    UINT16                      DTI2CDEV_usFlag;                        /*  标志, 仅支持 10bit 地址选项 */
    
#define LW_I2C_CLIENT_TEN       0x10                                    /*  与 LW_I2C_M_TEN 相同        */
#define LW_I2C_CLIENT_SLAVE     0x20                                    /*  slave                      */
    
    PLW_DT_I2C_ADAPTER          DTI2CDEV_pdti2cadapter;                 /*  挂载的适配器                */
    LW_DEV_INSTANCE             DTI2CDEV_devinstance;                   /*  设备实例                    */
    ULONG                       DTI2CDEV_ulIrq;                         /*  中断号                      */
    atomic_t                    DTI2CDEV_atomicUsageCnt;                /*  设备使用计数                */
    CHAR                        DTI2CDEV_cName[LW_I2C_NAME_SIZE];       /*  设备的名称                  */
} LW_DT_I2C_DEVICE;
typedef LW_DT_I2C_DEVICE       *PLW_DT_I2C_DEVICE;

/*********************************************************************************************************
  I2C 控制器驱动结构体
*********************************************************************************************************/

typedef struct lw_dt_i2c_driver  {
    LW_DRV_INSTANCE          DTI2CDRV_drvinstance;                      /*  驱动模型中的驱动            */
    CPCHAR                   DTI2CDRV_pcName;                           /*  驱动名称                    */
    PVOID                    DTI2CDRV_pvPriv;                           /*  私有数据                    */

    /* 
     *  以下为操作函数，具体功能见名称
     */    
    INT     (*I2CDRV_pfuncProbe )(PLW_DT_I2C_DEVICE  pdti2cdev);
    VOID    (*I2CDRV_pfuncRemove)(PLW_DT_I2C_DEVICE  pdti2cdev);
    INT     (*I2CDRV_pfuncCmd   )(PLW_DT_I2C_DEVICE  pdti2cdev,
                                  UINT               uiCmd,
                                  PVOID              pArg);
} LW_DT_I2C_DRIVER;
typedef LW_DT_I2C_DRIVER         *PLW_DT_I2C_DRIVER;

/*********************************************************************************************************
  对外接口函数
*********************************************************************************************************/

LW_API INT               API_I2cBusInit(VOID);

LW_API PLW_BUS_TYPE      API_I2cBusGet(VOID);

LW_API INT               API_I2cAdapterRegister(PLW_DT_I2C_ADAPTER  pdti2cadapter, CPCHAR  pcName);

LW_API VOID              API_I2cAdapterUnregister(PLW_DT_I2C_ADAPTER  pdti2cadapter);

LW_API INT               API_I2cDevRegister(PLW_DT_I2C_DEVICE       pdti2cdev);

LW_API VOID              API_I2cDevDelete(PLW_DT_I2C_DEVICE         pdti2cdev);

LW_API INT               API_I2cDrvRegister(PLW_DT_I2C_DRIVER       pdti2cdriver);

LW_API VOID              API_I2cDrvUnregister(PLW_DT_I2C_DRIVER     pdti2cdriver);

LW_API INT               API_I2cBusTransfer(PLW_DT_I2C_ADAPTER      pdti2cadapter,
                                            PLW_DT_I2C_MSG          pdti2cmsg,
                                            INT                     iNum);

LW_API INT               API_I2cDevTransfer(PLW_DT_I2C_DEVICE       pdti2cdev,
                                            PLW_DT_I2C_MSG          pdti2cmsg,
                                            INT                     iNum);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __I2C_LIB_DEVTREE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
