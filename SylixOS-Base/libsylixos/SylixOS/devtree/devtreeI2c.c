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
** 文   件   名: devtreeI2c.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 20 日
**
** 描        述: I2C 驱动框架中设备树相关接口
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
/*********************************************************************************************************
** 函数名称: __deviceTreeI2cDevInfoGet
** 功能描述: 获取 I2C 设备信息
** 输　入  : pdti2cdevice   I2C 设备指针
**           pdtnDev        I2C 设备的设备树节点
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __deviceTreeI2cDevInfoGet (PLW_DT_I2C_DEVICE  pdti2cdevice, PLW_DEVTREE_NODE  pdtnDev)
{
    ULONG   ulIrqNum = 0;  
    UINT32  uiAddr   = 0;
    INT     iRet;

    API_DeviceTreeIrqGet(pdtnDev, 0, &ulIrqNum);                        /*  获取 I2C 设备中断号         */
    if (ulIrqNum > 0) {
        pdti2cdevice->DTI2CDEV_ulIrq = ulIrqNum;
    }

    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "reg", &uiAddr);      /*  获取 I2C 设备地址           */
    if (iRet) {
        DEVTREE_ERR("%s has invalid address\n", pdtnDev->DTN_pcFullName);
        return  (PX_ERROR);
    }

    if (uiAddr & LW_I2C_TEN_BIT_ADDRESS) {
        uiAddr &= ~LW_I2C_TEN_BIT_ADDRESS;
        pdti2cdevice->DTI2CDEV_usFlag |= LW_I2C_CLIENT_TEN;
    }
    
    if (uiAddr & LW_I2C_OWN_SLAVE_ADDRESS) {
        uiAddr &= ~LW_I2C_OWN_SLAVE_ADDRESS;
        pdti2cdevice->DTI2CDEV_usFlag |= LW_I2C_CLIENT_SLAVE;
    }
    
    /*
     *  检查地址合法性
     */
    if (pdti2cdevice->DTI2CDEV_usFlag & LW_I2C_CLIENT_TEN) {
        if (uiAddr > 0x3ff) {
            return  (PX_ERROR);
        }
    } else {
        if ((uiAddr == 0x00) || (uiAddr > 0x7f)) {
            return  (PX_ERROR);
        }
    }
    
    pdti2cdevice->DTI2CDEV_usAddr = uiAddr;
    
    snprintf(pdti2cdevice->DTI2CDEV_cName, LW_I2C_NAME_SIZE, "%s",
             pdtnDev->DTN_pcFullName);                                  /* 设置 I2C 设备名称            */

    pdti2cdevice->DTI2CDEV_atomicUsageCnt.counter    = 0;
    pdti2cdevice->DTI2CDEV_devinstance.DEVHD_pdtnDev = pdtnDev;
    pdti2cdevice->DTI2CDEV_devinstance.DEVHD_pcName  = pdtnDev->DTN_pcFullName;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeI2cAdapterRegister
** 功能描述: 从设备树中解析 I2C 控制器下挂载的设备
** 输　入  : pdti2cadapter  I2C 控制器指针
**           pdtnDev        I2C 控制器的设备树节点
**           pcName         I2C 控制器名称
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeI2cAdapterRegister (PLW_DT_I2C_ADAPTER  pdti2cadapter,
                                       PLW_DEVTREE_NODE    pdtnDev,
                                       CPCHAR              pcName)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iRet;

    iRet = API_I2cAdapterRegister(pdti2cadapter, pcName);               /*  先注册 I2C 控制器           */
    if (iRet) {
        return  (iRet);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  遍历该节点的子节点          */
        API_DeviceTreeI2cDevRegister(pdti2cadapter, pdtnChild);         /*  注册 I2C 设备               */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeI2cDevRegister
** 功能描述: 通过设备树注册 I2C 设备
** 输　入  : pdti2cadapter  I2C 控制器指针
**           pdtnDev        I2C 设备节点
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeI2cDevRegister (PLW_DT_I2C_ADAPTER  pdti2cadapter,
                                   PLW_DEVTREE_NODE    pdtnDev)
{
    PLW_DT_I2C_DEVICE  pdti2cdevice;
    INT                iRet;
    
    pdti2cdevice = __SHEAP_ZALLOC(sizeof(LW_DT_I2C_DEVICE));
    if (!pdti2cdevice) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }

    pdti2cdevice->DTI2CDEV_pdti2cadapter = pdti2cadapter;

    iRet = __deviceTreeI2cDevInfoGet(pdti2cdevice, pdtnDev);            /*  从设备树中获取 I2C 设备信息 */
    if (iRet) {
        __SHEAP_FREE(pdti2cdevice);
        return  (PX_ERROR);
    }
           
    iRet = API_I2cDevRegister(pdti2cdevice);                            /*  注册 I2C 设备到系统         */
    if (iRet) {
        DEVTREE_ERR("I2C device %s register failed.\r\n", 
                    pdtnDev->DTN_pcFullName);
        __SHEAP_FREE(pdti2cdevice);

    } else {
        DEVTREE_MSG("Register I2C device %s at addr 0x%x.\r\n", 
                    pdtnDev->DTN_pcFullName, pdti2cdevice->DTI2CDEV_usAddr);
    }

    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
