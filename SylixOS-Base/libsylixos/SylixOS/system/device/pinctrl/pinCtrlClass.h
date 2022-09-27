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
** 文   件   名: pinCtrlClass.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 12 日
**
** 描        述: pinctrl 子系统变量类型定义
*********************************************************************************************************/

#ifndef __PINCTRLCLASS_H
#define __PINCTRLCLASS_H

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  引脚映射结构类型
*********************************************************************************************************/

typedef enum {
    PIN_MAP_TYPE_INVALID,
    PIN_MAP_TYPE_DUMMY_STATE,                                           /*  虚拟的引脚配置              */
    PIN_MAP_TYPE_MUX_GROUP,                                             /*  按组进行引脚复用配置        */
    PIN_MAP_TYPE_CONFIGS_PIN,                                           /*  按照单个引脚功能配置        */
    PIN_MAP_TYPE_CONFIGS_GROUP,                                         /*  按组进行引脚功能配置        */
} LW_PINCTRL_MAP_TYPE;

struct lw_pinctrl_desc;
struct lw_pinctrl;
struct lw_pinctrl_state;
struct lw_pinctrl_ops;
struct lw_pinmux_ops;
struct lw_pinconf_ops;
struct lw_pinctrl_gpio_range;

/*********************************************************************************************************
  引脚控制相关的引脚描述 (驱动中使用)
*********************************************************************************************************/

typedef struct lw_pinctrl_pin_desc {
    UINT32                       PCTLPD_uiIndex;                        /*  引脚序号                    */
    PCHAR                        PCTLPD_pcName;                         /*  引脚名称                    */
    PVOID                        PCTLPD_pvData;                         /*  私有数据                    */
} LW_PINCTRL_PIN_DESC;
typedef LW_PINCTRL_PIN_DESC     *PLW_PINCTRL_PIN_DESC;

#define PINCTRL_PIN(a, b)   { .PCTLPD_uiIndex = a, .PCTLPD_pcName = b }

/*********************************************************************************************************
  设备相关的引脚描述 (内核中使用)
*********************************************************************************************************/

typedef struct lw_pin_desc {
    LW_LIST_LINE                PIN_lineManage;                         /*  管理链表                    */
    struct lw_pinctrl_dev      *PIN_ppinctldev;                         /*  引脚控制器指针              */
    UINT                        PIN_uiPin;                              /*  引脚序号                    */
    CPCHAR                      PIN_pcName;                             /*  引脚名称                    */
    PVOID                       PIN_pvData;                             /*  私有数据                    */
} LW_PIN_DESC;
typedef LW_PIN_DESC            *PLW_PIN_DESC;

/*********************************************************************************************************
  引脚控制器设备
*********************************************************************************************************/

typedef struct lw_pinctrl_dev {
    LW_LIST_LINE                 PCTLD_lineManage;                      /*  引脚控制器链表              */
    LW_LIST_LINE_HEADER          PCTLD_plineDescs;                      /*  引脚描述的集合              */
    struct lw_pinctrl           *PCTLD_ppinctrl;                        /*  引脚节点链表                */
    struct lw_pinctrl_desc      *PCTLD_ppinctldesc;                     /*  引脚控制描述                */
    PLW_DEVTREE_NODE             PCTLD_pdtnDev;                         /*  引脚控制器的设备树节点      */
    PVOID                        PCTLD_pvData;                          /*  私有数据                    */
    LW_LIST_LINE_HEADER          PCTLD_plineGpioRange;                  /*  GPIO 范围                   */
} LW_PINCTRL_DEV;
typedef LW_PINCTRL_DEV          *PLW_PINCTRL_DEV;

/*********************************************************************************************************
  引脚控制描述
*********************************************************************************************************/

typedef struct lw_pinctrl_desc {
    PCHAR                        PCTLD_pcName;                          /*  引脚控制名称                */
    UINT                         PCTLD_uiPinsNum;                       /*  引脚个数                    */
    struct lw_pinctrl_pin_desc  *PCTLD_ppinctrlpindesc;                 /*  引脚描述集合                */
    struct lw_pinctrl_ops       *PCTLD_ppinctlops;                      /*  引脚控制操作集合            */
    struct lw_pinmux_ops        *PCTLD_ppinmuxops;                      /*  引脚复用操作集合            */
    struct lw_pinconf_ops       *PCTLD_ppinconfops;                     /*  引脚配置操作集合            */
} LW_PINCTRL_DESC;
typedef LW_PINCTRL_DESC         *PLW_PINCTRL_DESC;

/*********************************************************************************************************
  引脚控制映射复用相关
*********************************************************************************************************/

typedef struct lw_pinctrl_map_mux {
#define map_pcGroup         data.PCTLM_pctrlmapmux.PCTLMM_pcGroup
    CPCHAR                       PCTLMM_pcGroup;
#define map_pcFunction      data.PCTLM_pctrlmapmux.PCTLMM_pcFunction
    CPCHAR                       PCTLMM_pcFunction;
} LW_PINCTRL_MAP_MUX;
typedef LW_PINCTRL_MAP_MUX      *PLW_PINCTRL_MAP_MUX;

/*********************************************************************************************************
  引脚控制映射配置相关
*********************************************************************************************************/

typedef struct lw_pinctrl_map_configs {
#define map_pcGroupOrPin    data.PCTLM_pctrlmapconf.PCTLMC_pcGroupOrPin
    CPCHAR                       PCTLMC_pcGroupOrPin;
#define map_pulConfigs      data.PCTLM_pctrlmapconf.PCTLMC_ulConfigs
    ULONG                       *PCTLMC_ulConfigs;
#define map_uiNumConfigs    data.PCTLM_pctrlmapconf.PCTLMC_uiNumConfigs
    UINT                         PCTLMC_uiNumConfigs;
} LW_PINCTRL_MAP_CONFIGS;
typedef LW_PINCTRL_MAP_CONFIGS  *PLW_PINCTRL_MAP_CONFIGS;

/*********************************************************************************************************
  引脚映射结构
*********************************************************************************************************/

typedef struct lw_pinctrl_map {
    PLW_DEVTREE_NODE             PCTLM_pdtnDev;                         /*  引脚配置的设备树节点        */
    CPCHAR                       PCTLM_pcName;                          /*  引脚配置的状态名称          */
    LW_PINCTRL_MAP_TYPE          PCTLM_pctlmaptype;                     /*  引脚配置的类型              */
    PLW_DEVTREE_NODE             PCTLM_pdtnCtrlNode;                    /*  引脚控制器设备树节点        */
    union {
        struct lw_pinctrl_map_mux      PCTLM_pctrlmapmux;               /*  引脚复用配置                */
        struct lw_pinctrl_map_configs  PCTLM_pctrlmapconf;              /*  引脚功能配置                */
    } data;
} LW_PINCTRL_MAP;
typedef LW_PINCTRL_MAP          *PLW_PINCTRL_MAP;

typedef struct lw_pinctrl_maps {
    LW_LIST_LINE                 PCTLM_lineManage;                      /*  引脚映射结构链表            */
    LW_LIST_LINE                 PCTLM_lineGlobalManage;                /*  引脚映射结构全局链表        */
    struct lw_pinctrl_dev       *PCTLM_ppinctldev;                      /*  引脚控制器                  */
    struct lw_pinctrl_map       *PCTLM_ppinctlmaps;                     /*  引脚映射结构集合            */
    UINT32                       PCTLM_uiMapsNum;                       /*  引脚映射结构集合元素数量    */
} LW_PINCTRL_MAPS;
typedef LW_PINCTRL_MAPS         *PLW_PINCTRL_MAPS;

/*********************************************************************************************************
  引脚配置复用相关
*********************************************************************************************************/

typedef struct lw_pinctrl_setting_mux {
#define set_uiGroup        data.PCTLS_ppctlsetmux.PCTLSM_uiGroup
    UINT32                      PCTLSM_uiGroup;
#define set_uiFunc         data.PCTLS_ppctlsetmux.PCTLSM_uiFunc
    UINT32                      PCTLSM_uiFunc;
} LW_PINCTRL_SETTING_MUX;
typedef LW_PINCTRL_SETTING_MUX *PLW_PINCTRL_SETTING_MUX;

/*********************************************************************************************************
  引脚配置设置相关
*********************************************************************************************************/

typedef struct lw_pinctrl_setting_configs {
#define set_uiGroupOrPin   data.PCTLS_ppctlsetconf.PCTLSC_uiGroupOrPin
    UINT32                      PCTLSC_uiGroupOrPin;
#define set_pulConfigs     data.PCTLS_ppctlsetconf.PCTLSC_pulConfigs
    ULONG                      *PCTLSC_pulConfigs;
#define set_uiNumConfigs   data.PCTLS_ppctlsetconf.PCTLSC_uiNumConfigs
    UINT32                      PCTLSC_uiNumConfigs;
} LW_PINCTRL_SETTING_CONFIGS;
typedef LW_PINCTRL_SETTING_CONFIGS      *PLW_PINCTRL_SETTING_CONFIGS;

/*********************************************************************************************************
  引脚配置结构
*********************************************************************************************************/

typedef struct lw_pinctrl_setting {
    LW_LIST_LINE                 PCTLS_lineManage;                      /*  引脚配置链表                */
    LW_PINCTRL_MAP_TYPE          PCTLS_pinctlmaptype;                   /*  引脚配置的类型              */
    struct lw_pinctrl_dev       *PCTLS_ppinctldev;                       /*  引脚控制器                  */
    PLW_DEVTREE_NODE             PCTLS_pdtnDev;
    union {
        struct lw_pinctrl_setting_mux      PCTLS_ppctlsetmux;
        struct lw_pinctrl_setting_configs  PCTLS_ppctlsetconf;
    } data;
} LW_PINCTRL_SETTING;
typedef LW_PINCTRL_SETTING       *PLW_PINCTRL_SETTING;

/*********************************************************************************************************
  设备引脚范围 lw_pinctrl_gpio_range
*********************************************************************************************************/

typedef struct lw_pinctrl_gpio_range {
    LW_LIST_LINE               PCTLGR_lineManage;                       /*  引脚范围管理                */
    CPCHAR                     PCTLGR_pcName;                           /*  引脚范围名称                */
    UINT                       PCTLGR_uiGpioBase;                       /*  GPIO 基础偏移               */
    UINT                       PCTLGR_uiPinBase;                        /*  引脚基础偏移                */
    UINT                       PCTLGR_uiNPins;                          /*  引脚数量                    */
} LW_PINCTRL_GPIO_RANGE;
typedef LW_PINCTRL_GPIO_RANGE  *PLW_PINCTRL_GPIO_RANGE;

/*********************************************************************************************************
  引脚管理操作集合
*********************************************************************************************************/

typedef struct lw_pinctrl_ops {
    INT        (*pinctrlGroupCountGet)(PLW_PINCTRL_DEV  ppinctldev);    /*  获取引脚分组数量            */
    CPCHAR     (*pinctrlGroupNameGet)(PLW_PINCTRL_DEV   ppinctldev,
                                      UINT              uiSelector);    /*  获取引脚分组名称            */
    INT        (*pinctrlGroupPinsGet)(PLW_PINCTRL_DEV   ppinctldev,
                                      UINT              uiSelector,
                                      UINT            **ppPins,
                                      UINT             *puiNumPins);    /*  获取引脚分组中的引脚集合    */
    INT        (*pinctrlMapCreate)(PLW_PINCTRL_DEV    ppinctldev,
                                   PLW_DEVTREE_NODE   pdtnDev,
                                   LW_PINCTRL_MAP   **pppinctlmap,
                                   UINT              *puiNumMaps);      /*  创建引脚映射                */
    VOID       (*pinctrlMapFree)(PLW_PINCTRL_DEV  ppinctldev,
                                 LW_PINCTRL_MAP  *ppinctlmap,
                                 UINT             uiNumMaps);           /*  释放引脚映射                */
} LW_PINCTRL_OPS;
typedef LW_PINCTRL_OPS          *PLW_PINCTRL_OPS;

/*********************************************************************************************************
  引脚复用操作集合
*********************************************************************************************************/

typedef struct lw_pinmux_ops {
    INT        (*pinmuxFuncCountGet)(PLW_PINCTRL_DEV   ppinctldev);     /*  获取引脚复用类别总数        */
    CPCHAR     (*pinmuxFuncNameGet)(PLW_PINCTRL_DEV    ppinctldev,
                                    UINT               uiSelector);     /*  获取引脚复用名称            */
    INT        (*pinmuxFuncGroupsGet)(PLW_PINCTRL_DEV  ppinctldev,
                                      UINT             uiSelector,
                                      CHAR          ***pppcGroups,
                                      UINT            *puiNumGroups);   /*  获取引脚组集合              */
    INT        (*pinmuxMuxSet)(PLW_PINCTRL_DEV   ppinctldev,
                               UINT              uiFuncSelector,
                               UINT              uiGroupSelector);      /*  引脚复用设置                */
    INT        (*pinmuxRequest)(PLW_PINCTRL_DEV  ppinctldev,
                                UINT             uiOffset);             /*  引脚申请                    */
    INT        (*pinmuxFree)(PLW_PINCTRL_DEV     ppinctldev,
                             UINT                uiOffset);             /*  引脚释放                    */
    INT        (*pinmuxGpioRequest)(PLW_PINCTRL_DEV             ppinctldev,
                                    PLW_PINCTRL_GPIO_RANGE      pGpioRange,
                                    UINT                        uiPin);
    VOID       (*pinmuxGpioFree)(PLW_PINCTRL_DEV             ppinctldev,
                                 PLW_PINCTRL_GPIO_RANGE      pGpioRange,
                                 UINT                        uiPin);
    INT        (*pinmuxGpioDirectionSet)(PLW_PINCTRL_DEV  ppinctldev,
                                         UINT             uiOffset,
                                         BOOL             bInput);
} LW_PINMUX_OPS;
typedef LW_PINMUX_OPS           *PLW_PINMUX_OPS;

/*********************************************************************************************************
  引脚配置操作集合
*********************************************************************************************************/

typedef struct lw_pinconf_ops {
    INT       (*pinConfigGet)(PLW_PINCTRL_DEV  ppinctldev,
                              UINT             uiPin,
                              ULONG           *pulConfig);              /*  单个引脚配置获取            */
    INT       (*pinConfigSet)(PLW_PINCTRL_DEV  ppinctldev,
                              UINT             uiPin,
                              ULONG           *pulConfigs,
                              UINT             uiNumConfigs);           /*  单个引脚配置设置            */
    INT       (*pinConfigGroupGet)(PLW_PINCTRL_DEV  ppinctldev,
                                   UINT             uiSelector,
                                   ULONG           *pulConfig);         /*  引脚组配置获取              */
    INT       (*pinConfigGroupSet)(PLW_PINCTRL_DEV  ppinctldev,
                                   UINT             uiSelector,
                                   ULONG           *pulConfig,
                                   UINT             uiNumConfigs);      /*  引脚组配置设置              */
} LW_PINCONF_OPS;
typedef LW_PINCONF_OPS         *PLW_PINCONF_OPS;

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PINCTRLCLASS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
