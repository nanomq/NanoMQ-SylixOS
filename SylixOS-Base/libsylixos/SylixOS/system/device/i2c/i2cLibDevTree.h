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
** ��   ��   ��: i2cLibDevTree.h
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2020 �� 02 �� 6 ��
**
** ��        ��: I2C ����������ܿ�ͷ�ļ�
*********************************************************************************************************/

#ifndef __I2C_LIB_DEVTREE_H
#define __I2C_LIB_DEVTREE_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  �ṹ������
*********************************************************************************************************/

struct lw_dt_i2c_msg;
struct lw_dt_i2c_adapter;
struct lw_dt_i2c_device;
struct lw_dt_i2c_driver;
struct lw_dt_i2c_funcs;

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/

#define LW_I2C_NAME_SIZE            32
#define LW_I2C_TIME_OUT_DEFAULT     100
#define LW_I2C_RETRY_DEFAULT        1

#define __i2cDevGet(dev)            _LIST_ENTRY(dev, struct lw_dt_i2c_device, DTI2CDEV_devinstance)
#define __i2cDrvGet(drv)            _LIST_ENTRY(drv, struct lw_dt_i2c_driver, DTI2CDRV_drvinstance)

#define LW_I2C_TEN_BIT_ADDRESS      (1 << 31)
#define LW_I2C_OWN_SLAVE_ADDRESS    (1 << 30)

/*********************************************************************************************************
  I2C ���ߴ��������Ϣ
*********************************************************************************************************/

typedef struct lw_dt_i2c_msg {
    UINT16                      DTI2CMSG_usAddr;                        /*  ������ַ                    */
    UINT16                      DTI2CMSG_usFlag;                        /*  ������Ʋ���                */

#define LW_I2C_M_RD             0x0001                                  /*  Ϊ������, ����Ϊд          */
#define LW_I2C_M_TEN            0x0010                                  /*  ʹ�� 10bit �豸��ַ         */
#define LW_I2C_M_RECV_LEN       0x0400                                  /*  !Ŀǰ��֧��!                */
#define LW_I2C_M_NO_RD_ACK      0x0800                                  /*  ������ʱ������ ACK          */
#define LW_I2C_M_IGNORE_NAK     0x1000                                  /*  ���� ACK NACK               */
#define LW_I2C_M_REV_DIR_ADDR   0x2000                                  /*  ��д��־λ��ת              */
#define LW_I2C_M_NOSTART        0x4000                                  /*  ������ start ��־           */
    
    UINT16                      DTI2CMSG_usLen;                         /*  ����(��������С)            */
    UINT8                      *DTI2CMSG_pucBuffer;                     /*  ������                      */
} LW_DT_I2C_MSG;
typedef LW_DT_I2C_MSG          *PLW_DT_I2C_MSG;

/*********************************************************************************************************
  I2C �������ṹ��
*********************************************************************************************************/

typedef struct lw_dt_i2c_adapter {
    PLW_I2C_ADAPTER             DTI2CADPT_pi2cadapter;                  /*  ������ָ��                  */
    PLW_DEV_INSTANCE            DTI2CADPT_pdevinstance;                 /*  ����ģ���е��豸            */
    LW_OBJECT_HANDLE            DTI2CADPT_hBusLock;                     /*  ���߲�����                  */
    ULONG                       DTI2CADPT_ulTimeout;                    /*  ������ʱʱ��                */
    INT                         DTI2CADPT_iRetry;                       /*  ���Դ���                    */
    PVOID                       DTI2CADPT_pvPriv;                       /*  ˽������                    */
    struct lw_dt_i2c_funcs     *DTI2CADPT_pi2cfuncs;                    /*  ������������������          */

    ULONG                       DTI2CADPT_ulPad[16];                    /*  ����δ����չ                */
} LW_DT_I2C_ADAPTER;
typedef LW_DT_I2C_ADAPTER      *PLW_DT_I2C_ADAPTER;

/*********************************************************************************************************
  I2C ���ߴ��亯����
*********************************************************************************************************/

typedef struct lw_dt_i2c_funcs {
    INT     (*DTI2CFUNC_pfuncMasterXfer)(PLW_DT_I2C_ADAPTER  pdti2cadapter,
                                         PLW_DT_I2C_MSG      pdti2cmsg,
                                         INT                 iNum);
                                                                        /*  I2C ���亯��                */
    UINT32  (*DTI2CFUNC_pfuncFunction)(PLW_DT_I2C_ADAPTER    pdti2cadapter);
                                                                        /*  I2C ������֧�ֵĹ���        */
} LW_DT_I2C_FUNCS;
typedef LW_DT_I2C_FUNCS       *PLW_DT_I2C_FUNCS;

/*********************************************************************************************************
  I2C �豸�ṹ��
*********************************************************************************************************/

typedef struct lw_dt_i2c_device {
    UINT16                      DTI2CDEV_usAddr;                        /*  �豸��ַ                    */
    UINT16                      DTI2CDEV_usFlag;                        /*  ��־, ��֧�� 10bit ��ַѡ�� */
    
#define LW_I2C_CLIENT_TEN       0x10                                    /*  �� LW_I2C_M_TEN ��ͬ        */
#define LW_I2C_CLIENT_SLAVE     0x20                                    /*  slave                      */
    
    PLW_DT_I2C_ADAPTER          DTI2CDEV_pdti2cadapter;                 /*  ���ص�������                */
    LW_DEV_INSTANCE             DTI2CDEV_devinstance;                   /*  �豸ʵ��                    */
    ULONG                       DTI2CDEV_ulIrq;                         /*  �жϺ�                      */
    atomic_t                    DTI2CDEV_atomicUsageCnt;                /*  �豸ʹ�ü���                */
    CHAR                        DTI2CDEV_cName[LW_I2C_NAME_SIZE];       /*  �豸������                  */
} LW_DT_I2C_DEVICE;
typedef LW_DT_I2C_DEVICE       *PLW_DT_I2C_DEVICE;

/*********************************************************************************************************
  I2C �����������ṹ��
*********************************************************************************************************/

typedef struct lw_dt_i2c_driver  {
    LW_DRV_INSTANCE          DTI2CDRV_drvinstance;                      /*  ����ģ���е�����            */
    CPCHAR                   DTI2CDRV_pcName;                           /*  ��������                    */
    PVOID                    DTI2CDRV_pvPriv;                           /*  ˽������                    */

    /* 
     *  ����Ϊ�������������幦�ܼ�����
     */    
    INT     (*I2CDRV_pfuncProbe )(PLW_DT_I2C_DEVICE  pdti2cdev);
    VOID    (*I2CDRV_pfuncRemove)(PLW_DT_I2C_DEVICE  pdti2cdev);
    INT     (*I2CDRV_pfuncCmd   )(PLW_DT_I2C_DEVICE  pdti2cdev,
                                  UINT               uiCmd,
                                  PVOID              pArg);
} LW_DT_I2C_DRIVER;
typedef LW_DT_I2C_DRIVER         *PLW_DT_I2C_DRIVER;

/*********************************************************************************************************
  ����ӿں���
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
