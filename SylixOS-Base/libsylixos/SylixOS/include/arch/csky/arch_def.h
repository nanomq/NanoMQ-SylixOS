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
** 文   件   名: arch_def.h
**
** 创   建   人: Wang.Xuan (王翾)
**
** 文件创建日期: 2018 年 05 月 07 日
**
** 描        述: C-SKY 相关定义.
*********************************************************************************************************/

#ifndef __CSKY_ARCH_DEF_H
#define __CSKY_ARCH_DEF_H

#include "asm/archprob.h"

/*********************************************************************************************************
  SSEG0 SSEG1 地址转换
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define CSKY_SSEG0_PA(va)           ((va) & 0x7fffffff)
#define CSKY_SSEG1_PA(va)           ((va) & 0x1fffffff)

#define CSKY_SSEG0_VA(pa)           ((pa) | 0x80000000)
#define CSKY_SSEG1_VA(pa)           ((pa) | 0xa0000000)

/*********************************************************************************************************
  C-SKY 指令
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef UINT16                      CSKY_INSTRUCTION;
#define IS_T32(hi16)                (((hi16) & 0xc000) == 0xc000)
#endif                                                                  /*  !defined(__ASSEMBLY__)      */

/*********************************************************************************************************
  PSR Process Status Register (CR0)
    31  30                              24  23                         16
  +---+--------------------------------+---------------------------------+
  | S |      Reserved                  |            VEC[7:0]             |
  +------+----------------------------------+----------------------------+
   15     14  13  12   11  10   9    8    7    6   5    4   3  2   1   0
  +---------+---+----+-------+----+----+----+----+---+----+------+---+---+
  | TM[1:0] | 0 | TE |   0   | MM | EE | IC | IE | 0 | FE |   0  |AF*| C |
  +---------+---+----+-------+----+----+----+----+---+----+------+---+---+
*********************************************************************************************************/

#define M_PSR_S         (1 << S_PSR_S)
#define S_PSR_S         31
#define M_PSR_TE        (1 << S_PSR_TE)
#define S_PSR_TE        12
#define M_PSR_MM        (1 << S_PSR_MM)
#define S_PSR_MM        9
#define M_PSR_EE        (1 << S_PSR_EE)
#define S_PSR_EE        8
#define M_PSR_IC        (1 << S_PSR_IC)
#define S_PSR_IC        7
#define M_PSR_IE        (1 << S_PSR_IE)
#define S_PSR_IE        6
#define M_PSR_FE        (1 << S_PSR_FE)
#define S_PSR_FE        4
#define M_PSR_AF        (1 << S_PSR_AF)
#define S_PSR_AF        1
#define M_PSR_C         (1 << S_PSR_C)
#define S_PSR_C         0

#if defined(__SYLIXOS_CSKY_ARCH_CK860__)

/*********************************************************************************************************
  CFR Cache Function Register (CR17)
   31                                    18    17        16
  +----------------------------------------+---------+----------+
  |             Reserved                   | BTB_INV | BHT_INV  |
  +----------------------------------------+---------+----------+
   15                            6   5     4   3   2  1        0
  +-------------------------------+-----+-----+-----+-----------+
  |            Reserved           | CLR | INV |  0  | CACHE_SEL |
  +-------------------------------+-----+-----+-----+-----------+
*********************************************************************************************************/

#define M_CFR_BTB_INV   (1 << S_CFR_BTB_INV)
#define S_CFR_BTB_INV   17
#define M_CFR_BHT_INV   (1 << S_CFR_BHT_INV)
#define S_CFR_BHT_INV   16
#define M_CFR_CLR       (1 << S_CFR_CLR)
#define S_CFR_CLR       5
#define M_CFR_INV       (1 << S_CFR_INV)
#define S_CFR_INV       4
#define M_CFR_CACHE_SEL (3 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_I   (1 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_D   (2 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_A   (3 << S_CFR_CACHE_SEL)
#define S_CFR_CACHE_SEL 0

/*********************************************************************************************************
  以下的 bit 位在 CK860 手册上没有，但是 linux 代码中也用到了
*********************************************************************************************************/

#define M_CFR_LICF      (1 << S_CFR_LICF)
#define S_CFR_LICF      31
#define M_CFR_ITS       (1 << S_CFR_ITS)
#define S_CFR_ITS       7
#define M_CFR_OMS       (1 << S_CFR_OMS)
#define S_CFR_OMS       6
#else

/*********************************************************************************************************
  CFR Cache Function Register (CR17)
     31    30                            18    17        16
  +------+---------------------------------+---------+----------+
  | LICF |      Reserved                   | BTB_INV | BHT_INV  |
  +------+---------------------------------+---------+----------+
   15       9     8      7     6     5     4    3  2   1    0    
  +----------+--------+-----+-----+-----+-----+-----+-----------+
  | Reserved | UNLOCK | ITS | OMS | CLR | INV |  0  | CACHE_SEL |
  +----------+--------+-----+-----+-----+-----+-----+-----------+
*********************************************************************************************************/

#define M_CFR_LICF      (1 << S_CFR_LICF)
#define S_CFR_LICF      31
#define M_CFR_BTB_INV   (1 << S_CFR_BTB_INV)
#define S_CFR_BTB_INV   17
#define M_CFR_BHT_INV   (1 << S_CFR_BHT_INV)
#define S_CFR_BHT_INV   16
#define M_CFR_UNLOCK    (1 << S_CFR_UNLOCK)
#define S_CFR_UNLOCK    8
#define M_CFR_ITS       (1 << S_CFR_ITS)
#define S_CFR_ITS       7
#define M_CFR_OMS       (1 << S_CFR_OMS)
#define S_CFR_OMS       6
#define M_CFR_CLR       (1 << S_CFR_CLR)
#define S_CFR_CLR       5
#define M_CFR_INV       (1 << S_CFR_INV)
#define S_CFR_INV       4
#define M_CFR_CACHE_SEL (3 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_I   (1 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_D   (2 << S_CFR_CACHE_SEL)
#define B_CFR_CACHE_A   (3 << S_CFR_CACHE_SEL)
#define S_CFR_CACHE_SEL 0

#endif                                                                  /*  __SYLIXOS_CSKY_ARCH_CK860__ */

/*********************************************************************************************************
  MSA0 MMU SSEG0 Config Register (cr30)

  31   29 28                   7  6   5   4   3   2   1   0
  +------+----------------------+---+---+---+---+---+---+---+
  |  BA  |      Reserved        | B | SO|SEC| C | D | V | 0 |
  +------+----------------------+---+---+---+---+---+---+---+

  CK860: MSA0 MMU SSEG0 Config Register (cr<30,15>)

  31   29 28           9  8   7   6   5   4   3   2   1   0
  +------+----------------------+---+---+---+---+---+---+---+
  |  BA  |    Reserved  | 0 | 0 | B | SO|SEC| C | D | V | 0 |
  +------+----------------------+---+---+---+---+---+---+---+
*********************************************************************************************************/

#define M_MSA0_BA       (0x7 << S_MSA0_BA)                              /*  BA-SSEG0 映射的物理地址     */
#define S_MSA0_BA       29
#define M_MSA0_B        (0x1 << S_MSA0_B)                               /*  指示 SSEG0 区是否可 Buffer  */
#define S_MSA0_B        6
#define M_MSA0_SO       (0x1 << S_MSA0_SO)                              /*  是否和程序流的访问顺序相同  */
#define S_MSA0_SO       5 
#define M_MSA0_SEC      (0x1 << S_MSA0_SEC)                             /*  是否支持安全访问            */
#define S_MSA0_SEC      4
#define M_MSA0_C        (0x1 << S_MSA0_C)                               /*  SSEG0 区是否可高速缓冲      */
#define S_MSA0_C        3        
#define M_MSA0_D        (0x1 << S_MSA0_D)                               /*  指示 SSEG0 区是否可写       */
#define S_MSA0_D        2            
#define M_MSA0_V        (0x1 << S_MSA0_V)                               /*  指示 SSEG0 区映射是否有效   */
#define S_MSA0_V        1                                                                          

/*********************************************************************************************************
  MSA1 MMU SSEG1 Config Register (cr31)

  31   29 28                   7  6   5   4   3   2   1   0
  +------+----------------------+---+---+---+---+---+---+---+
  |  BA  |      Reserved        | B | SO|SEC| C | D | V | 0 |
  +------+----------------------+---+---+---+---+---+---+---+

  CK860: MSA1 MMU SSEG1 Config Register (cr<31,15>)

  31   29 28           9  8   7   6   5   4   3   2   1   0
  +------+----------------------+---+---+---+---+---+---+---+
  |  BA  |    Reserved  | 0 | 0 | B | SO|SEC| C | D | V | 0 |
  +------+----------------------+---+---+---+---+---+---+---+
*********************************************************************************************************/

#define M_MSA1_BA       (0x7 << S_MSA1_BA)                              /*  BA-SSEG1 映射的物理地址     */
#define S_MSA1_BA       29
#define M_MSA1_B        (0x1 << S_MSA1_B)                               /*  指示 SSEG1 区是否可 Buffer  */
#define S_MSA1_B        6
#define M_MSA1_SO       (0x1 << S_MSA1_SO)                              /*  是否和程序流的访问顺序相同  */
#define S_MSA1_SO       5 
#define M_MSA1_SEC      (0x1 << S_MSA1_SEC)                             /*  是否支持安全访问            */
#define S_MSA1_SEC      4
#define M_MSA1_C        (0x1 << S_MSA1_C)                               /*  SSEG1 区是否可高速缓冲      */
#define S_MSA1_C        3        
#define M_MSA1_D        (0x1 << S_MSA1_D)                               /*  指示 SSEG1 区是否可写       */
#define S_MSA1_D        2            
#define M_MSA1_V        (0x1 << S_MSA1_V)                               /*  指示 SSEG1 区映射是否有效   */
#define S_MSA1_V        1                                                                          

/*********************************************************************************************************
  C-SKY CACHE 属性
*********************************************************************************************************/
/*********************************************************************************************************
  CCR Cache Configuration Register (CR18)
   31                                    18    17         16
  +----------------------------------------+---------+----------+
  |             Reserved                   |   UCME  |   IPE    |
  +----------------------------------------+---------+----------+
   15    14    13    12  11  10  8  7  6   5    4    3    2 1  0
  +---+-----+------+----+---+-----+--+---+----+----+----+--+----+
  | 0 | WBR | E_V2 | WA |BTB| SCK |BE| Z | RS | WB | DE |IE| MP |
  +---+-----+------+----+---+-----+--+---+----+----+----+--+----+
*********************************************************************************************************/

#define M_CACHE_CFG_UCME        (0x1 << S_CACHE_CFG_UCME)               /*  普通用户执行 CACHE 使能位   */
#define S_CACHE_CFG_UCME        17
#define M_CACHE_CFG_IPE         (0x1 << S_CACHE_CFG_IPE)                /*  间接分支跳转预测使能位      */
#define S_CACHE_CFG_IPE         16
#define M_CACHE_CFG_WBR         (0x1 << S_CACHE_CFG_WBR)                /*  写突发传输使能位            */
                                                                        /*  CK860 默认为 1, 不可设置    */
#define S_CACHE_CFG_WBR         14
#define M_CACHE_CFG_E_V2        (0x1 << S_CACHE_CFG_E_V2)               /*  大小端版本选择位            */
#define S_CACHE_CFG_E_V2        13
#define M_CACHE_CFG_WA          (0x1 << S_CACHE_CFG_WA)                 /*  高速缓存写分配有效设置      */
#define S_CACHE_CFG_WA          12
#define M_CACHE_CFG_BTB         (0x1 << S_CACHE_CFG_BTB)                /*  分支目标预测使能位          */
#define S_CACHE_CFG_BTB         11
#define M_CACHE_CFG_SCK         (0x7 << S_CACHE_CFG_SCK)                /*  系统和处理器时钟比 = SCK + 1*/
#define S_CACHE_CFG_SCK         8
#define M_CACHE_CFG_BE          (0x1 << S_CACHE_CFG_BE)                 /*  大端模式                    */
#define S_CACHE_CFG_BE          7
#define M_CACHE_CFG_Z           (0x1 << S_CACHE_CFG_Z)                  /*  允许预测跳转设置位          */
#define S_CACHE_CFG_Z           6
#define M_CACHE_CFG_RS          (0x1 << S_CACHE_CFG_RS)                 /*  地址返回栈设置位            */
#define S_CACHE_CFG_RS          5
#define M_CACHE_CFG_WB          (0x1 << S_CACHE_CFG_WB)                 /*  高速缓存写回设置位          */
                                                                        /*  CK860 2.4 手册上此位为 0    */
#define S_CACHE_CFG_WB          4
#define M_CACHE_CFG_DE          (0x1 << S_CACHE_CFG_DE)                 /*  数据高速缓存设置位          */
#define S_CACHE_CFG_DE          3
#define M_CACHE_CFG_IE          (0x1 << S_CACHE_CFG_IE)                 /*  指令高速缓存设置位          */
#define S_CACHE_CFG_IE          2
#define M_CACHE_CFG_MP          (0x3 << S_CACHE_CFG_MP)                 /*  内存保护设置位              */
#define B_CACHE_CFG_MP_INVALID  (0x0 << S_CACHE_CFG_MP)                 /*  MMU 无效                    */
#define B_CACHE_CFG_MP_VALID    (0x1 << S_CACHE_CFG_MP)                 /*  MMU 有效                    */
#define S_CACHE_CFG_MP          0

/*********************************************************************************************************
  CCR2 Cache Configuration Register2 (CR23)
     31   30  29 28 26    25    24   22  21 20    19     18   16
  +------+------+-----+--------+--------+-----+--------+--------+
  | TPRF | IPRF |  0  | TSETUP | TPTNCY |  0  | DSETUP | DLTNCY |
  +------+------+-----+--------+--------+-----+--------+--------+
   15                                4    3     2     1      0
  +------------------------------------+------+---+-------+-----+
  |              RESERVED              | L2EN | 0 | ECCEN | RFE |
  +------------------------------------+------+---+-------+-----+
*********************************************************************************************************/

#define M_L2CACHE_CFG_TPRF      (0x1 << S_L2CACHE_CFG_TPRF)             /*  L2 CACHE TLB 预取使能位     */
#define S_L2CACHE_CFG_TPRF      31
#define M_L2CACHE_CFG_IPRF      (0x3 << S_L2CACHE_CFG_IPRF)             /*  L2 CACHE 指令预取设置位     */
#define S_L2CACHE_CFG_IPRF      29
#define M_L2CACHE_CFG_TSETUP    (0x1 << S_L2CACHE_CFG_TSETUP)           /*  TAG RAM setup 配置位        */
#define S_L2CACHE_CFG_TSETUP    25
#define M_L2CACHE_CFG_TPTNCY    (0x7 << S_L2CACHE_CFG_TPTNCY)           /*  TAG RAM 访问周期配置位      */
#define S_L2CACHE_CFG_TPTNCY    22
#define M_L2CACHE_CFG_DSETUP    (0x1 << S_L2CACHE_CFG_DSETUP)           /*  DATA RAM setup 配置位       */
#define S_L2CACHE_CFG_DSETUP    19
#define M_L2CACHE_CFG_DLTNCY    (0x7 << S_L2CACHE_CFG_DLTNCY)           /*  DATA RAM 访问周期配置位     */
#define S_L2CACHE_CFG_DLTNCY    16
#define M_L2CACHE_CFG_L2EN      (0x1 << S_L2CACHE_CFG_L2EN)             /*  L2 CACHE 使能位             */
#define S_L2CACHE_CFG_L2EN      3
#define M_L2CACHE_CFG_ECCEN     (0x1 << S_L2CACHE_CFG_ECCEN)            /*  L2 CACHE ECC 使能位         */
#define S_L2CACHE_CFG_ECCEN     1
#define M_L2CACHE_CFG_RFE       (0x1 << S_L2CACHE_CFG_RFE)              /*  数据访问读分配使能位        */
#define S_L2CACHE_CFG_RFE       0

/*********************************************************************************************************
  C-SKY 异常向量表
*********************************************************************************************************/

#define VEC_RESET       0
#define VEC_ALIGN       1
#define VEC_ACCESS      2
#define VEC_ZERODIV     3
#define VEC_ILLEGAL     4
#define VEC_PRIV        5
#define VEC_TRACE       6
#define VEC_BREAKPOINT  7
#define VEC_UNRECOVER   8
#define VEC_SOFTRESET   9
#define VEC_AUTOVEC     10
#define VEC_FAUTOVEC    11
#define VEC_HWACCEL     12

#define VEC_TLBFATAL    13
#define VEC_TLBMISS     14
#define VEC_TLBMODIFIED 15

#define VEC_SYS         16
#define VEC_TRAP0       16
#define VEC_TRAP1       17
#define VEC_TRAP2       18
#define VEC_TRAP3       19

#define VEC_TLBINVALIDL 20
#define VEC_TLBINVALIDS 21

#define VEC_PRFL        29
#define VEC_FPE         30

#define VEC_USER        32

#define VEC_INT1        33
#define VEC_INT2        34
#define VEC_INT3        35
#define VEC_INT4        36
#define VEC_INT5        37
#define VEC_INT6        38
#define VEC_INT7        39
#define VEC_INT8        40

#define VEC_MAX         255

#endif                                                                  /*  __SYLIXOS_KERNEL            */
                                                                        /*  __ASSEMBLY__                */
#endif                                                                  /*  __CSKY_ARCH_DEF_H           */
/*********************************************************************************************************
  END
*********************************************************************************************************/
