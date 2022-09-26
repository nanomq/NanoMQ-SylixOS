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
** 文   件   名: irqCtrlDev.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 06 月 21 日
**
** 描        述: 中断控制器
*********************************************************************************************************/

#ifndef __IRQCTRL_DEV_H
#define __IRQCTRL_DEV_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  结构体声明
*********************************************************************************************************/

struct lw_irqctrl_funcs;

/*********************************************************************************************************
  结构体定义
*********************************************************************************************************/

typedef struct {
    LW_LIST_LINE              IRQCDEV_lineManage;                       /*  中断控制器链表              */
    struct lw_irqctrl_funcs  *IRQCDEV_pirqctrlfuncs;                    /*  中断控制器需要实现的函数    */
    ULONG                     IRQCDEV_ulDirectMapIrqMax;                /*  支持直接映射的最大中断号    */
    ULONG                     IRQCDEV_ulIrqMax;                         /*  最大的硬件中断号            */
    ULONG                    *IRQCDEV_pulLinearMap;                     /*  中断地址映射表              */
    PVOID                     IRQCDEV_pvData;                           /*  中断控制器私有数据          */
    CHAR                      IRQCDEV_cName[1];                         /*  中断控制器名称              */
} LW_IRQCTRL_DEV;
typedef LW_IRQCTRL_DEV   *PLW_IRQCTRL_DEV;

typedef struct lw_irqctrl_funcs {
    BOOL         (*IRQCF_pfuncIrqCtrlMatch)(PLW_IRQCTRL_DEV   pirqctrldev,
                                            PLW_DEVTREE_NODE  pdtnDev); /*  中断控制器匹配              */

    INT          (*IRQCF_pfuncIrqCtrlTrans)(PLW_IRQCTRL_DEV           pirqctrldev,
                                            PLW_DEVTREE_PHANDLE_ARGS  pdtpaArgs,
                                            ULONG                    *pulHwIrq,
                                            UINT                     *uiType);
                                                                        /*  中断参数解析                */

    INT          (*IRQCF_pfuncIrqCtrlMap)(PLW_IRQCTRL_DEV    pirqctrldev,
                                          ULONG              ulHwIrq,
                                          ULONG             *pulVector);/*  中断号映射                  */
} LW_IRQCTRL_FUNCS;
typedef LW_IRQCTRL_FUNCS  *PLW_IRQCTRL_FUNCS;

/*********************************************************************************************************
  对外接口
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
