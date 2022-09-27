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
** 文   件   名: loongarch_atomic.h
**
** 创   建   人: Qin.Fei (秦飞)
**
** 文件创建日期: 2022 年 03 月 25 日
**
** 描        述: LoongArch 体系构架 ATOMIC 接口.
*********************************************************************************************************/

#ifndef __ARCH_LOONGARCH_ATOMIC_H
#define __ARCH_LOONGARCH_ATOMIC_H

/*********************************************************************************************************
  ATOMIC
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC_EN > 0

#include "arch/assembler.h"

#define ATOMIC_OP_RETURN(op, I, asm_op, c_op)               \
static LW_INLINE INT  archAtomic##op (INT  i, atomic_t  *v) \
{                                                           \
    INT  iResult;                                           \
                                                            \
    __asm__ __volatile__(                                   \
        "am"#asm_op"_db.w" " %1, %2, %0     \n"             \
        : "+ZB" (v->counter), "=&r" (iResult)               \
        : "r" (I)                                           \
        : "memory");                                        \
                                                            \
    return  (iResult c_op I);                               \
}

ATOMIC_OP_RETURN(Add,  i, add, +)
ATOMIC_OP_RETURN(Sub, -i, add, +)
ATOMIC_OP_RETURN(And,  i, and, &)
ATOMIC_OP_RETURN(Or,   i, or,  |)
ATOMIC_OP_RETURN(Xor,  i, xor, ^)

static LW_INLINE INT  archAtomicNand (INT  i, atomic_t  *v)
{
    INT  iTemp;
    INT  iResult;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        KN_WEAK_LLSC_MB
        "1: ll.w    %1, %2          \n"
        "   and     %0, %1, %3      \n"
        "   nor     %0, %0, $zero   \n"
        "   sc.w    %0, %2          \n"
        "   beqz    %0, 1b          \n"
        : "=&r" (iResult), "=&r" (iTemp),
          "+ZB" (v->counter)
        : "Ir" (i));

    iResult = ~(iTemp & i);

    KN_SMP_MB_AFTER_ATOMIC();

    return  (iResult);
}

static LW_INLINE VOID  archAtomicSet (INT  i, atomic_t  *v)
{
    INT  iResult;

    __asm__ __volatile__(
        "amswap_db.w" " %1, %2, %0     \n"
        : "+ZB" (v->counter), "=&r" (iResult)
        : "r" (i)
        : "memory");
}

static LW_INLINE INT  archAtomicGet (atomic_t  *v)
{
    return  (LW_ACCESS_ONCE(INT, v->counter));
}

/*********************************************************************************************************
  atomic cas op
*********************************************************************************************************/

static LW_INLINE INT  archAtomicCas (atomic_t  *v, INT  iOld, INT  iNew)
{
    INT  iResult;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        KN_WEAK_LLSC_MB
        "1: ll.w    %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   or      $t0, %z4, $zero     \n"
        "   sc.w    $t0, %1             \n"
        "   beqz    $t0, 1b             \n"
        "2:                             \n"
        KN_WEAK_LLSC_MB
        : "=&r" (iResult), "=ZB"(v->counter)
        : "ZB"(v->counter), "Jr" (iOld), "Jr" (iNew)
        : "t0", "memory");

    KN_SMP_MB_AFTER_ATOMIC();

    return  (iResult);
}

static LW_INLINE addr_t  archAtomicAddrCas (volatile addr_t *p, addr_t  ulOld, addr_t  ulNew)
{
    addr_t  ulResult;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        KN_WEAK_LLSC_MB
        "1: ll.w    %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   or      $t0, %z4, $zero     \n"
        "   sc.w    $t0, %1             \n"
        "   beqz    $t0, 1b             \n"
        "   b       3f                  \n"
        "2:                             \n"
        KN_WEAK_LLSC_MB
        "3:                             \n"
        : "=&r" (ulResult), "=ZB"(p)
        : "ZB"(p), "Jr" (ulOld), "Jr" (ulNew)
        : "t0", "memory");

    KN_SMP_MB_AFTER_ATOMIC();

    return  (ulResult);
}

/*********************************************************************************************************
  ATOMIC64
*********************************************************************************************************/
#if LW_CFG_CPU_ATOMIC64_EN > 0

#define ATOMIC64_OP_RETURN(op, I, asm_op, c_op)                       \
static LW_INLINE INT64  archAtomic64##op (INT64  i, atomic64_t  *v)   \
{                                                                     \
    INT64  i64Result;                                                 \
                                                                      \
    __asm__ __volatile__(                                             \
        "am"#asm_op"_db.d " " %1, %2, %0        \n"                   \
        : "+ZB" (v->counter), "=&r" (i64Result)                       \
        : "r" (I)                                                     \
        : "memory");                                                  \
                                                                      \
    return  (i64Result c_op I);                                       \
}

ATOMIC64_OP_RETURN(Add,  i, add, +)
ATOMIC64_OP_RETURN(Sub, -i, add, +)
ATOMIC64_OP_RETURN(And,  i, and, &)
ATOMIC64_OP_RETURN(Or,   i, or,  |)
ATOMIC64_OP_RETURN(Xor,  i, xor, ^)

static LW_INLINE INT64  archAtomic64Nand (INT64  i, atomic64_t  *v)
{
    INT64  i64Temp;
    INT64  i64Result;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        KN_WEAK_LLSC_MB
        "1: ll.d     %1, %2        \n"
        "   and      %0, %1, %3    \n"
        "   nor      %0, %0, $zero \n"
        "   sc.d     %0, %2        \n"
        "   beqz     %0, 1b        \n"
        : "=&r" (i64Result), "=&r" (i64Temp),
          "+ZB" (v->counter)
        : "Ir" (i));

    i64Result = ~(i64Temp & i);

    KN_SMP_MB_AFTER_ATOMIC();

    return  (i64Result);
}

static LW_INLINE VOID  archAtomic64Set (INT64  i, atomic64_t  *v)
{
    INT64  i64Result;

    __asm__ __volatile__(
        "amswap_db.d" " %1, %2, %0     \n"
        : "+ZB" (v->counter), "=&r" (i64Result)
        : "r" (i)
        : "memory");
}

static LW_INLINE INT64  archAtomic64Get (atomic64_t  *v)
{
    return  (LW_ACCESS_ONCE(INT64, v->counter));
}

/*********************************************************************************************************
  atomic64 cas op
*********************************************************************************************************/

static LW_INLINE INT64  archAtomic64Cas (atomic64_t  *v, INT64  i64Old, INT64  i64New)
{
    INT64  i64Result;

    KN_SMP_MB_BEFORE_ATOMIC();

    __asm__ __volatile__(
        KN_WEAK_LLSC_MB
        "1: ll.d    %0, %2              \n"
        "   bne     %0, %z3, 2f         \n"
        "   or      $t0, %z4, $zero     \n"
        "   sc.d    $t0, %1             \n"
        "   beqz    $t0, 1b             \n"
        "2:                             \n"
        KN_WEAK_LLSC_MB
        : "=&r" (i64Result), "=ZB"(v->counter)
        : "ZB"(v->counter), "Jr" (i64Old), "Jr" (i64New)
        : "t0", "memory");

    KN_SMP_MB_AFTER_ATOMIC();

    return  (i64Result);
}

#endif                                                                  /*  LW_CFG_CPU_ATOMIC64_EN      */
#endif                                                                  /*  LW_CFG_CPU_ATOMIC_EN        */
#endif                                                                  /*  __ARCH_LOONGARCH_ATOMIC_H   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
