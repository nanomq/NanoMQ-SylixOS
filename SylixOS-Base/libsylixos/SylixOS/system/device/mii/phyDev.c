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
** 文   件   名: phyDev.c
**
** 创   建   人: Zhang.Jian (张健)
**
** 文件创建日期: 2019 年 11 月 22 日
**
** 描        述: phy 设备驱动框架头文件
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "mdioLib.h"
#include "phyDev.h"
/*********************************************************************************************************
** 函数名称: __phyIdGet
** 功能描述: 获取 phy ID
** 输　入  : pmdiodev    MDIO 设备指针
** 输　出  : 成功返回 phyid, 失败返回 0xffffffff
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __phyIdGet (PMDIO_DEVICE  pmdiodev)
{
    PMDIO_ADAPTER  pmdioadapter = pmdiodev->MDIODEV_pmdioadapter;
    UINT           uiAddr       = pmdiodev->MDIODEV_uiAddr;
    UINT           uiFlag       = pmdiodev->MDIODEV_uiFlag;
    UINT           uiDevID      = 0;
    INT            iValue;

    if (uiFlag & MDIO_DEVICE_IS_C45) {                                  /*  暂未对 c45 设备进行处理     */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "No mdio c45 handler.\r\n");
        uiDevID = 0xffffffff;
        return  (PX_ERROR);

    } else {
        iValue = API_MdioBusRead(pmdioadapter, uiAddr, 0x02);           /*  读第 1 个 ID 寄存器         */
        if (iValue < 0) {
            uiDevID = 0xffffffff;
            return  (PX_ERROR);
        }

        uiDevID = (iValue & 0xffff) << 16;   

        iValue = API_MdioBusRead(pmdioadapter, uiAddr, 0x03);           /*  读第 2 个 ID 寄存器         */
        if (iValue < 0) {
            uiDevID = 0xffffffff;
            return  (PX_ERROR);
        }

        uiDevID |= (iValue & 0xffff); 
        
        return  (uiDevID);
    }
}
/*********************************************************************************************************
** 函数名称: __phyMatch
** 功能描述: phy 匹配函数
** 输　入  : pdevinstance       设备指针
**           pdrvinstance       驱动指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __phyMatch (PLW_DEV_INSTANCE  pdevinstance, PLW_DRV_INSTANCE  pdrvinstance)
{
    PMDIO_DEVICE  pmdiodev = __mdioDevGet(pdevinstance);
    PMDIO_DRIVER  pmdiodrv = __mdioDrvGet(pdrvinstance);
    PPHY_DRIVER   pphydrv  = pmdiodrv->MDIODRV_pvPriv;
    PPHY_DEVICE   pphydev;

    if (!pphydrv) {
        return  (PX_ERROR);
    }

    pphydev = pmdiodev->MDIODEV_pvPriv;

    /*
     *  如果 phy 设备和驱动的 ID 相同则匹配
     */
    if ((pphydev->PHYDEV_uiPhyID & pphydrv->PHYDRV_uiPhyIDMask) ==
        (pphydrv->PHYDRV_uiPhyID & pphydrv->PHYDRV_uiPhyIDMask)) {
        return  (ERROR_NONE);
    } 

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: API_PhyDevCreate
** 功能描述: phy 设备创建
** 输　入  : pmdiodev    MDIO 设备指针
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
PPHY_DEVICE  API_PhyDevCreate (PMDIO_DEVICE  pmdiodev)
{
    PPHY_DEVICE  pphydev;
    
    if (!pmdiodev) {
        return  (LW_NULL);
    }

    pphydev = __SHEAP_ZALLOC(sizeof(*pphydev));                         /*  为 phy 设备分配内存         */
    if (LW_NULL == pphydev) {
        return  (LW_NULL);
    }

    pphydev->PHYDEV_uiPhyID  = __phyIdGet(pmdiodev);                    /*  获取 phy ID                 */
    pphydev->PHYDEV_pmdiodev = pmdiodev;                                /*  设置父设备 MDIO 设备        */

    _DebugFormat(__LOGMESSAGE_LEVEL, "Phy id = 0x%x \r\n", pphydev->PHYDEV_uiPhyID);
    
    pmdiodev->MDIODEV_pfuncBusMatch = __phyMatch;                       /*  挂接 phy 设备的匹配函数     */

    return  (pphydev);
}
/*********************************************************************************************************
** 函数名称: API_PhyRead
** 功能描述: phy 设备读寄存器函数
** 输　入  : pphydev       phy 设备
**           uiRegNum      寄存器地址
** 输　出  : 读取的寄存器数据
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PhyRead (PPHY_DEVICE  pphydev, UINT  uiRegNum)
{
    PMDIO_ADAPTER  pmdioadapter;
    UINT           uiAddr;

    if (!pphydev ||
        !pphydev->PHYDEV_pmdiodev) {
        return  (PX_ERROR);
    }

    pmdioadapter = pphydev->PHYDEV_pmdiodev->MDIODEV_pmdioadapter;
    uiAddr       = pphydev->PHYDEV_pmdiodev->MDIODEV_uiAddr;

    return  (API_MdioBusRead(pmdioadapter, uiAddr, uiRegNum));
}
/*********************************************************************************************************
** 函数名称: API_PhyWrite
** 功能描述: phy 设备写寄存器函数
** 输　入  : pphydev    phy 设备
**           uiRegNum   寄存器地址
**           usValue    要写入的数据
** 输　出  : ERROR_CODE
** 全局变量: 
** 调用模块: 
**                                            API 函数
*********************************************************************************************************/
LW_API
INT  API_PhyWrite (PPHY_DEVICE  pphydev, UINT  uiRegNum, UINT16  usValue)
{    
    PMDIO_ADAPTER  pmdioadapter;
    UINT           uiAddr;

    if (!pphydev ||
        !pphydev->PHYDEV_pmdiodev) {
        return  (PX_ERROR);
    }

    pmdioadapter = pphydev->PHYDEV_pmdiodev->MDIODEV_pmdioadapter;
    uiAddr       = pphydev->PHYDEV_pmdiodev->MDIODEV_uiAddr;
    
    return  (API_MdioBusWrite(pmdioadapter, uiAddr, uiRegNum, usValue));
}

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
