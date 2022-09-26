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
** 文   件   名: devtreeMdio.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 20 日
**
** 描        述: MDIO 驱动框架中设备树相关接口
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
** 函数名称: __deviceTreeMdioAddrGet
** 功能描述: 通过设备树获取 MDIO 设备地址
** 输　入  : pdtnDev        设备树节点指针
** 输　出  : 非 0 时为有效地址，错误时为 PX_ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __deviceTreeMdioAddrGet (PLW_DEVTREE_NODE  pdtnDev)
{
    UINT32  uiAddr = 0;
    INT     iRet;

    iRet = API_DeviceTreePropertyU32Read(pdtnDev, "reg", &uiAddr);      /*  获取 MDIO 设备地址          */
    if (iRet) {
        DEVTREE_ERR("%s has invalid PHY address\n", pdtnDev->DTN_pcFullName);
        return  (PX_ERROR);
    }

    if (uiAddr >= MDIO_MAX_ADDR) {                                      /*  地址超过正常范围，返回失败  */
        DEVTREE_ERR("%s PHY address %d is too large\n", 
                    pdtnDev->DTN_pcFullName, uiAddr);
        return  (PX_ERROR);
    }
        
    return  (uiAddr);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeMdioDevFind
** 功能描述: 通过设备树节点查找 MDIO 设备
** 输　入  : pdtnDev   设备树节点
** 输　出  : 成功返回查找到的 MDIO 设备指针,
**           失败返回   LW_NULL
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
PMDIO_DEVICE  API_DeviceTreeMdioDevFind (PLW_DEVTREE_NODE  pdtnDev)
{
    PLW_DEV_INSTANCE  pdevinstance;
    PMDIO_DEVICE      pmdiodev;
    
    if (!pdtnDev) {
        return  (LW_NULL);
    }
    
    pdevinstance = API_BusFindDevice(API_MdioBusGet(), pdtnDev);        /*  在 MDIO 总线上查找设备      */
    if (pdevinstance) {
        pmdiodev = __mdioDevGet(pdevinstance);                          /*  获取 MDIO 设备结构体        */
    } else {
        pmdiodev = LW_NULL;
    }

    return  (pmdiodev);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeMdioRegister
** 功能描述: 从设备树中解析 MDIO 控制器下挂载的设备
** 输　入  : pmdioadapter    MDIO 控制器指针
**           pdtnDev         MDIO 控制器的设备树节点
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeMdioRegister (PMDIO_ADAPTER      pmdioadapter,
                                 PLW_DEVTREE_NODE   pdtnDev)
{
    PLW_DEVTREE_NODE    pdtnChild;
    INT                 iDevAddr;                                       /*  MDIO 设备地址               */
    INT                 iRet;

    iRet = API_MdioAdapterRegister(pmdioadapter);                       /*  先注册 MDIO 控制器          */
    if (iRet) {
        return  (iRet);
    }
    
    _LIST_EACH_CHILD_OF_NODE(pdtnDev, pdtnChild) {                      /*  遍历该节点的子节点          */
        iDevAddr = __deviceTreeMdioAddrGet(pdtnChild);                  /*  必须有 reg 地址属性         */
        if (iDevAddr < 0) {
            continue;
        }
            
        API_DeviceTreeMdioDevRegister(pmdioadapter,
                                      pdtnChild, iDevAddr);             /*  注册 MDIO 设备              */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_DeviceTreeMdioDevRegister
** 功能描述: 通过设备树注册 MDIO 设备
** 输　入  : pmdioadapter   MDIO 控制器指针
**           pdtnDev        MDIO 控制器的设备树节点
**           uiAddr         MDIO 设备地址
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块:
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_DeviceTreeMdioDevRegister (PMDIO_ADAPTER      pmdioadapter,
                                    PLW_DEVTREE_NODE   pdtnDev,
                                    UINT               uiAddr)
{
    PMDIO_DEVICE    pmdiodevice;
    ULONG           ulIrqNum = 0;                                       /*  MDIO 设备的中断号           */
    UINT            uiFlag   = 0;
    INT             iRet;

    if (!pmdioadapter || !pdtnDev) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (API_DeviceTreePropertyFind(pdtnDev, "phy", LW_NULL)) {
        uiFlag |= MDIO_DEVICE_IS_PHY;                                   /*  phy 设备设置 phy 标志       */
    }

    if (API_DeviceTreePropertyFind(pdtnDev, "c45", LW_NULL)) {
        uiFlag |= MDIO_DEVICE_IS_C45;                                   /*  MDIO c45 协议设置 c45 标志  */
    }

    API_DeviceTreeIrqGet(pdtnDev, 0, &ulIrqNum);
    if (ulIrqNum > 0) {
        pmdioadapter->MDIOADPT_uiIrqMap[uiAddr] = ulIrqNum;             /*  获取并存储设备的中断号      */
    }

    DEVTREE_MSG("Mdio addr = 0x%x ,flag = 0x%x\r\n", uiAddr, uiFlag);
           
    pmdiodevice = API_MdioDevCreate(pmdioadapter, uiAddr, uiFlag);      /*  创建 MDIO 设备              */
    if (LW_NULL == pmdiodevice) {
        return  (PX_ERROR);
    }
    
    pmdiodevice->MDIODEV_devinstance.DEVHD_pdtnDev = pdtnDev;
                                                                        /*  保存 MDIO 设备的设备树节点  */
    pmdiodevice->MDIODEV_devinstance.DEVHD_pcName  = pdtnDev->DTN_pcFullName;
                                                                        /*  使用节点名作为设备名        */

    iRet = API_MdioDevRegister(pmdiodevice);                            /*  注册 MDIO 设备到系统        */
    if (iRet) {
        DEVTREE_ERR("Mdio device %s register faild.\r\n", pdtnDev->DTN_pcFullName);
        API_MdioDevDelete(pmdiodevice);
    }

    return  (iRet);
}

#endif                                                                  /*  LW_CFG_DEVTREE_EN > 0       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
