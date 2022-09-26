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
** 文   件   名: clock.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2019 年 08 月 26 日
**
** 描        述: 时钟控制核心逻辑
*********************************************************************************************************/

#ifndef __CLOCK_H
#define __CLOCK_H

/*********************************************************************************************************
  裁减控制
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_DEVTREE_EN > 0)

/*********************************************************************************************************
  操作宏
*********************************************************************************************************/

#define LW_CLOCK_IS_ROOT                    0x0001                      /*  根时钟类型                  */
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
  类型提前声明
*********************************************************************************************************/

struct lw_clk;
struct lw_clk_ops;
struct lw_clk_div_table;
struct lw_clk_mux_table;

/*********************************************************************************************************
  时钟管理结构
*********************************************************************************************************/

typedef struct lw_clk {
    LW_LIST_LINE               CLK_lineManage;                          /*  时钟管理链表                */
    PCHAR                      CLK_pcName;                              /*  时钟名称                    */
    struct lw_clk_ops         *CLK_clkops;                              /*  时钟操作集                  */
    struct lw_clk             *CLK_clkparent;                           /*  当前的父时钟                */
    struct lw_clk            **CLK_clkparents;                          /*  父时钟集合                  */
    CHAR                     **CLK_ppcParentNames;                      /*  父时钟名称集合              */
    UINT                       CLK_uiNumParents;                        /*  父时钟数量                  */
    ULONG                      CLK_ulFlags;                             /*  时钟标示                    */
    LW_LIST_LINE_HEADER        CLK_plineclkchild;                       /*  所有的子时钟节点            */

    ULONG                      CLK_ulRate;                              /*  时钟频率                    */
    UINT                       CLK_uiEnableCount;                       /*  时钟使能计数                */
    PVOID                      CLK_pvClkInfo;                           /*  指向外部定义的时钟信息集合  */
} LW_CLOCK;
typedef LW_CLOCK              *PLW_CLOCK;

/*********************************************************************************************************
  时钟的操作函数集合

  clockPrepare   : 时钟使能前需执行的操作，可以有阻塞操作
  clockUnprepare : 时钟禁能前需执行的操作，可以有阻塞操作
  clockIsPrepared: 时钟使能前执行的操作是否完成

  clockEnable    : 时钟使能操作，不可有阻塞操作
  clockDisable   : 时钟禁能操作，不可有阻塞操作
  clockIsEnabled : 时钟是否使能

  clockRateRecalc: 获取时钟当前频率
  clockRateRound : 获取与预期时钟最接近的实际时钟频率
  clockRateSet   : 设置时钟频率

  clockParentSet : 设置当前选中的父时钟
  clockParentGet : 获取当前选中的父时钟

  clockInit      : 时钟初始化
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
  时钟数据管理结构
*********************************************************************************************************/

typedef struct lw_clk_div_table {                                       /*  时钟分频表                  */
    UINT                       CLKDT_uiVal;                             /*  寄存器对应数值              */
    UINT                       CLKDT_uiDiv;                             /*  时钟分频值                  */
} LW_CLOCK_DIV_TABLE;
typedef LW_CLOCK_DIV_TABLE    *PLW_CLOCK_DIV_TABLE;

typedef struct lw_clk_mux_table {                                       /*  时钟多路选择表              */
    UINT                       CLKMT_uiVal;                             /*  寄存器对应数值              */
    UINT                       CLKMT_uiSource;                          /*  时钟源序号                  */
} LW_CLOCK_MUX_TABLE;
typedef LW_CLOCK_MUX_TABLE    *PLW_CLOCK_MUX_TABLE;

/*********************************************************************************************************
  支持的时钟控制类型
*********************************************************************************************************/

typedef struct lw_clk_provider {                                        /*  统一抽象的时钟类型          */
    LW_LIST_LINE               CLKP_lineManage;
    PLW_DEVTREE_NODE           CLKP_pdtndev;
    PLW_CLOCK                (*CLKP_clkGet)(PLW_DEVTREE_PHANDLE_ARGS  pdtpaClkSpec,
                                            PVOID                     pvData);
    PVOID                      CLKP_pvData;
} LW_CLOCK_PROVIDER;
typedef LW_CLOCK_PROVIDER     *PLW_CLOCK_PROVIDER;

typedef struct lw_clk_fixed_rate {                                      /*  固定频率时钟                */
    struct lw_clk              CLKFR_clk;                               /*  时钟管理单元                */
    ULONG                      CLKFR_ulFixedRate;                       /*  固定时钟频率                */
    ULONG                      CLKFR_ulFlags;                           /*  时钟标示                    */
} LW_CLOCK_FIXED_RATE;
typedef LW_CLOCK_FIXED_RATE   *PLW_CLOCK_FIXED_RATE;

typedef struct lw_clk_fixed_factor {                                    /*  固定分频或倍频时钟          */
    struct lw_clk              CLKFF_clk;                               /*  时钟管理单元                */
    UINT32                     CLKFF_uiFixedMult;                       /*  固定时钟倍频值              */
    UINT32                     CLKFF_uiFixedDiv;                        /*  固定时钟分频值              */
} LW_CLOCK_FIXED_FACTOR;
typedef LW_CLOCK_FIXED_FACTOR *PLW_CLOCK_FIXED_FACTOR;

typedef struct lw_clk_divider {                                         /*  可调整分频值的时钟          */
    struct lw_clk              CLKD_clk;                                /*  时钟管理单元                */
    struct lw_clk_div_table   *CLKD_pclkdivtable;                       /*  分频值表格                  */
    UINT                       CLKD_uiMask;                             /*  寄存器对应掩码              */
    UINT                       CLKD_uiShift;                            /*  寄存器对应移位              */
    UINT                     (*CLKD_pfuncValGet)(PLW_CLOCK  pclk);      /*  寄存器数值获取              */
    INT                      (*CLKD_pfuncValSet)(PLW_CLOCK  pclk,       /*  寄存器数值设置              */
                                                 UINT       uiVal);
} LW_CLOCK_DIVIDER;
typedef LW_CLOCK_DIVIDER      *PLW_CLOCK_DIVIDER;

typedef struct lw_clk_gate {                                            /*  时钟控制门                  */
    struct lw_clk             CLKG_clk;                                 /*  时钟管理单元                */
    INT                     (*CLKG_pfuncEnable)(PLW_CLOCK     pclk);    /*  时钟使能                    */
    INT                     (*CLKG_pfuncDisable)(PLW_CLOCK    pclk);    /*  时钟禁能                    */
    BOOL                    (*CLKG_pfuncIsEnabled)(PLW_CLOCK  pclk);    /*  时钟是否使能                */
} LW_CLOCK_GATE;
typedef LW_CLOCK_GATE        *PLW_CLOCK_GATE;

typedef struct lw_clk_mux {                                             /*  多路选择门                  */
    struct lw_clk             CLKM_clk;                                 /*  时钟管理单元                */
    struct lw_clk_mux_table  *CLKM_pclktable;                           /*  多路复选表格                */
    UINT                      CLKM_uiMask;                              /*  寄存器对应掩码              */
    UINT                      CLKM_uiShift;                             /*  寄存器对应移位              */
    UINT                    (*CLKM_pfuncValGet)(PLW_CLOCK  pclk);       /*  寄存器数值获取              */
    INT                     (*CLKM_pfuncValSet)(PLW_CLOCK  pclk,        /*  寄存器数值设置              */
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
