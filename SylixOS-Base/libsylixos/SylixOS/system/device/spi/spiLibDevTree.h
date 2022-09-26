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
** 文   件   名: spiLibDevTree.h
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2020 年 02 月 06 日
**
** 描        述: SPI 总线驱动框架库头文件
*********************************************************************************************************/

#ifndef __SPI_LIB_DEVTREE_H
#define __SPI_LIB_DEVTREE_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  结构体声明
*********************************************************************************************************/

struct lw_dt_spi_xfer;
struct lw_dt_spi_msg;
struct lw_dt_spi_ctrl;
struct lw_dt_spi_dev;
struct lw_dt_spi_drv;

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/

#define LW_SPI_NAME_SIZE        32
#define __spiDevGet(dev)        _LIST_ENTRY(dev, struct lw_dt_spi_dev, DTSPIDEV_devinstance)
#define __spiDrvGet(drv)        _LIST_ENTRY(drv, struct lw_dt_spi_drv, DTSPIDRV_drvinstance)

/*********************************************************************************************************
  mode 模式宏定义
*********************************************************************************************************/

#define LW_SPI_CPHA             0x01                                    /*  clock phase                 */
#define LW_SPI_CPOL             0x02                                    /*  clock polarity              */
#define LW_SPI_MODE_0           (0 | 0)                                 /*  (original MicroWire)        */
#define LW_SPI_MODE_1           (0 | LW_SPI_CPHA)
#define LW_SPI_MODE_2           (LW_SPI_CPOL | 0)
#define LW_SPI_MODE_3           (LW_SPI_CPOL | LW_SPI_CPHA)
#define LW_SPI_CS_HIGH          0x04                                    /*  chipselect active high?     */
#define LW_SPI_LSB_FIRST        0x08                                    /*  per-word bits-on-wire       */
#define LW_SPI_3WIRE            0x10                                    /*  SI/SO signals shared        */
#define LW_SPI_LOOP             0x20                                    /*  loopback mode               */
#define LW_SPI_NO_CS            0x40                                    /*  1 dev/bus, no chipselect    */
#define LW_SPI_READY            0x80                                    /*  slave pulls low to pause    */
#define LW_SPI_TX_DUAL          0x100                                   /*  transmit with 2 wires       */
#define LW_SPI_TX_QUAD          0x200                                   /*  transmit with 4 wires       */
#define LW_SPI_RX_DUAL          0x400                                   /*  receive with 2 wires        */
#define LW_SPI_RX_QUAD          0x800                                   /*  receive with 4 wires        */
#define LW_SPI_CS_WORD          0x1000                                  /*  toggle cs after each word   */
#define LW_SPI_TX_OCTAL         0x2000                                  /*  transmit with 8 wires       */
#define LW_SPI_RX_OCTAL         0x4000                                  /*  receive with 8 wires        */
#define LW_SPI_3WIRE_HIZ        0x8000                                  /*  high impedance turnaround   */

/*********************************************************************************************************
  SPI 总线传输控制消息 xfer
*********************************************************************************************************/

typedef struct lw_dt_spi_xfer {
    PVOID                    DTSPIXFER_pvTxBuf;                         /*  发送缓冲区                  */
    PVOID                    DTSPIXFER_pvRxBuf;                         /*  接收缓冲区                  */

    dma_addr_t               DTSPIXFER_addrTxDma;                       /*  DMA 传输，发送数据地址      */
    dma_addr_t               DTSPIXFER_addrRxDma;                       /*  DMA 传输，接收数据地址      */

    UINT32                   DTSPIXFER_uiLen;                           /*  长度(缓冲区大小)            */

    UINT8                    DTSPIXFER_ucTxNbits;                       /*  发送数据时总线位宽          */
    UINT8                    DTSPIXFER_ucRxNbits;                       /*  接收数据时总线位宽          */

#define LW_SPI_NBITS_SINGLE  0x01                                       /*  1bit transfer               */
#define LW_SPI_NBITS_DUAL    0x02                                       /*  2bits transfer              */
#define LW_SPI_NBITS_QUAD    0x04                                       /*  4bits transfer              */

    UINT8                    DTSPIXFER_ucCsChange;                      /*  传输完成是否需要改变 CS     */
    UINT8                    DTSPIXFER_ucBitsPerWord;                   /*  0 为默认使用 spi 设备参数   */
    UINT32                   DTSPIXFER_uiSpeed;                         /*  0 为默认使用 spi 设备参数   */
    UINT32                   DTSPIXFER_uiDelay;                         /*  延时，单位是 us             */

    LW_LIST_LINE             DTSPIMSG_lineXfer;                         /*  xfer 链表信息               */
} LW_DT_SPI_XFER;
typedef LW_DT_SPI_XFER      *PLW_DT_SPI_XFER;

/*********************************************************************************************************
  SPI 总线消息 msg
*********************************************************************************************************/

typedef struct lw_dt_spi_msg {
    struct lw_dt_spi_dev    *DTSPIMSG_pdtspidev;
    LW_LIST_LINE             DTSPIMSG_lineXfers;                        /*  xfer 链表头，异步模式用     */
    
    UINT32                   DTSPIMSG_uiFrameLen;                       /*  msg 中总的字节数            */
    UINT32                   DTSPIMSG_uiActulLen;                       /*  已成功传输的字节数          */

    VOIDFUNCPTR              DTSPIMSG_pfuncComplete;                    /*  传输结束后的回调函数        */
    PVOID                    DTSPIMSG_pvContext;                        /*  回调函数参数                */

    INT                      DTSPIMSG_iStatus;                          /*  当前 msg 状态               */
    LW_LIST_LINE             DTSPIMSG_lineMsg;                          /*  msg 链表信息，异步模式用    */
} LW_DT_SPI_MSG;
typedef LW_DT_SPI_MSG       *PLW_DT_SPI_MSG;

/*********************************************************************************************************
  SPI 总线控制器
*********************************************************************************************************/

typedef struct lw_dt_spi_ctrl {
    PLW_SPI_ADAPTER          DTSPICTRL_pspiadapter;
    PLW_DEV_INSTANCE         DTSPICTRL_pdevinstance;                    /*  驱动模型中的设备            */
    LW_OBJECT_HANDLE         DTSPICTRL_hBusLock;                        /*  总线操作锁                  */

    UINT16                   DTSPICTRL_usChipSelNums;                   /*  片选信号数量                */
    UINT16                   DTSPICTRL_usDmaAlignment;                  /*  DMA 对齐方式                */
    UINT32                   DTSPICTRL_uiXferSizeMax;                   /*  控制器一次最多传输的字节数  */
    UINT32                   DTSPICTRL_uiSpeedMax;                      /*  SPI 总线最高工作速率        */
    UINT32                   DTSPICTRL_uiSpeedMin;                      /*  SPI 总线最低工作速率        */
    UINT32                   DTSPICTRL_uiMode;                          /*  工作模式                    */
    UINT16                   DTSPICTRL_usFlag;                          /*  其他功能标志                */
    UINT32                  *DTSPICTRL_puiChipSelGpios;                 /*  片选 GPIO                   */
    
#define LW_SPI_HALF_DUPLEX   BIT(0)                                     /*  can't do full duplex        */
#define LW_SPI_NO_RX         BIT(1)                                     /*  can't do buffer read        */
#define LW_SPI_NO_TX         BIT(2)                                     /*  can't do buffer write       */
#define LW_SPI_MUST_RX       BIT(3)                                     /*  requires rx                 */
#define LW_SPI_MUST_TX       BIT(4)                                     /*  requires tx                 */
#define LW_SPI_GPIO_SS       BIT(5)                                     /*  GPIO CS must select slave   */
        
    PVOID                    DTSPICTRL_pvPriv;                          /*  私有数据                    */

    /*
     *  SPI 控制器基本操作函数
     */
    INT                    (*DTSPICTRL_pfuncSetup)(struct lw_dt_spi_dev        *pdtspidev);
    INT                    (*DTSPICTRL_pfuncXferOne)(struct lw_dt_spi_dev      *pdtspidev,
                                                     PLW_DT_SPI_XFER            pdtspixfer);
    VOID                   (*DTSPICTRL_pfuncSetCs)(struct lw_dt_spi_dev        *pdtspidev,
                                                   BOOL                         bEnable);
    INT                    (*DTSPICTRL_pfuncPrepareXfer)(struct lw_dt_spi_dev  *pdtspidev);
    INT                    (*DTSPICTRL_pfuncUnprepareXfer)(struct lw_dt_spi_dev  *pdtspidev);

    
    /*
     *  SPI 控制器其他操作函数
     */
    INT                    (*DTSPICTRL_pfuncPrepareHw)(struct lw_dt_spi_ctrl     *pdtspictrl);
    INT                    (*DTSPICTRL_pfuncUnprepareHw)(struct lw_dt_spi_ctrl   *pdtspictrl);
    INT                    (*DTSPICTRL_pfuncPrepareMsg)(struct lw_dt_spi_ctrl    *pdtspictrl,
                                                        struct lw_dt_spi_msg     *pdtspimsg);
    INT                    (*DTSPICTRL_pfuncUnprepareMsg)(struct lw_dt_spi_ctrl  *pdtspictrl,
                                                          struct lw_dt_spi_msg   *pdtspimsg);
    INT                    (*DTSPICTRL_pfuncXferOneMsg)(struct lw_dt_spi_dev     *pdtspidev,
                                                        struct lw_dt_spi_msg     *pdtspimsg);

    ULONG                    DTSPICTRL_ulPad[16];                       /*  保留未来扩展                */
} LW_DT_SPI_CTRL;
typedef LW_DT_SPI_CTRL     *PLW_DT_SPI_CTRL;

/*********************************************************************************************************
  SPI 设备结构体
*********************************************************************************************************/

typedef struct lw_dt_spi_dev {
    PLW_DT_SPI_CTRL          DTSPIDEV_pdtspictrl;                       /*  挂载的控制器                */
    LW_DEV_INSTANCE          DTSPIDEV_devinstance;                      /*  设备                        */
    atomic_t                 DTSPIDEV_atomicUsageCnt;                   /*  设备使用计数                */
    CHAR                     DTSPIDEV_cName[LW_SPI_NAME_SIZE];          /*  设备的名称                  */

    UINT32                   DTSPIDEV_uiSpeedMax;                       /*  最大工作速率                */
    UINT8                    DTSPIDEV_ucChipSel;                        /*  控制器中片选引脚编号索引    */
    UINT8                    DTSPIDEV_ucBitsPerWord;                    /*  有效数据位数                */
    UINT32                   DTSPIDEV_uiMode;                           /*  SPI 工作模式                */
    UINT                     DTSPIDEV_uiIrq;                            /*  中断号                      */
} LW_DT_SPI_DEVICE;
typedef LW_DT_SPI_DEVICE    *PLW_DT_SPI_DEVICE;

/*********************************************************************************************************
  SPI 设备驱动
*********************************************************************************************************/

typedef struct lw_dt_spi_drv  {
    LW_DRV_INSTANCE          DTSPIDRV_drvinstance;                      /*  驱动模型中的驱动            */
    CPCHAR                   DTSPIDRV_pcName;                           /*  驱动名称                    */
    PVOID                    DTSPIDRV_pvPriv;                           /*  私有数据                    */

    /*
     *  以下为操作函数，具体功能见名称
     */
    INT     (*DTSPIDRV_pfuncProbe)(PLW_DT_SPI_DEVICE     pdtspidev);
    VOID    (*DTSPIDRV_pfuncRemove)(PLW_DT_SPI_DEVICE    pdtspidev);
    INT     (*DTSPIDRV_pfuncShutdown)(PLW_DT_SPI_DEVICE  pdtspidev);
} LW_DT_SPI_DRIVER;
typedef LW_DT_SPI_DRIVER    *PLW_DT_SPI_DRIVER;

/*********************************************************************************************************
  对外接口函数
*********************************************************************************************************/

LW_API INT               API_SpiBusInit(VOID);

LW_API PLW_BUS_TYPE      API_SpiBusGet(VOID);

LW_API INT               API_SpiCtrlRegister(PLW_DT_SPI_CTRL     pdtspictrl,
                                             CPCHAR              pcName);

LW_API VOID              API_SpiCtrlUnregister(PLW_DT_SPI_CTRL   pdtspictrl);

LW_API INT               API_SpiDrvRegister(PLW_DT_SPI_DRIVER    pdtspidriver);

LW_API VOID              API_SpiDrvUnregister(PLW_DT_SPI_DRIVER  pdtspidriver);

LW_API INT               API_SpiDevRegister(PLW_DT_SPI_DEVICE    pdtspidev);

LW_API VOID              API_SpiDevDelete(PLW_DT_SPI_DEVICE      pdtspidev);

LW_API INT               API_SpiDevSetup(PLW_DT_SPI_DEVICE       pdtspidev);

LW_API INT               API_SpiDevTransfer(PLW_DT_SPI_DEVICE    pdtspidev,
                                            PLW_DT_SPI_XFER      pdtspixfers,
                                            INT                  iNum);

LW_API INT               API_SpiWrite(PLW_DT_SPI_DEVICE       pdtspidev,
                                      PVOID                   pvBuf,
                                      INT                     iLen);

LW_API INT               API_SpiRead(PLW_DT_SPI_DEVICE        pdtspidev,
                                     PVOID                    pvBuf,
                                     INT                      iLen);

LW_API INT               API_SpiWriteThenRead(PLW_DT_SPI_DEVICE  pdtspidev,
                                              PVOID              pvTxBuf,
                                              INT                iTxLen,
                                              PVOID              pvRxBuf,
                                              INT                iRxLen);

LW_API ssize_t           API_SpiW8R8(PLW_DT_SPI_DEVICE  pdtspidev, UINT8  ucCmd);

LW_API ssize_t           API_SpiW8R16(PLW_DT_SPI_DEVICE  pdtspidev, UINT8  ucCmd);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __SPI_LIB_DEVTREE_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
