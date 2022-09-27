/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE FAST LOCK IN USER PROCESS
 * this file is support lock fast mode.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 */

#ifndef __FASTLOCK_H
#define __FASTLOCK_H

#include <pthread.h>
#include <semaphore.h>

/*
 * Basic atomic
 */
#define sys_atomic_vutex(patomic)           ((int *)&((patomic)->counter))
#define sys_atomic_add(patomic, val)        API_AtomicAdd(val, patomic)
#define sys_atomic_sub(patomic, val)        API_AtomicSub(val, patomic)
#define sys_atomic_inc(patomic)             API_AtomicInc(patomic)
#define sys_atomic_dec(patomic)             API_AtomicDec(patomic)
#define sys_atomic_and(patomic, val)        API_AtomicAnd(val, patomic)
#define sys_atomic_nand(patomic, val)       API_AtomicNand(val, patomic)
#define sys_atomic_or(patomic, val)         API_AtomicOr(val, patomic)
#define sys_atomic_set(patomic, val)        API_AtomicSet(val, patomic)
#define sys_atomic_get(patomic)             API_AtomicGet(patomic)
#define sys_atomic_cas(patomic, o, n)       API_AtomicCas(patomic, o, n)
#define sys_atomic_swp(patomic, n)          API_AtomicSwp(n, patomic)

/*
 * Get and time
 */
#define sys_time_get()              API_TimeGet()
#define sys_time_recal(prev, to)    _sigTimeoutRecalc(prev, to)

#endif /* __FASTLOCK_H */
/*
 * end
 */
