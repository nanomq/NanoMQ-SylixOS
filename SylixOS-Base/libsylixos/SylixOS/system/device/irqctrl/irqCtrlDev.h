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
** ��   ��   ��: irqCtrlDev.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 06 �� 21 ��
**
** ��        ��: �жϿ�����
*********************************************************************************************************/

#ifndef __IRQCTRL_DEV_H
#define __IRQCTRL_DEV_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  �ṹ������
*********************************************************************************************************/

struct lw_irqctrl_funcs;

/*********************************************************************************************************
  �ṹ�嶨��
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE              IRQCDEV_lineManage;                       /*  �жϿ���������              */
    struct lw_irqctrl_funcs  *IRQCDEV_pirqctrlfuncs;                    /*  �жϿ�������Ҫʵ�ֵĺ���    */
    ULONG                     IRQCDEV_ulDirectMapIrqMax;                /*  ֧��ֱ��ӳ�������жϺ�    */
    ULONG                     IRQCDEV_ulIrqMax;                         /*  ����Ӳ���жϺ�            */
    ULONG                    *IRQCDEV_pulLinearMap;                     /*  �жϵ�ַӳ���              */
    PVOID                     IRQCDEV_pvData;                           /*  �жϿ�����˽������          */
    CHAR                      IRQCDEV_cName[1];                         /*  �жϿ���������              */
} LW_IRQCTRL_DEV;
typedef LW_IRQCTRL_DEV   *PLW_IRQCTRL_DEV;

typedef struct lw_irqctrl_funcs {
    BOOL         (*IRQCF_pfuncIrqCtrlMatch)(PLW_IRQCTRL_DEV   pirqctrldev,
                                            PLW_DEVTREE_NODE  pdtnDev); /*  �жϿ�����ƥ��              */

    INT          (*IRQCF_pfuncIrqCtrlTrans)(PLW_IRQCTRL_DEV           pirqctrldev,
                                            PLW_DEVTREE_PHANDLE_ARGS  pdtpaArgs,
                                            ULONG                    *pulHwIrq,
                                            UINT                     *uiType);
                                                                        /*  �жϲ�������                */

    INT          (*IRQCF_pfuncIrqCtrlMap)(PLW_IRQCTRL_DEV    pirqctrldev,
                                          ULONG              ulHwIrq,
                                          ULONG             *pulVector);/*  �жϺ�ӳ��                  */
} LW_IRQCTRL_FUNCS;
typedef LW_IRQCTRL_FUNCS  *PLW_IRQCTRL_FUNCS;

/*********************************************************************************************************
  ����ӿ�
*********************************************************************************************************/

LW_API PLW_IRQCTRL_DEV  API_IrqCtrlDevtreeNodeMatch(PLW_DEVTREE_NODE  pdtnDev);

LW_API INT              API_IrqCtrlDevtreeTrans(PLW_IRQCTRL_DEV           pirqctrldev,
                                                PLW_DEVTREE_PHANDLE_ARGS  pdtpaArgs,
                                                ULONG                    *pulHwIrq,
                                                UINT                     *uiType);

LW_API INT              API_IrqCtrlFindVectorByHwIrq(PLW_IRQCTRL_DEV      pirqctrldev,
                                                     ULONG                ulHwIrq,
                                                     ULONG               *pulVector);

LW_API INT              API_IrqCtrlHwIrqMapToVector(PLW_IRQCTRL_DEV      pirqctrldev,
                                                    ULONG                ulHwIrq,
                                                    ULONG               *pulVector);

LW_API PLW_IRQCTRL_DEV  API_IrqCtrlDevCreate(CPCHAR             pcName,
                                             PLW_IRQCTRL_FUNCS  pirqctrlfuncs,
                                             ULONG              ulDirectMapMax,
                                             ULONG              ulIrqMax);

LW_API INT              API_IrqCtrlDevDelete(PLW_IRQCTRL_DEV  pirqctrldev);

LW_API PLW_IRQCTRL_DEV  API_IrqCtrlDevFind(CPCHAR  pcName);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __IRQCTRL_DEV_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
