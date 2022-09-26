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
** ��   ��   ��: phyDev.c
**
** ��   ��   ��: Zhang.Jian (�Ž�)
**
** �ļ���������: 2019 �� 11 �� 22 ��
**
** ��        ��: phy �豸�������ͷ�ļ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_DEVTREE_DRV
#include "SylixOS.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)
#include "mdioLib.h"
#include "phyDev.h"
/*********************************************************************************************************
** ��������: __phyIdGet
** ��������: ��ȡ phy ID
** �䡡��  : pmdiodev    MDIO �豸ָ��
** �䡡��  : �ɹ����� phyid, ʧ�ܷ��� 0xffffffff
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __phyIdGet (PMDIO_DEVICE  pmdiodev)
{
    PMDIO_ADAPTER  pmdioadapter = pmdiodev->MDIODEV_pmdioadapter;
    UINT           uiAddr       = pmdiodev->MDIODEV_uiAddr;
    UINT           uiFlag       = pmdiodev->MDIODEV_uiFlag;
    UINT           uiDevID      = 0;
    INT            iValue;

    if (uiFlag & MDIO_DEVICE_IS_C45) {                                  /*  ��δ�� c45 �豸���д���     */
        _DebugFormat(__ERRORMESSAGE_LEVEL, "No mdio c45 handler.\r\n");
        uiDevID = 0xffffffff;
        return  (PX_ERROR);

    } else {
        iValue = API_MdioBusRead(pmdioadapter, uiAddr, 0x02);           /*  ���� 1 �� ID �Ĵ���         */
        if (iValue < 0) {
            uiDevID = 0xffffffff;
            return  (PX_ERROR);
        }

        uiDevID = (iValue & 0xffff) << 16;   

        iValue = API_MdioBusRead(pmdioadapter, uiAddr, 0x03);           /*  ���� 2 �� ID �Ĵ���         */
        if (iValue < 0) {
            uiDevID = 0xffffffff;
            return  (PX_ERROR);
        }

        uiDevID |= (iValue & 0xffff); 
        
        return  (uiDevID);
    }
}
/*********************************************************************************************************
** ��������: __phyMatch
** ��������: phy ƥ�亯��
** �䡡��  : pdevinstance       �豸ָ��
**           pdrvinstance       ����ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
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
     *  ��� phy �豸�������� ID ��ͬ��ƥ��
     */
    if ((pphydev->PHYDEV_uiPhyID & pphydrv->PHYDRV_uiPhyIDMask) ==
        (pphydrv->PHYDRV_uiPhyID & pphydrv->PHYDRV_uiPhyIDMask)) {
        return  (ERROR_NONE);
    } 

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: API_PhyDevCreate
** ��������: phy �豸����
** �䡡��  : pmdiodev    MDIO �豸ָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
*********************************************************************************************************/
LW_API
PPHY_DEVICE  API_PhyDevCreate (PMDIO_DEVICE  pmdiodev)
{
    PPHY_DEVICE  pphydev;
    
    if (!pmdiodev) {
        return  (LW_NULL);
    }

    pphydev = __SHEAP_ZALLOC(sizeof(*pphydev));                         /*  Ϊ phy �豸�����ڴ�         */
    if (LW_NULL == pphydev) {
        return  (LW_NULL);
    }

    pphydev->PHYDEV_uiPhyID  = __phyIdGet(pmdiodev);                    /*  ��ȡ phy ID                 */
    pphydev->PHYDEV_pmdiodev = pmdiodev;                                /*  ���ø��豸 MDIO �豸        */

    _DebugFormat(__LOGMESSAGE_LEVEL, "Phy id = 0x%x \r\n", pphydev->PHYDEV_uiPhyID);
    
    pmdiodev->MDIODEV_pfuncBusMatch = __phyMatch;                       /*  �ҽ� phy �豸��ƥ�亯��     */

    return  (pphydev);
}
/*********************************************************************************************************
** ��������: API_PhyRead
** ��������: phy �豸���Ĵ�������
** �䡡��  : pphydev       phy �豸
**           uiRegNum      �Ĵ�����ַ
** �䡡��  : ��ȡ�ļĴ�������
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
** ��������: API_PhyWrite
** ��������: phy �豸д�Ĵ�������
** �䡡��  : pphydev    phy �豸
**           uiRegNum   �Ĵ�����ַ
**           usValue    Ҫд�������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���: 
** ����ģ��: 
**                                            API ����
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
