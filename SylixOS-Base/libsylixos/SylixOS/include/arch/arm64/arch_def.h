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
** 文件创建日期: 2018 年 06 月 22 日
**
** 描        述: ARM64 相关定义.
*********************************************************************************************************/

#ifndef __ARM64_ARCH_DEF_H
#define __ARM64_ARCH_DEF_H

/*********************************************************************************************************
  __CONST64
*********************************************************************************************************/

#if defined(__ASSEMBLY__) || defined(ASSEMBLY)
#define __CONST64(x)                x
#else
#ifdef __SYLIXOS_KERNEL
#if LW_CFG_CPU_WORD_LENGHT == 32
#define __CONST64(x)                x##ull
#else
#define __CONST64(x)                x##ul
#endif                                                                  /*  LW_CFG_CPU_WORD_LENGHT      */
#endif                                                                  /*  __SYLIXOS_KERNEL            */
#endif                                                                  /*  __ASSEMBLY__                */

/*********************************************************************************************************
  ARM64 指令
*********************************************************************************************************/

#if !defined(__ASSEMBLY__) && !defined(ASSEMBLY)
typedef UINT32                      ARM64_INSTRUCTION;
#endif                                                                  /*  !defined(__ASSEMBLY__)      */

/*********************************************************************************************************
  PREFETCH
*********************************************************************************************************/
#if defined(__SYLIXOS_KERNEL) || defined(__ASSEMBLY__) || defined(ASSEMBLY)

#define ARM_PREFETCH(ptr)           __asm__ __volatile__("prfm pldl1keep, %a0\n" : : "p" (ptr))

#if (LW_CFG_SMP_EN > 0) && (LW_CFG_ARM64_PREFETCH_W > 0)
#define ARM_PREFETCH_W(ptr)         __asm__ __volatile__("prfm pstl1keep, %a0\n" : : "p" (ptr))
#else
#define ARM_PREFETCH_W(ptr)         ARM_PREFETCH(ptr)
#endif

/*********************************************************************************************************
  WFI WFE SEV
*********************************************************************************************************/

#define ARM_SEV()                   __asm__ __volatile__ ("sev" : : : "memory")
#define ARM_WFE()                   __asm__ __volatile__ ("wfe" : : : "memory")
#define ARM_WFI()                   __asm__ __volatile__ ("wfi" : : : "memory")

/*********************************************************************************************************
  SPSR_EL2/SPSR_EL1
     31    30    29    28            21   20             9     8     7     6     5      4     3     0 
  +-----+-----+-----+-----+--------+----+----+--------+-----+-----+-----+-----+------+------+--------+
  |  N  |  Z  |  C  |  V  |  RES0  | SS | IL |  RES0  |  D  |  A  |  I  |  F  | RES0 | M[4] | M[3:0] |
  +-----+-----+-----+-----+--------+----+----+--------+-----+-----+-----+-----+------+------+--------+
             NZCV                                             DAIF                   
*********************************************************************************************************/

#define SPSR_N_BIT                  (0x1 << 31)
#define SPSR_Z_BIT                  (0x1 << 30)
#define SPSR_C_BIT                  (0x1 << 29)
#define SPSR_V_BIT                  (0x1 << 28)
#define SPSR_SS_BIT                 (0x1 << 21)
#define SPSR_IL_BIT                 (0x1 << 20)
#define SPSR_D_BIT                  (0x1 <<  9)
#define SPSR_A_BIT                  (0x1 <<  8)
#define SPSR_I_BIT                  (0x1 <<  7)
#define SPSR_F_BIT                  (0x1 <<  6)
#define SPSR_MODE64_BIT             (0x0 <<  4)
#define SPSR_MODE32_BIT             (0x1 <<  4)
#define SPSR_MODE_EL0t              (0x0 <<  0)
#define SPSR_MODE_EL1t              (0x4 <<  0)
#define SPSR_MODE_EL1h              (0x5 <<  0)
#define SPSR_MODE_EL2t              (0x8 <<  0)
#define SPSR_MODE_EL2h              (0x9 <<  0)
#define SPSR_MODE_EL3t              (0xc <<  0)
#define SPSR_MODE_EL3h              (0xd <<  0)
#define SPSR_MODE_MASK              (0xf <<  0)

/*********************************************************************************************************
  SCTLR_EL1
*********************************************************************************************************/

#define SCTLR_EL1_RES1              (3 << 28 | 3 << 22 |\
                                     1 << 20 | 1 << 11)                 /* Reserved, RES1               */
#define SCTLR_EL1_UCI_DIS           (0 << 26)                           /* Cache instruction disabled   */
#define SCTLR_EL1_EE_LE             (0 << 25)                           /* Exception Little-endian      */
#define SCTLR_EL1_WXN_DIS           (0 << 19)                           /* Write permission is not XN   */
#define SCTLR_EL1_NTWE_DIS          (0 << 18)                           /* WFE instruction disabled     */
#define SCTLR_EL1_NTWI_DIS          (0 << 16)                           /* WFI instruction disabled     */
#define SCTLR_EL1_UCT_DIS           (0 << 15)                           /* CTR_EL0 access disabled      */
#define SCTLR_EL1_DZE_DIS           (0 << 14)                           /* DC ZVA instruction disabled  */
#define SCTLR_EL1_ICACHE_DIS        (0 << 12)                           /* Instruction cache disabled   */
#define SCTLR_EL1_UMA_DIS           (0 <<  9)                           /* User Mask Access disabled    */
#define SCTLR_EL1_SED_EN            (0 <<  8)                           /* SETEND instruction enabled   */
#define SCTLR_EL1_ITD_EN            (0 <<  7)                           /* IT instruction enabled       */
#define SCTLR_EL1_CP15BEN_DIS       (0 <<  5)                           /* CP15 barrier operation dis.. */
#define SCTLR_EL1_SA0_DIS           (0 <<  4)                           /* Stack Alignment EL0 disabled */
#define SCTLR_EL1_SA_DIS            (0 <<  3)                           /* Stack Alignment EL1 disabled */
#define SCTLR_EL1_DCACHE_DIS        (0 <<  2)                           /* Data cache disabled          */
#define SCTLR_EL1_ALIGN_DIS         (0 <<  1)                           /* Alignment check disabled     */
#define SCTLR_EL1_MMU_DIS           (0)                                 /* MMU disabled                 */

/*********************************************************************************************************
  CNTHCTL_EL2/CNTHCTL_EL1
*********************************************************************************************************/

#define CNTHCTL_EL2_EL1PCEN_EN      (1 << 1)                            /* Physical timer reg accessible*/
#define CNTHCTL_EL2_EL1PCTEN_EN     (1 << 0)                            /* Physical counter accessible  */

/*********************************************************************************************************
  CPTR_EL2
*********************************************************************************************************/

#define CPTR_EL2_RES1               (3 << 12 | 0x3ff)                   /* Reserved, RES1               */

/*********************************************************************************************************
  CPACR_EL1
*********************************************************************************************************/

#define CPACR_EL1_FPEN_EN           (3 << 20)                           /* SIMD and FP enabled          */
#define CPACR_EL1_FPEN_DIS          (2 << 20)                           /* SIMD and FP disabled         */


/*********************************************************************************************************
  TCR_EL1
*********************************************************************************************************/

#define TCR_HD                      (1ULL << 40)                        /* hardware Dirty flag update   */
#define TCR_HA                      (1ULL << 39)                        /* hardware Access flag update  */
#define TCR_IPS_32BIT_4GB           (0ULL << 32)                        /* Phy Addr 32bits 4GB          */
#define TCR_IPS_36BIT_64GB          (1ULL << 32)                        /* Phy Addr 36bits 64GB         */
#define TCR_IPS_40BIT_1TB           (2ULL << 32)                        /* Phy Addr 40bits 1TB          */
#define TCR_IPS_42BIT_4TB           (3ULL << 32)                        /* Phy Addr 42bits 4TB          */
#define TCR_IPS_44BIT_16TB          (4ULL << 32)                        /* Phy Addr 44bits 16TB         */
#define TCR_IPS_48BIT_256TB         (5ULL << 32)                        /* Phy Addr 48bits 256TB        */
#define TCR_TG1_16KB                (1ULL << 30)                        /* Granule size for TTBR1_EL1   */
#define TCR_TG1_4KB                 (2ULL << 30)                        /* Granule size for TTBR1_EL1   */
#define TCR_TG1_64KB                (3ULL << 30)                        /* Granule size for TTBR1_EL1   */
#define TCR_EPD1_GEN_FAULT          (1ULL << 23)                        /* TLB miss gens fault TTBR1    */
#define TCR_T1SZ_SHIFT              (16)                                /* size offset of memory region */
#define TCR_T1SZ_MASK               (0x1f << 16)                        /* address by TTBR1             */
#define TCR_TG0_4KB                 (0ULL << 14)                        /* Granule size for TTBR0_EL1   */
#define TCR_TG0_64KB                (1ULL << 14)                        /* Granule size for TTBR0_EL1   */
#define TCR_TG0_16KB                (2ULL << 14)                        /* Granule size for TTBR0_EL1   */
#define TCR_SH0_NON_SHAREABLE       (0ULL << 12)                        /* Shareability attr TTBR0      */
#define TCR_SH0_OUTER_SHAREABLE     (2ULL << 12)                        /* Shareability attr TTBR0      */
#define TCR_SH0_INNER_SHAREABLE     (3ULL << 12)                        /* Shareability attr TTBR0      */
#define TCR_ORGN0_NON_CACHE         (0ULL << 10)                        /* Outer cacheability Non-cache */
#define TCR_ORGN0_WB_RA_WA          (1ULL << 10)                        /* Outer cacheability Write-Back*/
                                                                        /* Read-Alloc Write-Alloc       */
#define TCR_ORGN0_WT_RA_NO_WA       (2ULL << 10)                        /* Outer cacheability Write-Th  */
                                                                        /* Read-Alloc No Write-Alloc    */
#define TCR_ORGN0_WB_RA_NO_WA       (3ULL << 10)                        /* Outer cacheability Write-Back*/
                                                                        /* Read-Alloc No Write-Alloc    */
#define TCR_IRGN0_NON_CACHE         (0ULL <<  8)                        /* Inner cacheability Non-cache */
#define TCR_IRGN0_WB_RA_WA          (1ULL <<  8)                        /* Inner cacheability Write-Back*/
                                                                        /* Read-Alloc Write-Alloc       */
#define TCR_IRGN0_WT_RA_NO_WA       (2ULL <<  8)                        /* Inner cacheability Write-Th  */
                                                                        /* Read-Alloc No Write-Alloc    */
#define TCR_IRGN0_WB_RA_NO_WA       (3ULL <<  8)                        /* Inner cacheability Write-Back*/
                                                                        /* Read-Alloc No Write-Alloc    */
#define TCR_T0SZ_SHIFT              (0)                                 /* size offset of memory region */
#define TCR_T0SZ_MASK               (0x1f)                              /* address by TTBR0             */

/*********************************************************************************************************
  MDSCR_EL1
*********************************************************************************************************/

#define MDSCR_EL1_MDE_EN            (1 << 15)                           /* Monitor debug events enable  */
#define MDSCR_EL1_KDE_EN            (1 << 13)                           /* Local (kernel) debug enable  */
#define MDSCR_EL1_SS_EN             (1 <<  0)                           /* Software step enabled        */

/*********************************************************************************************************
  HCR_EL2
*********************************************************************************************************/

#define HCR_EL2_RW_AARCH64          (1 << 31)                           /* EL1 is AArch64               */
#define HCR_EL2_RW_AARCH32          (0 << 31)                           /* Lower levels are AArch32     */
#define HCR_EL2_HCD_DIS             (1 << 29)                           /* Hypervisor Call disabled     */

/*********************************************************************************************************
  CurrentEL
*********************************************************************************************************/

#define CurrentEL_EL0               (0  << 2)
#define CurrentEL_EL1               (1  << 2)
#define CurrentEL_EL2               (2  << 2)
#define CurrentEL_EL3               (3  << 2)

/*********************************************************************************************************
  PSTATE
     31    30    29    28          21           9     8     7     6              3   2    1     0
  +-----+-----+-----+-----+------+----+------+-----+-----+-----+-----+---------+------+------+----+
  |  N  |  Z  |  C  |  V  | RES0 | SS | RES0 |  D  |  A  |  I  |  F  |  RES0   |  EL  | RES0 | SP |
  +-----+-----+-----+-----+------+----+------+-----+-----+----+------+---------+------+------+----+
             NZCV             Software Step             DAIF                   CurrentEL      SPSel
*********************************************************************************************************/

#define M_PSTATE_N                  (1 << S_PSTATE_N)
#define S_PSTATE_N                  31
#define M_PSTATE_Z                  (1 << S_PSTATE_Z)
#define S_PSTATE_Z                  30
#define M_PSTATE_C                  (1 << S_PSTATE_C)
#define S_PSTATE_C                  29
#define M_PSTATE_V                  (1 << S_PSTATE_V)
#define S_PSTATE_V                  28
#define M_PSTATE_SS                 (1 << S_PSTATE_SS)
#define S_PSTATE_SS                 21
#define M_PSTATE_D                  (1 << S_PSTATE_D)
#define S_PSTATE_D                  9
#define M_PSTATE_A                  (1 << S_PSTATE_A)
#define S_PSTATE_A                  8
#define M_PSTATE_I                  (1 << S_PSTATE_I)
#define S_PSTATE_I                  7
#define M_PSTATE_F                  (1 << S_PSTATE_F)
#define S_PSTATE_F                  6
#define M_PSTATE_EL                 (3 << S_PSTATE_EL)
#define S_PSTATE_EL                 2
#define M_PSTATE_SP                 (1 << S_PSTATE_SP)
#define S_PSTATE_SP                 0

#define M_PSTATE_NZCV               (M_PSTATE_N | M_PSTATE_Z | M_PSTATE_C | M_PSTATE_V)
#define M_PSTATE_DAIF               (M_PSTATE_D | M_PSTATE_A | M_PSTATE_I | M_PSTATE_F)

#define EL0                         0
#define EL1                         1
#define EL2                         2
#define EL3                         3

#define EL1_SYN_INVALID             0x1
#define EL1_IRQ_INVALID             0x2
#define EL1_FIQ_INVALID             0x3
#define EL1_ERR_INVALID             0x4
#define EL2_IRQ_AARCH64_INVALID     0x5
#define EL2_FIQ_AARCH64_INVALID     0x6
#define EL2_ERR_AARCH64_INVALID     0x7
#define EL2_SYN_AARCH32_INVALID     0x8
#define EL2_IRQ_AARCH32_INVALID     0x9
#define EL2_FIQ_AARCH32_INVALID     0xa
#define EL2_ERR_AARCH32_INVALID     0xb

/*********************************************************************************************************
  CPU TYPE
*********************************************************************************************************/

#define MIDR_IMPLEMENTOR_SHIFT      24
#define MIDR_IMPLEMENTOR_MASK       (0xff << MIDR_IMPLEMENTOR_SHIFT)
#define MIDR_VARIANT_SHIFT          20
#define MIDR_VARIANT_MASK           (0xf << MIDR_VARIANT_SHIFT)
#define MIDR_ARCHITECTURE_SHIFT     16
#define MIDR_ARCHITECTURE_MASK      (0xf << MIDR_ARCHITECTURE_SHIFT)
#define MIDR_PARTNUM_SHIFT          4
#define MIDR_PARTNUM_MASK           (0xfff << MIDR_PARTNUM_SHIFT)
#define MIDR_REVISION_MASK          0xf
#define MIDR_REVISION(midr)         ((midr) & MIDR_REVISION_MASK)

#define MIDR_CPU_MODEL_MASK         (MIDR_IMPLEMENTOR_MASK | MIDR_PARTNUM_MASK | \
                                     MIDR_ARCHITECTURE_MASK)

#define MIDR_CPU_VAR_REV(var, rev) \
    (((var) << MIDR_VARIANT_SHIFT) | (rev))

#define MIDR_CPU_MODEL(imp, partnum) \
    (((imp)    << MIDR_IMPLEMENTOR_SHIFT)  | \
    (0xf       << MIDR_ARCHITECTURE_SHIFT) | \
    ((partnum) << MIDR_PARTNUM_SHIFT))

#define ARM_CPU_IMP_ARM             0x41
#define ARM_CPU_PART_CORTEX_A55     0xD05
#define MIDR_CORTEX_A55             MIDR_CPU_MODEL(ARM_CPU_IMP_ARM, ARM_CPU_PART_CORTEX_A55)

/*********************************************************************************************************
  EL2 HVC 调用命令号
*********************************************************************************************************/

#define HVC_CMD_FPU                 17

/*********************************************************************************************************
  ARM64 硬件单步类型
*********************************************************************************************************/

#define ARM64_DBG_TRAP_STEP         1

#endif                                                                  /*  __SYLIXOS_KERNEL            */
                                                                        /*  __ASSEMBLY__                */
#endif                                                                  /*  __ARM64_ARCH_DEF_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
