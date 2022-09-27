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
** ��   ��   ��: pinCtrlClass.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 12 ��
**
** ��        ��: pinctrl ��ϵͳ�������Ͷ���
*********************************************************************************************************/

#ifndef __PINCTRLCLASS_H
#define __PINCTRLCLASS_H

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  ����ӳ��ṹ����
*********************************************************************************************************/

typedef enum {
    PIN_MAP_TYPE_INVALID,
    PIN_MAP_TYPE_DUMMY_STATE,                                           /*  �������������              */
    PIN_MAP_TYPE_MUX_GROUP,                                             /*  ����������Ÿ�������        */
    PIN_MAP_TYPE_CONFIGS_PIN,                                           /*  ���յ������Ź�������        */
    PIN_MAP_TYPE_CONFIGS_GROUP,                                         /*  ����������Ź�������        */
} LW_PINCTRL_MAP_TYPE;

struct lw_pinctrl_desc;
struct lw_pinctrl;
struct lw_pinctrl_state;
struct lw_pinctrl_ops;
struct lw_pinmux_ops;
struct lw_pinconf_ops;
struct lw_pinctrl_gpio_range;

/*********************************************************************************************************
  ���ſ�����ص��������� (������ʹ��)
*********************************************************************************************************/

typedef struct lw_pinctrl_pin_desc {
    UINT32                       PCTLPD_uiIndex;                        /*  �������                    */
    PCHAR                        PCTLPD_pcName;                         /*  ��������                    */
    PVOID                        PCTLPD_pvData;                         /*  ˽������                    */
} LW_PINCTRL_PIN_DESC;
typedef LW_PINCTRL_PIN_DESC     *PLW_PINCTRL_PIN_DESC;

#define PINCTRL_PIN(a, b)   { .PCTLPD_uiIndex = a, .PCTLPD_pcName = b }

/*********************************************************************************************************
  �豸��ص��������� (�ں���ʹ��)
*********************************************************************************************************/

typedef struct lw_pin_desc {
    LW_LIST_LINE                PIN_lineManage;                         /*  ��������                    */
    struct lw_pinctrl_dev      *PIN_ppinctldev;                         /*  ���ſ�����ָ��              */
    UINT                        PIN_uiPin;                              /*  �������                    */
    CPCHAR                      PIN_pcName;                             /*  ��������                    */
    PVOID                       PIN_pvData;                             /*  ˽������                    */
} LW_PIN_DESC;
typedef LW_PIN_DESC            *PLW_PIN_DESC;

/*********************************************************************************************************
  ���ſ������豸
*********************************************************************************************************/

typedef struct lw_pinctrl_dev {
    LW_LIST_LINE                 PCTLD_lineManage;                      /*  ���ſ���������              */
    LW_LIST_LINE_HEADER          PCTLD_plineDescs;                      /*  ���������ļ���              */
    struct lw_pinctrl           *PCTLD_ppinctrl;                        /*  ���Žڵ�����                */
    struct lw_pinctrl_desc      *PCTLD_ppinctldesc;                     /*  ���ſ�������                */
    PLW_DEVTREE_NODE             PCTLD_pdtnDev;                         /*  ���ſ��������豸���ڵ�      */
    PVOID                        PCTLD_pvData;                          /*  ˽������                    */
    LW_LIST_LINE_HEADER          PCTLD_plineGpioRange;                  /*  GPIO ��Χ                   */
} LW_PINCTRL_DEV;
typedef LW_PINCTRL_DEV          *PLW_PINCTRL_DEV;

/*********************************************************************************************************
  ���ſ�������
*********************************************************************************************************/

typedef struct lw_pinctrl_desc {
    PCHAR                        PCTLD_pcName;                          /*  ���ſ�������                */
    UINT                         PCTLD_uiPinsNum;                       /*  ���Ÿ���                    */
    struct lw_pinctrl_pin_desc  *PCTLD_ppinctrlpindesc;                 /*  ������������                */
    struct lw_pinctrl_ops       *PCTLD_ppinctlops;                      /*  ���ſ��Ʋ�������            */
    struct lw_pinmux_ops        *PCTLD_ppinmuxops;                      /*  ���Ÿ��ò�������            */
    struct lw_pinconf_ops       *PCTLD_ppinconfops;                     /*  �������ò�������            */
} LW_PINCTRL_DESC;
typedef LW_PINCTRL_DESC         *PLW_PINCTRL_DESC;

/*********************************************************************************************************
  ���ſ���ӳ�临�����
*********************************************************************************************************/

typedef struct lw_pinctrl_map_mux {
#define map_pcGroup         data.PCTLM_pctrlmapmux.PCTLMM_pcGroup
    CPCHAR                       PCTLMM_pcGroup;
#define map_pcFunction      data.PCTLM_pctrlmapmux.PCTLMM_pcFunction
    CPCHAR                       PCTLMM_pcFunction;
} LW_PINCTRL_MAP_MUX;
typedef LW_PINCTRL_MAP_MUX      *PLW_PINCTRL_MAP_MUX;

/*********************************************************************************************************
  ���ſ���ӳ���������
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
  ����ӳ��ṹ
*********************************************************************************************************/

typedef struct lw_pinctrl_map {
    PLW_DEVTREE_NODE             PCTLM_pdtnDev;                         /*  �������õ��豸���ڵ�        */
    CPCHAR                       PCTLM_pcName;                          /*  �������õ�״̬����          */
    LW_PINCTRL_MAP_TYPE          PCTLM_pctlmaptype;                     /*  �������õ�����              */
    PLW_DEVTREE_NODE             PCTLM_pdtnCtrlNode;                    /*  ���ſ������豸���ڵ�        */
    union {
        struct lw_pinctrl_map_mux      PCTLM_pctrlmapmux;               /*  ���Ÿ�������                */
        struct lw_pinctrl_map_configs  PCTLM_pctrlmapconf;              /*  ���Ź�������                */
    } data;
} LW_PINCTRL_MAP;
typedef LW_PINCTRL_MAP          *PLW_PINCTRL_MAP;

typedef struct lw_pinctrl_maps {
    LW_LIST_LINE                 PCTLM_lineManage;                      /*  ����ӳ��ṹ����            */
    LW_LIST_LINE                 PCTLM_lineGlobalManage;                /*  ����ӳ��ṹȫ������        */
    struct lw_pinctrl_dev       *PCTLM_ppinctldev;                      /*  ���ſ�����                  */
    struct lw_pinctrl_map       *PCTLM_ppinctlmaps;                     /*  ����ӳ��ṹ����            */
    UINT32                       PCTLM_uiMapsNum;                       /*  ����ӳ��ṹ����Ԫ������    */
} LW_PINCTRL_MAPS;
typedef LW_PINCTRL_MAPS         *PLW_PINCTRL_MAPS;

/*********************************************************************************************************
  �������ø������
*********************************************************************************************************/

typedef struct lw_pinctrl_setting_mux {
#define set_uiGroup        data.PCTLS_ppctlsetmux.PCTLSM_uiGroup
    UINT32                      PCTLSM_uiGroup;
#define set_uiFunc         data.PCTLS_ppctlsetmux.PCTLSM_uiFunc
    UINT32                      PCTLSM_uiFunc;
} LW_PINCTRL_SETTING_MUX;
typedef LW_PINCTRL_SETTING_MUX *PLW_PINCTRL_SETTING_MUX;

/*********************************************************************************************************
  ���������������
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
  �������ýṹ
*********************************************************************************************************/

typedef struct lw_pinctrl_setting {
    LW_LIST_LINE                 PCTLS_lineManage;                      /*  ������������                */
    LW_PINCTRL_MAP_TYPE          PCTLS_pinctlmaptype;                   /*  �������õ�����              */
    struct lw_pinctrl_dev       *PCTLS_ppinctldev;                       /*  ���ſ�����                  */
    PLW_DEVTREE_NODE             PCTLS_pdtnDev;
    union {
        struct lw_pinctrl_setting_mux      PCTLS_ppctlsetmux;
        struct lw_pinctrl_setting_configs  PCTLS_ppctlsetconf;
    } data;
} LW_PINCTRL_SETTING;
typedef LW_PINCTRL_SETTING       *PLW_PINCTRL_SETTING;

/*********************************************************************************************************
  �豸���ŷ�Χ lw_pinctrl_gpio_range
*********************************************************************************************************/

typedef struct lw_pinctrl_gpio_range {
    LW_LIST_LINE               PCTLGR_lineManage;                       /*  ���ŷ�Χ����                */
    CPCHAR                     PCTLGR_pcName;                           /*  ���ŷ�Χ����                */
    UINT                       PCTLGR_uiGpioBase;                       /*  GPIO ����ƫ��               */
    UINT                       PCTLGR_uiPinBase;                        /*  ���Ż���ƫ��                */
    UINT                       PCTLGR_uiNPins;                          /*  ��������                    */
} LW_PINCTRL_GPIO_RANGE;
typedef LW_PINCTRL_GPIO_RANGE  *PLW_PINCTRL_GPIO_RANGE;

/*********************************************************************************************************
  ���Ź����������
*********************************************************************************************************/

typedef struct lw_pinctrl_ops {
    INT        (*pinctrlGroupCountGet)(PLW_PINCTRL_DEV  ppinctldev);    /*  ��ȡ���ŷ�������            */
    CPCHAR     (*pinctrlGroupNameGet)(PLW_PINCTRL_DEV   ppinctldev,
                                      UINT              uiSelector);    /*  ��ȡ���ŷ�������            */
    INT        (*pinctrlGroupPinsGet)(PLW_PINCTRL_DEV   ppinctldev,
                                      UINT              uiSelector,
                                      UINT            **ppPins,
                                      UINT             *puiNumPins);    /*  ��ȡ���ŷ����е����ż���    */
    INT        (*pinctrlMapCreate)(PLW_PINCTRL_DEV    ppinctldev,
                                   PLW_DEVTREE_NODE   pdtnDev,
                                   LW_PINCTRL_MAP   **pppinctlmap,
                                   UINT              *puiNumMaps);      /*  ��������ӳ��                */
    VOID       (*pinctrlMapFree)(PLW_PINCTRL_DEV  ppinctldev,
                                 LW_PINCTRL_MAP  *ppinctlmap,
                                 UINT             uiNumMaps);           /*  �ͷ�����ӳ��                */
} LW_PINCTRL_OPS;
typedef LW_PINCTRL_OPS          *PLW_PINCTRL_OPS;

/*********************************************************************************************************
  ���Ÿ��ò�������
*********************************************************************************************************/

typedef struct lw_pinmux_ops {
    INT        (*pinmuxFuncCountGet)(PLW_PINCTRL_DEV   ppinctldev);     /*  ��ȡ���Ÿ����������        */
    CPCHAR     (*pinmuxFuncNameGet)(PLW_PINCTRL_DEV    ppinctldev,
                                    UINT               uiSelector);     /*  ��ȡ���Ÿ�������            */
    INT        (*pinmuxFuncGroupsGet)(PLW_PINCTRL_DEV  ppinctldev,
                                      UINT             uiSelector,
                                      CHAR          ***pppcGroups,
                                      UINT            *puiNumGroups);   /*  ��ȡ�����鼯��              */
    INT        (*pinmuxMuxSet)(PLW_PINCTRL_DEV   ppinctldev,
                               UINT              uiFuncSelector,
                               UINT              uiGroupSelector);      /*  ���Ÿ�������                */
    INT        (*pinmuxRequest)(PLW_PINCTRL_DEV  ppinctldev,
                                UINT             uiOffset);             /*  ��������                    */
    INT        (*pinmuxFree)(PLW_PINCTRL_DEV     ppinctldev,
                             UINT                uiOffset);             /*  �����ͷ�                    */
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
  �������ò�������
*********************************************************************************************************/

typedef struct lw_pinconf_ops {
    INT       (*pinConfigGet)(PLW_PINCTRL_DEV  ppinctldev,
                              UINT             uiPin,
                              ULONG           *pulConfig);              /*  �����������û�ȡ            */
    INT       (*pinConfigSet)(PLW_PINCTRL_DEV  ppinctldev,
                              UINT             uiPin,
                              ULONG           *pulConfigs,
                              UINT             uiNumConfigs);           /*  ����������������            */
    INT       (*pinConfigGroupGet)(PLW_PINCTRL_DEV  ppinctldev,
                                   UINT             uiSelector,
                                   ULONG           *pulConfig);         /*  ���������û�ȡ              */
    INT       (*pinConfigGroupSet)(PLW_PINCTRL_DEV  ppinctldev,
                                   UINT             uiSelector,
                                   ULONG           *pulConfig,
                                   UINT             uiNumConfigs);      /*  ��������������              */
} LW_PINCONF_OPS;
typedef LW_PINCONF_OPS         *PLW_PINCONF_OPS;

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __PINCTRLCLASS_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
