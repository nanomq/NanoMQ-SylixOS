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
** ��   ��   ��: clock.h
**
** ��   ��   ��: Wang.Xuan (���Q)
**
** �ļ���������: 2019 �� 08 �� 26 ��
**
** ��        ��: ʱ�ӿ��ƺ����߼�
*********************************************************************************************************/

#ifndef __CLOCK_H
#define __CLOCK_H

/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  ������
*********************************************************************************************************/

#define LW_CLOCK_IS_ROOT                    0x0001                      /*  ��ʱ������                  */
#define LW_CLOCK_SET_RATE_PARENT            0x0002
#define LW_CLOCK_GET_RATE_NOCACHE           0x0004
#define LW_CLOCK_DIVIDER_ROUND_CLOSEST      0x0008
#define LW_CLOCK_IS_MUX                     0x0010

#define __HW_FLAGS_GET(clk)                 (clk->CLK_ulFlags)
#define __HW_PARENT_GET(clk)                (clk->CLK_clkparent ? clk->CLK_clkparent : LW_NULL)

#ifndef __OFFSET_OF
#define __OFFSET_OF(type, member)           ((ULONG)((CHAR *)&((type *)0)->member - (CHAR *)(type *)0))
#endif
#ifndef __CONTAINER_OF
#define __CONTAINER_OF(ptr, type, member)   ((type *)((CHAR *)ptr - __OFFSET_OF(type, member)))
#endif

#define __CLK_TO_CLK_FIXED_FACTOR(clk)      __CONTAINER_OF(clk, LW_CLOCK_FIXED_FACTOR, CLKFF_clk)
#define __CLK_TO_CLK_FIXED_RATE(clk)        __CONTAINER_OF(clk, LW_CLOCK_FIXED_RATE,   CLKFR_clk)
#define __CLK_TO_CLK_DIVIDER(clk)           __CONTAINER_OF(clk, LW_CLOCK_DIVIDER,      CLKD_clk)
#define __CLK_TO_CLK_GATE(clk)              __CONTAINER_OF(clk, LW_CLOCK_GATE,         CLKG_clk)
#define __CLK_TO_CLK_MUX(clk)               __CONTAINER_OF(clk, LW_CLOCK_MUX,          CLKM_clk)

/*********************************************************************************************************
  ������ǰ����
*********************************************************************************************************/

struct lw_clk;
struct lw_clk_ops;
struct lw_clk_div_table;
struct lw_clk_mux_table;

/*********************************************************************************************************
  ʱ�ӹ���ṹ
*********************************************************************************************************/

typedef struct lw_clk {
    LW_LIST_LINE               CLK_lineManage;                          /*  ʱ�ӹ�������                */
    PCHAR                      CLK_pcName;                              /*  ʱ������                    */
    struct lw_clk_ops         *CLK_clkops;                              /*  ʱ�Ӳ�����                  */
    struct lw_clk             *CLK_clkparent;                           /*  ��ǰ�ĸ�ʱ��                */
    struct lw_clk            **CLK_clkparents;                          /*  ��ʱ�Ӽ���                  */
    CHAR                     **CLK_ppcParentNames;                      /*  ��ʱ�����Ƽ���              */
    UINT                       CLK_uiNumParents;                        /*  ��ʱ������                  */
    ULONG                      CLK_ulFlags;                             /*  ʱ�ӱ�ʾ                    */
    LW_LIST_LINE_HEADER        CLK_plineclkchild;                       /*  ���е���ʱ�ӽڵ�            */

    ULONG                      CLK_ulRate;                              /*  ʱ��Ƶ��                    */
    UINT                       CLK_uiEnableCount;                       /*  ʱ��ʹ�ܼ���                */
    PVOID                      CLK_pvClkInfo;                           /*  ָ���ⲿ�����ʱ����Ϣ����  */
} LW_CLOCK;
typedef LW_CLOCK              *PLW_CLOCK;

/*********************************************************************************************************
  ʱ�ӵĲ�����������

  clockPrepare   : ʱ��ʹ��ǰ��ִ�еĲ�������������������
  clockUnprepare : ʱ�ӽ���ǰ��ִ�еĲ�������������������
  clockIsPrepared: ʱ��ʹ��ǰִ�еĲ����Ƿ����

  clockEnable    : ʱ��ʹ�ܲ�������������������
  clockDisable   : ʱ�ӽ��ܲ�������������������
  clockIsEnabled : ʱ���Ƿ�ʹ��

  clockRateRecalc: ��ȡʱ�ӵ�ǰƵ��
  clockRateRound : ��ȡ��Ԥ��ʱ����ӽ���ʵ��ʱ��Ƶ��
  clockRateSet   : ����ʱ��Ƶ��

  clockParentSet : ���õ�ǰѡ�еĸ�ʱ��
  clockParentGet : ��ȡ��ǰѡ�еĸ�ʱ��

  clockInit      : ʱ�ӳ�ʼ��
*********************************************************************************************************/

typedef struct lw_clk_ops {
    INT     (*clockPrepare   )(PLW_CLOCK  pclk);
    VOID    (*clockUnprepare )(PLW_CLOCK  pclk);
    BOOL    (*clockIsPrepared)(PLW_CLOCK  pclk);

    INT     (*clockEnable    )(PLW_CLOCK  pclk);
    VOID    (*clockDisable   )(PLW_CLOCK  pclk);
    BOOL    (*clockIsEnabled )(PLW_CLOCK  pclk);

    ULONG   (*clockRateRecalc)(PLW_CLOCK  pclk, ULONG  ulParentRate);
    LONG    (*clockRateRound )(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  *pulParentRate);
    INT     (*clockRateSet   )(PLW_CLOCK  pclk, ULONG  ulRate, ULONG  ulParentRate);

    INT     (*clockParentSet )(PLW_CLOCK  pclk, UINT8  uiIndex);
    UINT8   (*clockParentGet )(PLW_CLOCK  pclk);

    VOID    (*clockInit      )(PLW_CLOCK  pclk);
} LW_CLOCK_OPS;
typedef LW_CLOCK_OPS            *PLW_CLOCK_OPS;

/*********************************************************************************************************
  ʱ�����ݹ���ṹ
*********************************************************************************************************/

typedef struct lw_clk_div_table {                                       /*  ʱ�ӷ�Ƶ��                  */
    UINT                       CLKDT_uiVal;                             /*  �Ĵ�����Ӧ��ֵ              */
    UINT                       CLKDT_uiDiv;                             /*  ʱ�ӷ�Ƶֵ                  */
} LW_CLOCK_DIV_TABLE;
typedef LW_CLOCK_DIV_TABLE    *PLW_CLOCK_DIV_TABLE;

typedef struct lw_clk_mux_table {                                       /*  ʱ�Ӷ�·ѡ���              */
    UINT                       CLKMT_uiVal;                             /*  �Ĵ�����Ӧ��ֵ              */
    UINT                       CLKMT_uiSource;                          /*  ʱ��Դ���                  */
} LW_CLOCK_MUX_TABLE;
typedef LW_CLOCK_MUX_TABLE    *PLW_CLOCK_MUX_TABLE;

/*********************************************************************************************************
  ֧�ֵ�ʱ�ӿ�������
*********************************************************************************************************/

typedef struct lw_clk_provider {                                        /*  ͳһ�����ʱ������          */
    LW_LIST_LINE               CLKP_lineManage;
    PLW_DEVTREE_NODE           CLKP_pdtndev;
    PLW_CLOCK                (*CLKP_clkGet)(PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec,
                                            PVOID                     pvData);
    PVOID                      CLKP_pvData;
} LW_CLOCK_PROVIDER;
typedef LW_CLOCK_PROVIDER     *PLW_CLOCK_PROVIDER;

typedef struct lw_clk_fixed_rate {                                      /*  �̶�Ƶ��ʱ��                */
    struct lw_clk              CLKFR_clk;                               /*  ʱ�ӹ���Ԫ                */
    ULONG                      CLKFR_ulFixedRate;                       /*  �̶�ʱ��Ƶ��                */
    ULONG                      CLKFR_ulFlags;                           /*  ʱ�ӱ�ʾ                    */
} LW_CLOCK_FIXED_RATE;
typedef LW_CLOCK_FIXED_RATE   *PLW_CLOCK_FIXED_RATE;

typedef struct lw_clk_fixed_factor {                                    /*  �̶���Ƶ��Ƶʱ��          */
    struct lw_clk              CLKFF_clk;                               /*  ʱ�ӹ���Ԫ                */
    UINT32                     CLKFF_uiFixedMult;                       /*  �̶�ʱ�ӱ�Ƶֵ              */
    UINT32                     CLKFF_uiFixedDiv;                        /*  �̶�ʱ�ӷ�Ƶֵ              */
} LW_CLOCK_FIXED_FACTOR;
typedef LW_CLOCK_FIXED_FACTOR *PLW_CLOCK_FIXED_FACTOR;

typedef struct lw_clk_divider {                                         /*  �ɵ�����Ƶֵ��ʱ��          */
    struct lw_clk              CLKD_clk;                                /*  ʱ�ӹ���Ԫ                */
    struct lw_clk_div_table   *CLKD_pclkdivtable;                       /*  ��Ƶֵ���                  */
    UINT                       CLKD_uiMask;                             /*  �Ĵ�����Ӧ����              */
    UINT                       CLKD_uiShift;                            /*  �Ĵ�����Ӧ��λ              */
    UINT                     (*CLKD_pfuncValGet)(PLW_CLOCK  pclk);      /*  �Ĵ�����ֵ��ȡ              */
    INT                      (*CLKD_pfuncValSet)(PLW_CLOCK  pclk,       /*  �Ĵ�����ֵ����              */
                                                 UINT       uiVal);
} LW_CLOCK_DIVIDER;
typedef LW_CLOCK_DIVIDER      *PLW_CLOCK_DIVIDER;

typedef struct lw_clk_gate {                                            /*  ʱ�ӿ�����                  */
    struct lw_clk             CLKG_clk;                                 /*  ʱ�ӹ���Ԫ                */
    INT                     (*CLKG_pfuncEnable)(PLW_CLOCK     pclk);    /*  ʱ��ʹ��                    */
    INT                     (*CLKG_pfuncDisable)(PLW_CLOCK    pclk);    /*  ʱ�ӽ���                    */
    BOOL                    (*CLKG_pfuncIsEnabled)(PLW_CLOCK  pclk);    /*  ʱ���Ƿ�ʹ��                */
} LW_CLOCK_GATE;
typedef LW_CLOCK_GATE        *PLW_CLOCK_GATE;

typedef struct lw_clk_mux {                                             /*  ��·ѡ����                  */
    struct lw_clk             CLKM_clk;                                 /*  ʱ�ӹ���Ԫ                */
    struct lw_clk_mux_table  *CLKM_pclktable;                           /*  ��·��ѡ���                */
    UINT                      CLKM_uiMask;                              /*  �Ĵ�����Ӧ����              */
    UINT                      CLKM_uiShift;                             /*  �Ĵ�����Ӧ��λ              */
    UINT                    (*CLKM_pfuncValGet)(PLW_CLOCK  pclk);       /*  �Ĵ�����ֵ��ȡ              */
    INT                     (*CLKM_pfuncValSet)(PLW_CLOCK  pclk,        /*  �Ĵ�����ֵ����              */
                                                UINT       uiVal);
} LW_CLOCK_MUX;
typedef LW_CLOCK_MUX         *PLW_CLOCK_MUX;

/*********************************************************************************************************
  API
*********************************************************************************************************/

LW_API INT        API_ClockRegister(PLW_CLOCK   pclk);

LW_API INT        API_ClockParentSet(PLW_CLOCK  pclk, PLW_CLOCK  pclkParent);

LW_API ULONG      API_ClockRateGet(PLW_CLOCK    pclk);

LW_API INT        API_ClockRateSet(PLW_CLOCK    pclk, ULONG  ulReqRate);

LW_API LONG       API_ClockRateRound(PLW_CLOCK  pclk, ULONG  ulRate);

LW_API INT        API_ClockInitDataSet(PLW_CLOCK      pclk,          CPCHAR  pcName,
                                       PLW_CLOCK_OPS  pclkops,       ULONG   ulFlags,
                                       CHAR         **ppcParentName, UINT    uiParentNum);

LW_API PLW_CLOCK  API_ClockSimpleGet(PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec, PVOID  pvData);

LW_API INT        API_ClockProviderAdd(PLW_DEVTREE_NODE  pdtndev,
                                       PLW_CLOCK       (*pfuncClkGet)(PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec,
                                                                      PVOID                     pvData),
                                       PVOID             pvData);

LW_API PLW_CLOCK  API_ClockGetFromProvider(PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec);

LW_API PLW_CLOCK  API_ClockFind(CPCHAR        pcName);

LW_API INT        API_ClockEnable(PLW_CLOCK   pclk);

LW_API VOID       API_ClockDisable(PLW_CLOCK  pclk);

LW_API INT        API_ClockCoreInit(VOID);

LW_API PLW_CLOCK  API_ClockDividerRegister(CPCHAR               pcName,
                                           CHAR               **ppcParentName,
                                           ULONG                ulFlags,
                                           PLW_CLOCK_DIV_TABLE  pclkdivtable,
                                           UINT                 uiMask,
                                           UINT                 uiShift,
                                           UINTFUNCPTR          pfuncValGet,
                                           FUNCPTR              pfuncValSet);

LW_API PLW_CLOCK  API_ClockFixedFactorRegister(CPCHAR  pcName,
                                               PCHAR  *ppcParentName,
                                               ULONG   ulFlags,
                                               UINT    uiFixedMult,
                                               UINT    uiFixedDiv);

LW_API PLW_CLOCK  API_ClockFixedRateRegister(CPCHAR    pcName,
                                             ULONG     ulFlags,
                                             ULONG     ulFixedRate);

LW_API PLW_CLOCK  API_ClockGateRegister(CPCHAR       pcName,
                                        PCHAR       *ppcParentName,
                                        ULONG        ulFlags,
                                        FUNCPTR      pfuncEnable,
                                        FUNCPTR      pfuncDisable,
                                        BOOLFUNCPTR  pfuncIsEnabled);

LW_API PLW_CLOCK  API_ClockMuxRegister(CPCHAR               pcName,
                                       PCHAR               *ppcParentName,
                                       ULONG                ulFlags,
                                       UINT                 uiParentNum,
                                       PLW_CLOCK_MUX_TABLE  pclkmuxtable,
                                       UINT                 uiMask,
                                       UINT                 uiShift,
                                       UINTFUNCPTR          pfuncValGet,
                                       FUNCPTR              pfuncValSet);

#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0) &&   */
                                                                        /*  (LW_CFG_DEVTREE_EN > 0)     */
#endif                                                                  /*  __CLOCK_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
