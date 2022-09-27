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
** 文   件   名: devtreeSpi.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 20 日
**
** 描        述: SPI 驱动框架中设备树相关接口
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_DEVTREE_EN > 0
#include "devtree.h"
/*********************************************************************************************************
** 函数名称: __deviceTreeSpiDevInfoGet
** 功能描述: 获取 SPI 设备信息
** 输　入  : pdtspidev     SPI 设备指针
**           pdtnDev       SPI 设备的设备树节点
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __deviceTreeSpiDevInfoGet (PLW_DT_SPI_DEVICE  pdtspidev, PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiValue = 0;
    INT     iRet;

    iRet = API_DeviceTreeModaliasGet(pdtnDev,
                                     pdtspidev->DTSPIDEV_cName,
                                     sizeof(pdtspidev->DTSPIDEV_cName));/*  获取 SPI 设备名称           */
    if (iRet) {
        DEVTREE_ERR("%s Get name failed supported\r\n", 
                    pdtnDev->DTN_pcFullName);
        return  (PX_ERROR);
    }

    /*
     *  获取基本的模式信息（相位、极性等）
     */
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-cpha")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_CPHA;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-cpol")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_CPOL;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-3wire")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_3WIRE;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-lsb-first")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_LSB_FIRST;
    }
    if (API_DeviceTreePropertyBoolRead(pdtnDev, "spi-cs-high")) {
        pdtspidev->DTSPIDEV_uiMode |= LW_SPI_CS_HIGH;
    }

    /*
     *  获取 SPI 发送总线的数据宽度，设置到模式中
     */
    if (!API_DeviceTreePropertyU32Read(pdtnDev, "spi-tx-bus-width", &uiValue)) {

        switch (uiValue) {

        case 1:
            break;

        case 2:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_TX_DUAL;
            break;

        case 4:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_TX_QUAD;
            break;

        case 8:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_TX_OCTAL;
            break;

        default:
            DEVTREE_ERR("%s spi-tx-bus-width %d not supported\r\n", 
                        pdtnDev->DTN_pcFullName, uiValue);
            break;
        }
    }

    /*
     *  获取 SPI 接受总线的数据宽度，设置到模式中
     */
    if (!API_DeviceTreePropertyU32Read(pdtnDev, "spi-rx-bus-width", &uiValue)) {

        switch (uiValue) {

        case 1:
            break;

        case 2:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_RX_DUAL;
            break;

        case 4:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_RX_QUAD;
            break;

        case 8:
            pdtspidev->DTSPIDEV_uiMode |= LW_SPI_RX_OCTAL;
            break;

        default:
            DEVTREE_ERR("%s spi-rx-bus-width %d not supported\r\n", 
                        pdtnDev->DTN_pcFullName, uiValue);
            break;
        }
    }
    
    /*
     *  获取设备地址
     */
    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "reg", &uiValue);
    if (iRet) {
        DEVTREE_ERR("%s has no valid 'reg' property.\r\n", 
                    pdtnDev->DTN_pcFullName);
        
        return  (PX_ERROR);
    }
    pdtspidev->DTSPIDEV_ucChipSel = uiValue;

    /*
     *  获取 SPI 设备最大传输速率
     */
    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "spi-max-frequency", &uiValue);
    if (iRet) {
        DEVTREE_ERR("%s has no valid 'spi-max-frequency' property.\r\n", 
                    pdtnDev->DTN_pcFullName);
        
        return  (PX_ERROR);
    }

    pdtspidev->DTSPIDEV_uiSpeedMax                = uiValue;
    pdtspidev->DTSPIDEV_atomicUsageCnt.counter    = 0;
    pdtspidev->DTSPIDEV_devinstance.DEVHD_pdtnDev = pdtnDev;
    pdtspidev->DTSPIDEV_devinstance.DEVHD_pcName  = pdtnDev->DTN_pcFullName;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeSpiCtrlRegister
** 功能描述: 从设备树中解析并注册 SPI 控制器下挂载的设备
** 输　入  : pdtspictrl    SPI 控制器指针
**           pdtnDev     SPI 设备节点
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeSpiCtrlRegister (PLW_DT_SPI_CTRL      pdtspictrl,
                                    PLW_DEVTREE_NODE     pdtnDev,
                                    CPCHAR               pcName)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iRet;

    iRet = API_SpiCtrlRegister(pdtspictrl, pcName);                     /*  注册 SPI 控制器             */
    if (iRet) {
        return  (iRet);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  遍历该节点的子节点          */
        API_DeviceTreeSpiDevRegister(pdtspictrl, pdtnChild);            /*  注册 SPI 设备               */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeSpiDevRegister
** 功能描述: 通过设备树注册 SPI 设备
** 输　入  : pdtspictrl    SPI 控制器指针
**           pdtnDev       SPI 设备节点
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeSpiDevRegister (PLW_DT_SPI_CTRL    pdtspictrl,
                                   PLW_DEVTREE_NODE   pdtnDev)
{
    PLW_DT_SPI_DEVICE  pdtspidev;
    INT                iRet;

    if (!pdtspictrl || !pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    pdtspidev = __SHEAP_ZALLOC(sizeof(LW_DT_SPI_DEVICE));
    if (!pdtspidev) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    pdtspidev->DTSPIDEV_pdtspictrl = pdtspictrl;                        /*  设置 SPI 设备连接的控制器   */
    
    iRet = __deviceTreeSpiDevInfoGet(pdtspidev, pdtnDev);               /*  从设备树中获取 SPI 设备信息 */
    if (iRet) {
        goto  __error_handle;
    }
           
    iRet = API_SpiDevRegister(pdtspidev);                               /*  注册 SPI 设备到系统         */
    if (iRet) {
        DEVTREE_ERR("SPI device %s register failed.\r\n",
                    pdtspidev->DTSPIDEV_cName);
        goto  __error_handle;
    } else {
        DEVTREE_MSG("SPI device %s register successfully.\r\n",
                    pdtspidev->DTSPIDEV_cName);
    }

    return  (ERROR_NONE);

__error_handle:
    __SHEAP_FREE(pdtspidev);
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
