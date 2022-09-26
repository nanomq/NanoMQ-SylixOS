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
** 文   件   名: tlb.h
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2022 年 03 月 28 日
**
** 描        述: LoongArch tlb 相关指令.
*********************************************************************************************************/

#ifndef __ASM_TLB_H
#define __ASM_TLB_H

/*
 * TLB Invalidate Flush
 */
static inline void tlbinv(void)
{
    __asm__ __volatile__("tlbclr");
}

static inline void tlbinvf(void)
{
    __asm__ __volatile__("tlbflush");
}

/*
 * TLB R/W operations.
 */
static inline void tlb_probe(void)
{
    __asm__ __volatile__("tlbsrch");
}

static inline void tlb_read(void)
{
    __asm__ __volatile__("tlbrd");
}

static inline void tlb_write_indexed(void)
{
    __asm__ __volatile__("tlbwr");
}

static inline void tlb_write_random(void)
{
    __asm__ __volatile__("tlbfill");
}

enum invtlb_ops {
    /* Invalid all tlb */
    INVTLB_ALL = 0x0,
    /* Invalid current tlb */
    INVTLB_CURRENT_ALL = 0x1,
    /* Invalid all global=1 lines in current tlb */
    INVTLB_CURRENT_GTRUE = 0x2,
    /* Invalid all global=0 lines in current tlb */
    INVTLB_CURRENT_GFALSE = 0x3,
    /* Invalid global=0 and matched asid lines in current tlb */
    INVTLB_GFALSE_AND_ASID = 0x4,
    /* Invalid addr with global=0 and matched asid in current tlb */
    INVTLB_ADDR_GFALSE_AND_ASID = 0x5,
    /* Invalid addr with global=1 or matched asid in current tlb */
    INVTLB_ADDR_GTRUE_OR_ASID = 0x6,
    /* Invalid matched gid in guest tlb */
    INVGTLB_GID = 0x9,
    /* Invalid global=1, matched gid in guest tlb */
    INVGTLB_GID_GTRUE = 0xa,
    /* Invalid global=0, matched gid in guest tlb */
    INVGTLB_GID_GFALSE = 0xb,
    /* Invalid global=0, matched gid and asid in guest tlb */
    INVGTLB_GID_GFALSE_ASID = 0xc,
    /* Invalid global=0 , matched gid, asid and addr in guest tlb */
    INVGTLB_GID_GFALSE_ASID_ADDR = 0xd,
    /* Invalid global=1 , matched gid, asid and addr in guest tlb */
    INVGTLB_GID_GTRUE_ASID_ADDR = 0xe,
    /* Invalid all gid gva-->gpa guest tlb */
    INVGTLB_ALLGID_GVA_TO_GPA = 0x10,
    /* Invalid all gid gpa-->hpa tlb */
    INVTLB_ALLGID_GPA_TO_HPA = 0x11,
    /* Invalid all gid tlb, including  gva-->gpa and gpa-->hpa */
    INVTLB_ALLGID = 0x12,
    /* Invalid matched gid gva-->gpa guest tlb */
    INVGTLB_GID_GVA_TO_GPA = 0x13,
    /* Invalid matched gid gpa-->hpa tlb */
    INVTLB_GID_GPA_TO_HPA = 0x14,
    /* Invalid matched gid tlb,including gva-->gpa and gpa-->hpa */
    INVTLB_GID_ALL = 0x15,
    /* Invalid matched gid and addr gpa-->hpa tlb */
    INVTLB_GID_ADDR = 0x16,
};

/*
 * invtlb op info addr
 * (0x1 << 26) | (0x24 << 20) | (0x13 << 15) |
 * (addr << 10) | (info << 5) | op
 */
#define invtlb_all(op, info, addr) \
{  \
    __asm__ __volatile__( \
        ".word ((0x6498000) | (0 << 10) | (0 << 5) | %0) \n"\
        : \
        : "i"(op) \
        : \
        ); \
}

#endif                                                                  /*  __ASM_TLB_H                 */
/*********************************************************************************************************
  END
*********************************************************************************************************/
