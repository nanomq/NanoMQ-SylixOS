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
** ��   ��   ��: spiLibDevTree.h
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2020 �� 02 �� 06 ��
**
** ��        ��: SPI ����������ܿ�ͷ�ļ�
*********************************************************************************************************/

#ifndef __SPI_LIB_DEVTREE_H
#define __SPI_LIB_DEVTREE_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  �ṹ������
*********************************************************************************************************/

struct lw_dt_spi_xfer;
struct lw_dt_spi_msg;
struct lw_dt_spi_ctrl;
struct lw_dt_spi_dev;
struct lw_dt_spi_drv;

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

#define LW_SPI_NAME_SIZE        32
#define __spiDevGet(dev)        _LIST_ENTRY(dev, struct lw_dt_spi_dev, DTSPIDEV_devinstance)
#define __spiDrvGet(drv)        _LIST_ENTRY(drv, struct lw_dt_spi_drv, DTSPIDRV_drvinstance)

/*********************************************************************************************************
  mode ģʽ�궨��
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
  SPI ���ߴ��������Ϣ xfer
*********************************************************************************************************/

typedef struct lw_dt_spi_xfer {
    PVOID                    DTSPIXFER_pvTxBuf;                         /*  ���ͻ�����                  */
    PVOID                    DTSPIXFER_pvRxBuf;                         /*  ���ջ�����                  */

    dma_addr_t               DTSPIXFER_addrTxDma;                       /*  DMA ���䣬�������ݵ�ַ      */
    dma_addr_t               DTSPIXFER_addrRxDma;                       /*  DMA ���䣬�������ݵ�ַ      */

    UINT32                   DTSPIXFER_uiLen;                           /*  ����(��������С)            */

    UINT8                    DTSPIXFER_ucTxNbits;                       /*  ��������ʱ����λ��          */
    UINT8                    DTSPIXFER_ucRxNbits;                       /*  ��������ʱ����λ��          */

#define LW_SPI_NBITS_SINGLE  0x01                                       /*  1bit transfer               */
#define LW_SPI_NBITS_DUAL    0x02                                       /*  2bits transfer              */
#define LW_SPI_NBITS_QUAD    0x04                                       /*  4bits transfer              */

    UINT8                    DTSPIXFER_ucCsChange;                      /*  ��������Ƿ���Ҫ�ı� CS     */
    UINT8                    DTSPIXFER_ucBitsPerWord;                   /*  0 ΪĬ��ʹ�� spi �豸����   */
    UINT32                   DTSPIXFER_uiSpeed;                         /*  0 ΪĬ��ʹ�� spi �豸����   */
    UINT32                   DTSPIXFER_uiDelay;                         /*  ��ʱ����λ�� us             */

    LW_LIST_LINE             DTSPIMSG_lineXfer;                         /*  xfer ������Ϣ               */
} LW_DT_SPI_XFER;
typedef LW_DT_SPI_XFER      *PLW_DT_SPI_XFER;

/*********************************************************************************************************
  SPI ������Ϣ msg
*********************************************************************************************************/

typedef struct lw_dt_spi_msg {
    struct lw_dt_spi_dev    *DTSPIMSG_pdtspidev;
    LW_LIST_LINE             DTSPIMSG_lineXfers;                        /*  xfer ����ͷ���첽ģʽ��     */
    
    UINT32                   DTSPIMSG_uiFrameLen;                       /*  msg ���ܵ��ֽ���            */
    UINT32                   DTSPIMSG_uiActulLen;                       /*  �ѳɹ�������ֽ���          */

    VOIDFUNCPTR              DTSPIMSG_pfuncComplete;                    /*  ���������Ļص�����        */
    PVOID                    DTSPIMSG_pvContext;                        /*  �ص���������                */

    INT                      DTSPIMSG_iStatus;                          /*  ��ǰ msg ״̬               */
    LW_LIST_LINE             DTSPIMSG_lineMsg;                          /*  msg ������Ϣ���첽ģʽ��    */
} LW_DT_SPI_MSG;
typedef LW_DT_SPI_MSG       *PLW_DT_SPI_MSG;

/*********************************************************************************************************
  SPI ���߿�����
*********************************************************************************************************/

typedef struct lw_dt_spi_ctrl {
    PLW_SPI_ADAPTER          DTSPICTRL_pspiadapter;
    PLW_DEV_INSTANCE         DTSPICTRL_pdevinstance;                    /*  ����ģ���е��豸            */
    LW_OBJECT_HANDLE         DTSPICTRL_hBusLock;                        /*  ���߲�����                  */

    UINT16                   DTSPICTRL_usChipSelNums;                   /*  Ƭѡ�ź�����                */
    UINT16                   DTSPICTRL_usDmaAlignment;                  /*  DMA ���뷽ʽ                */
    UINT32                   DTSPICTRL_uiXferSizeMax;                   /*  ������һ����ഫ����ֽ���  */
    UINT32                   DTSPICTRL_uiSpeedMax;                      /*  SPI ������߹�������        */
    UINT32                   DTSPICTRL_uiSpeedMin;                      /*  SPI ������͹�������        */
    UINT32                   DTSPICTRL_uiMode;                          /*  ����ģʽ                    */
    UINT16                   DTSPICTRL_usFlag;                          /*  �������ܱ�־                */
    UINT32                  *DTSPICTRL_puiChipSelGpios;                 /*  Ƭѡ GPIO                   */
    
#define LW_SPI_HALF_DUPLEX   BIT(0)                                     /*  can't do full duplex        */
#define LW_SPI_NO_RX         BIT(1)                                     /*  can't do buffer read        */
#define LW_SPI_NO_TX         BIT(2)                                     /*  can't do buffer write       */
#define LW_SPI_MUST_RX       BIT(3)                                     /*  requires rx                 */
#define LW_SPI_MUST_TX       BIT(4)                                     /*  requires tx                 */
#define LW_SPI_GPIO_SS       BIT(5)                                     /*  GPIO CS must select slave   */
        
    PVOID                    DTSPICTRL_pvPriv;                          /*  ˽������                    */

    /*
     *  SPI ������������������
     */
    INT                    (*DTSPICTRL_pfuncSetup)(struct lw_dt_spi_dev        *pdtspidev);
    INT                    (*DTSPICTRL_pfuncXferOne)(struct lw_dt_spi_dev      *pdtspidev,
                                                     PLW_DT_SPI_XFER            pdtspixfer);
    VOID                   (*DTSPICTRL_pfuncSetCs)(struct lw_dt_spi_dev        *pdtspidev,
                                                   BOOL                         bEnable);
    INT                    (*DTSPICTRL_pfuncPrepareXfer)(struct lw_dt_spi_dev  *pdtspidev);
    INT                    (*DTSPICTRL_pfuncUnprepareXfer)(struct lw_dt_spi_dev  *pdtspidev);

    
    /*
     *  SPI ������������������
     */
    INT                    (*DTSPICTRL_pfuncPrepareHw)(struct lw_dt_spi_ctrl     *pdtspictrl);
    INT                    (*DTSPICTRL_pfuncUnprepareHw)(struct lw_dt_spi_ctrl   *pdtspictrl);
    INT                    (*DTSPICTRL_pfuncPrepareMsg)(struct lw_dt_spi_ctrl    *pdtspictrl,
                                                        struct lw_dt_spi_msg     *pdtspimsg);
    INT                    (*DTSPICTRL_pfuncUnprepareMsg)(struct lw_dt_spi_ctrl  *pdtspictrl,
                                                          struct lw_dt_spi_msg   *pdtspimsg);
    INT                    (*DTSPICTRL_pfuncXferOneMsg)(struct lw_dt_spi_dev     *pdtspidev,
                                                        struct lw_dt_spi_msg     *pdtspimsg);

    ULONG                    DTSPICTRL_ulPad[16];                       /*  ����δ����չ                */
} LW_DT_SPI_CTRL;
typedef LW_DT_SPI_CTRL     *PLW_DT_SPI_CTRL;

/*********************************************************************************************************
  SPI �豸�ṹ��
*********************************************************************************************************/

typedef struct lw_dt_spi_dev {
    PLW_DT_SPI_CTRL          DTSPIDEV_pdtspictrl;                       /*  ���صĿ�����                */
    LW_DEV_INSTANCE          DTSPIDEV_devinstance;                      /*  �豸                        */
    atomic_t                 DTSPIDEV_atomicUsageCnt;                   /*  �豸ʹ�ü���                */
    CHAR                     DTSPIDEV_cName[LW_SPI_NAME_SIZE];          /*  �豸������                  */

    UINT32                   DTSPIDEV_uiSpeedMax;                       /*  ���������                */
    UINT8                    DTSPIDEV_ucChipSel;                        /*  ��������Ƭѡ���ű������    */
    UINT8                    DTSPIDEV_ucBitsPerWord;                    /*  ��Ч����λ��                */
    UINT32                   DTSPIDEV_uiMode;                           /*  SPI ����ģʽ                */
    UINT                     DTSPIDEV_uiIrq;                            /*  �жϺ�                      */
} LW_DT_SPI_DEVICE;
typedef LW_DT_SPI_DEVICE    *PLW_DT_SPI_DEVICE;

/*********************************************************************************************************
  SPI �豸����
*********************************************************************************************************/

typedef struct lw_dt_spi_drv  {
    LW_DRV_INSTANCE          DTSPIDRV_drvinstance;                      /*  ����ģ���е�����            */
    CPCHAR                   DTSPIDRV_pcName;                           /*  ��������                    */
    PVOID                    DTSPIDRV_pvPriv;                           /*  ˽������                    */

    /*
     *  ����Ϊ�������������幦�ܼ�����
     */
    INT     (*DTSPIDRV_pfuncProbe)(PLW_DT_SPI_DEVICE     pdtspidev);
    VOID    (*DTSPIDRV_pfuncRemove)(PLW_DT_SPI_DEVICE    pdtspidev);
    INT     (*DTSPIDRV_pfuncShutdown)(PLW_DT_SPI_DEVICE  pdtspidev);
} LW_DT_SPI_DRIVER;
typedef LW_DT_SPI_DRIVER    *PLW_DT_SPI_DRIVER;

/*********************************************************************************************************
  ����ӿں���
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
