/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE FAST LOCK IN USER PROCESS
 * this file is support pthread_rwlock fast mode.
 *
 * Author: Wu.Pengcheng <wupengcheng@acoinfo.com>
 */
#define  __SYLIXOS_KERNEL
#include "fastlock.h"

/* Owner */
#define __GET_OWN(prwlock)  \
        (((prwlock)->PRWLOCK_ulRwLock & 0xffff0000) >> 16)
#define __SET_OWN(prwlock, own)  \
        { ((prwlock)->PRWLOCK_ulRwLock |= (((unsigned long)own) << 16)); \
          KN_SMP_WMB(); }
#define __CLR_OWN(prwlock)  \
        { (prwlock)->PRWLOCK_ulRwLock = 0ul; \
          KN_SMP_WMB(); }

/*
 * PRWLOCK_ulReserved[0] : atomic lock for rwlock
 * PRWLOCK_ulReserved[1] : critical lock
 * PRWLOCK_uiReserved[0] : write lock recursion counter
 * PRWLOCK_uiReserved[1] : operate counter
 * PRWLOCK_uiReserved[2] : read pend counter
 * PRWLOCK_uiReserved[3] : write pend counter
 */

/* Get atomic lock */
#define __GET_LOCK(prwlock, plock)        do { \
                                              if (prwlock == LW_NULL) { \
                                                  errno = EINVAL; return  (EINVAL); \
                                              } else { \
                                                  plock = ((atomic_t *)&((prwlock)->PRWLOCK_ulReserved[0])); \
                                              } \
                                          } while (0);

/* Recursion counter */
#define __GET_RECURSION(prwlock)          ((prwlock)->PRWLOCK_uiReserved[0] & 0xffff)
#define __INC_RECURSION(prwlock)          ((prwlock)->PRWLOCK_uiReserved[0]++)
#define __DEC_RECURSION(prwlock)          ((prwlock)->PRWLOCK_uiReserved[0]--)

/* rwlock operate counter */
#define __GET_RWLOCK_OP_CNT(prwlock)      ((prwlock)->PRWLOCK_uiReserved[1] & 0xffff)
#define __INC_RWLOCK_OP_CNT(prwlock)      ((prwlock)->PRWLOCK_uiReserved[1]++)
#define __DEC_RWLOCK_OP_CNT(prwlock)      ((prwlock)->PRWLOCK_uiReserved[1]--)

/* rdlock pend counter */
#define __GET_RWLOCK_RD_PEND_CNT(prwlock) ((prwlock)->PRWLOCK_uiReserved[2] & 0xffff)
#define __INC_RWLOCK_RD_PEND_CNT(prwlock) ((prwlock)->PRWLOCK_uiReserved[2]++)
#define __DEC_RWLOCK_RD_PEND_CNT(prwlock) ((prwlock)->PRWLOCK_uiReserved[2]--)

/* wrlock pend counter */
#define __GET_RWLOCK_WR_PEND_CNT(prwlock) ((prwlock)->PRWLOCK_uiReserved[3] & 0xffff)
#define __INC_RWLOCK_WR_PEND_CNT(prwlock) ((prwlock)->PRWLOCK_uiReserved[3]++)
#define __DEC_RWLOCK_WR_PEND_CNT(prwlock) ((prwlock)->PRWLOCK_uiReserved[3]--)

/* Lock state
 *
 * __STATE_UNLOCK:
 * there is no thread take lock.
 *
 * __STATE_RDLOCK_PREPARE:
 * a thread post a lock and the read threads can get this lock first.
 *
 * __STATE_RDLOCK:
 * there are only read threads take this lock and no write thread blocked in read lock.
 *
 * __STATE_RDLOCK_HAS_WAITER:
 * there are read threads take this lock and some other threads blocked in read lock.
 *
 * __STATE_WRLOCK_PREPARE:
 * a write thread can get this lock first.
 *
 * __STATE_WRLOCK:
 * there is a write thread take this lock.
 */
#define __STATE_STEP                    0x00010000
#define __STATE_UNLOCK                  0x00000000
#define __STATE_RDLOCK_PREPARE          (__STATE_UNLOCK + __STATE_STEP)
#define __STATE_RDLOCK                  (__STATE_RDLOCK_PREPARE + __STATE_STEP)
#define __STATE_RDLOCK_HAS_WAITER       (__STATE_RDLOCK + __STATE_STEP)
#define __STATE_WRLOCK_PREPARE          (__STATE_RDLOCK_HAS_WAITER + __STATE_STEP)
#define __STATE_WRLOCK                  (__STATE_WRLOCK_PREPARE + __STATE_STEP)

/* Get critical lock */
#define __GET_CRITICAL_LOCK(prwlock)        (atomic_t *)&((prwlock)->PRWLOCK_ulReserved[1])

/* Critical lock state */
#define __CRITICAL_STATE_STEP               0x00010000
#define __CRITICAL_STATE_UNLOCK             0x00000000
#define __CRITICAL_STATE_LOCK_NO_WAITER     (__CRITICAL_STATE_UNLOCK + __CRITICAL_STATE_STEP)
#define __CRITICAL_STATE_LOCK_HAS_WAITER    (__CRITICAL_STATE_LOCK_NO_WAITER + __CRITICAL_STATE_STEP)

#define __RWLOCK_CRITICAL_ENTER(prwlock)    __rwlock_critical_enter(prwlock)
#define __RWLOCK_CRITICAL_EXIT(prwlock)     __rwlock_critical_exit(prwlock)

/*
 * Enter critical
 */
static int __rwlock_critical_enter (pthread_rwlock_t  *prwlock)
{
    atomic_t *plock;
    int c;

    plock = __GET_CRITICAL_LOCK(prwlock);

    do {
        c = sys_atomic_cas(plock, __CRITICAL_STATE_UNLOCK, __CRITICAL_STATE_LOCK_NO_WAITER);
        if (c == __CRITICAL_STATE_UNLOCK) {
            break;

        } else if ((c == __CRITICAL_STATE_LOCK_HAS_WAITER) ||
                   (sys_atomic_cas(plock, __CRITICAL_STATE_LOCK_NO_WAITER, __CRITICAL_STATE_LOCK_HAS_WAITER)
                                   != __CRITICAL_STATE_UNLOCK)) {
            API_VutexPendEx(sys_atomic_vutex(plock),
                            LW_OPTION_VUTEX_LESS_EQU,
                            __CRITICAL_STATE_LOCK_NO_WAITER, LW_OPTION_WAIT_INFINITE);
        }
    } while (1);

    return  (ERROR_NONE);
}

/*
 * Exit critical
 */
static int __rwlock_critical_exit (pthread_rwlock_t  *prwlock)
{
    atomic_t *plock;
    int o;

    plock = __GET_CRITICAL_LOCK(prwlock);

    o = sys_atomic_swp(plock, __CRITICAL_STATE_UNLOCK);
    if (o == __CRITICAL_STATE_LOCK_HAS_WAITER) {
        API_VutexPostEx(sys_atomic_vutex(plock), __CRITICAL_STATE_UNLOCK,
                        LW_OPTION_VUTEX_FLAG_DONTSET | LW_OPTION_VUTEX_FLAG_DEEPWAKE);
    }

    return  (ERROR_NONE);
}

/*
 * rwlock_init
 */
int pthread_rwlock_init (pthread_rwlock_t  *prwlock, const pthread_rwlockattr_t  *prwlockattr)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    lib_bzero(prwlock, sizeof(pthread_rwlock_t));

    return  (ERROR_NONE);
}

/*
 * rwlock_destroy
 */
int pthread_rwlock_destroy (pthread_rwlock_t  *prwlock)
{
    if (prwlock == LW_NULL) {
        errno = EINVAL;
        return  (EINVAL);
    }

    lib_bzero(prwlock, sizeof(pthread_rwlock_t));

    return  (ERROR_NONE);
}

/*
 * rwlock_rdlock
 */
int  pthread_rwlock_rdlock (pthread_rwlock_t  *prwlock)
{
    int c;
    atomic_t *plock;

    __GET_LOCK(prwlock, plock);

    __RWLOCK_CRITICAL_ENTER(prwlock);

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_RDLOCK);
        if ((c == __STATE_UNLOCK) || (c == __STATE_RDLOCK)) {
            __INC_RWLOCK_OP_CNT(prwlock);
            break;

        } else if (c == __STATE_RDLOCK_PREPARE) {
            __INC_RWLOCK_OP_CNT(prwlock);
            if (!__GET_RWLOCK_RD_PEND_CNT(prwlock)) {
                /* if there isn't read thread blocked in lock,
                 * we should change status to '__STATE_RDLOCK_HAS_WAITER' from '__STATE_RDLOCK_PREPARE'.*/
                sys_atomic_set(plock, __STATE_RDLOCK_HAS_WAITER);
            }
            break;

        } else {
            __INC_RWLOCK_RD_PEND_CNT(prwlock);
            __RWLOCK_CRITICAL_EXIT(prwlock);

            API_VutexPendEx(sys_atomic_vutex(plock),
                            LW_OPTION_VUTEX_LESS_EQU,
                            __STATE_RDLOCK_PREPARE, LW_OPTION_WAIT_INFINITE);

            __RWLOCK_CRITICAL_ENTER(prwlock);
            __DEC_RWLOCK_RD_PEND_CNT(prwlock);
        }
    } while (1);

    __RWLOCK_CRITICAL_EXIT(prwlock);

    return  (ERROR_NONE);
}

/*
 * rwlock_tryrdlock
 */
int pthread_rwlock_tryrdlock (pthread_rwlock_t  *prwlock)
{
    int c;
    atomic_t *plock;

    __GET_LOCK(prwlock, plock);

    __RWLOCK_CRITICAL_ENTER(prwlock);

    c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_RDLOCK);
    if ((c == __STATE_UNLOCK) || (c == __STATE_RDLOCK)) {
        __INC_RWLOCK_OP_CNT(prwlock);
        __RWLOCK_CRITICAL_EXIT(prwlock);
        return  (ERROR_NONE);

    } else if (c == __STATE_RDLOCK_PREPARE) {
        __INC_RWLOCK_OP_CNT(prwlock);
        if (!__GET_RWLOCK_RD_PEND_CNT(prwlock)) {
           sys_atomic_set(plock, __STATE_RDLOCK_HAS_WAITER);
        }
        __RWLOCK_CRITICAL_EXIT(prwlock);
        return  (ERROR_NONE);
    }

    __RWLOCK_CRITICAL_EXIT(prwlock);

    errno = EBUSY;
    return  (EBUSY);
}

/*
 * rdlock_timeout implement
 */
static int __pthread_rwlock_rdlock_timeout (pthread_rwlock_t  *prwlock,
                                            unsigned long  timeout)
{
    int c;
    unsigned long timesave;
    atomic_t *plock;

    __GET_LOCK(prwlock, plock);

    __RWLOCK_CRITICAL_ENTER(prwlock);

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_RDLOCK);
        if ((c == __STATE_UNLOCK) || (c == __STATE_RDLOCK)) {
            __INC_RWLOCK_OP_CNT(prwlock);
            __RWLOCK_CRITICAL_EXIT(prwlock);
            return  (ERROR_NONE);

        } else if (c == __STATE_RDLOCK_PREPARE) {
            __INC_RWLOCK_OP_CNT(prwlock);
            if (!__GET_RWLOCK_RD_PEND_CNT(prwlock)) {
               sys_atomic_set(plock, __STATE_RDLOCK_HAS_WAITER);
            }
            __RWLOCK_CRITICAL_EXIT(prwlock);
            return  (ERROR_NONE);

        } else {
            __INC_RWLOCK_RD_PEND_CNT(prwlock);
            __RWLOCK_CRITICAL_EXIT(prwlock);

            timesave = sys_time_get();
            API_VutexPendEx(sys_atomic_vutex(plock),
                            LW_OPTION_VUTEX_LESS_EQU,
                            __STATE_RDLOCK_PREPARE, timeout);
            timeout = sys_time_recal(timesave, timeout);

            __RWLOCK_CRITICAL_ENTER(prwlock);
            __DEC_RWLOCK_RD_PEND_CNT(prwlock);
        }
    } while (timeout);

    __RWLOCK_CRITICAL_EXIT(prwlock);

    errno = ETIMEDOUT;
    return  (ETIMEDOUT);
}

/*
 * rwlock_timedlock
 */
int pthread_rwlock_timedrdlock (pthread_rwlock_t  *prwlock,
                                const struct timespec  *abs_timeout)
{
    if ((prwlock == LW_NULL) ||
        (abs_timeout == LW_NULL) ||
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (__pthread_rwlock_rdlock_timeout(prwlock,
                                             LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout)));
}

/*
 * rwlock_reltimedlock_np
 */
int pthread_rwlock_reltimedrdlock_np (pthread_rwlock_t  *prwlock,
                                      const struct timespec  *rel_timeout)
{
    if ((prwlock == LW_NULL) ||
        (rel_timeout == LW_NULL) ||
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (__pthread_rwlock_rdlock_timeout(prwlock,
                                             LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout)));
}

/*
 * rwlock_wrlock
 */
int pthread_rwlock_wrlock (pthread_rwlock_t  *prwlock)
{
    atomic_t *plock;
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;

    __GET_LOCK(prwlock, plock);

    __RWLOCK_CRITICAL_ENTER(prwlock);

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_WRLOCK);
        if (c == __STATE_UNLOCK) {
            __INC_RWLOCK_OP_CNT(prwlock);
            __SET_OWN(prwlock, me);
            break;

        } else if (__GET_OWN(prwlock) == idx) {
            __INC_RECURSION(prwlock);
            break;

        } else if (__STATE_WRLOCK_PREPARE ==
                   sys_atomic_cas(plock, __STATE_WRLOCK_PREPARE, __STATE_WRLOCK)) {
            __INC_RWLOCK_OP_CNT(prwlock);
            __SET_OWN(prwlock, me);
            break;
        }

        sys_atomic_cas(plock, __STATE_RDLOCK, __STATE_RDLOCK_HAS_WAITER);

        __INC_RWLOCK_WR_PEND_CNT(prwlock);
        __RWLOCK_CRITICAL_EXIT(prwlock);

        API_VutexPendEx(sys_atomic_vutex(plock),
                        LW_OPTION_VUTEX_EQU,
                        __STATE_WRLOCK_PREPARE, LW_OPTION_WAIT_INFINITE);

        __RWLOCK_CRITICAL_ENTER(prwlock);
        __DEC_RWLOCK_WR_PEND_CNT(prwlock);
    } while (1);

    __RWLOCK_CRITICAL_EXIT(prwlock);

    return  (ERROR_NONE);
}

/*
 * rwlock_trywrlock
 */
int pthread_rwlock_trywrlock (pthread_rwlock_t  *prwlock)
{
    atomic_t *plock;
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;

    __GET_LOCK(prwlock, plock);

    __RWLOCK_CRITICAL_ENTER(prwlock);

    c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_WRLOCK);
    if (c == __STATE_UNLOCK) {
        __INC_RWLOCK_OP_CNT(prwlock);
        __SET_OWN(prwlock, me);
        __RWLOCK_CRITICAL_EXIT(prwlock);
        return  (ERROR_NONE);

    } else if (__GET_OWN(prwlock) == idx) {
        __INC_RECURSION(prwlock);
        __RWLOCK_CRITICAL_EXIT(prwlock);
        return  (ERROR_NONE);

    } else if (__STATE_WRLOCK_PREPARE ==
               sys_atomic_cas(plock, __STATE_WRLOCK_PREPARE, __STATE_WRLOCK)) {
         __INC_RWLOCK_OP_CNT(prwlock);
         __SET_OWN(prwlock, me);
         __RWLOCK_CRITICAL_EXIT(prwlock);
         return  (ERROR_NONE);
    }

    __RWLOCK_CRITICAL_EXIT(prwlock);

    errno = EBUSY;
    return  (EBUSY);
}

/*
 * wrlock_timeout implement
 */
static int __pthread_rwlock_wrlock_timeout (pthread_rwlock_t  *prwlock,
                                            unsigned long  timeout)
{
    unsigned long timesave;
    atomic_t *plock;
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;

    __GET_LOCK(prwlock, plock);

    __RWLOCK_CRITICAL_ENTER(prwlock);

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_WRLOCK);
        if (c == __STATE_UNLOCK) {
            __INC_RWLOCK_OP_CNT(prwlock);
            __SET_OWN(prwlock, me);
            __RWLOCK_CRITICAL_EXIT(prwlock);
            return  (ERROR_NONE);

        } else if (__GET_OWN(prwlock) == idx) {
            __INC_RECURSION(prwlock);
            __RWLOCK_CRITICAL_EXIT(prwlock);
            return  (ERROR_NONE);

        } else if (__STATE_WRLOCK_PREPARE ==
                   sys_atomic_cas(plock, __STATE_WRLOCK_PREPARE, __STATE_WRLOCK)) {
            __INC_RWLOCK_OP_CNT(prwlock);
            __SET_OWN(prwlock, me);
            __RWLOCK_CRITICAL_EXIT(prwlock);
            return  (ERROR_NONE);

        } else if (c != __STATE_WRLOCK) {
            sys_atomic_set(plock, __STATE_RDLOCK_HAS_WAITER);
        }

        __INC_RWLOCK_WR_PEND_CNT(prwlock);
        __RWLOCK_CRITICAL_EXIT(prwlock);

        timesave = sys_time_get();
        API_VutexPendEx(sys_atomic_vutex(plock),
                        LW_OPTION_VUTEX_EQU,
                        __STATE_WRLOCK_PREPARE, timeout);
        timeout = sys_time_recal(timesave, timeout);

        __RWLOCK_CRITICAL_ENTER(prwlock);
        __DEC_RWLOCK_WR_PEND_CNT(prwlock);
    } while (timeout);

    __RWLOCK_CRITICAL_EXIT(prwlock);

    errno = ETIMEDOUT;
    return  (ETIMEDOUT);
}

/*
 * rwlock_timedwrlock
 */
int pthread_rwlock_timedwrlock (pthread_rwlock_t  *prwlock,
                                 const struct timespec  *abs_timeout)
{
    if ((prwlock == LW_NULL) ||
        (abs_timeout == LW_NULL) ||
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (__pthread_rwlock_wrlock_timeout(prwlock,
                                             LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout)));
}

/*
 * rwlock_reltimedwrlock
 */
int pthread_rwlock_reltimedwrlock_np (pthread_rwlock_t  *prwlock,
                                      const struct timespec  *rel_timeout)
{
    if ((prwlock == LW_NULL) ||
        (rel_timeout == LW_NULL) ||
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    return  (__pthread_rwlock_wrlock_timeout(prwlock,
                                             LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout)));
}

/*
 * rwlock_unlock
 */
int pthread_rwlock_unlock (pthread_rwlock_t  *prwlock)
{
    atomic_t *plock;
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;

    __GET_LOCK(prwlock, plock);

    if (__GET_RWLOCK_OP_CNT(prwlock) == 0) {
        errno = EPERM;
        return  (EPERM);
    }

    __RWLOCK_CRITICAL_ENTER(prwlock);

    c = sys_atomic_get(plock);
    switch (c) {
    case __STATE_RDLOCK:
        __DEC_RWLOCK_OP_CNT(prwlock);
        if (!__GET_RWLOCK_OP_CNT(prwlock)) {
            sys_atomic_set(plock, __STATE_UNLOCK);
        }
        break;

    case __STATE_WRLOCK:
        if (__GET_OWN(prwlock) != idx) {
            __RWLOCK_CRITICAL_EXIT(prwlock);
            errno = EPERM;
            return  (EPERM);
        }

        if (__GET_RECURSION(prwlock)) {
            __DEC_RECURSION(prwlock);

        } else {
            __CLR_OWN(prwlock);
            __DEC_RWLOCK_OP_CNT(prwlock);

            if (__GET_RWLOCK_WR_PEND_CNT(prwlock)) {
                /* a write thread can take this lock after post */
                API_VutexPostEx(sys_atomic_vutex(plock),
                                __STATE_WRLOCK_PREPARE,
                                LW_OPTION_VUTEX_FLAG_DEEPWAKE);
            } else {
                API_VutexPostEx(sys_atomic_vutex(plock),
                                __STATE_UNLOCK,
                                LW_OPTION_VUTEX_FLAG_DEEPWAKE);
            }
        }
        break;

    case __STATE_RDLOCK_HAS_WAITER:
        __DEC_RWLOCK_OP_CNT(prwlock);

        if (!__GET_RWLOCK_RD_PEND_CNT(prwlock)) {
            API_VutexPostEx(sys_atomic_vutex(plock),
                            __GET_RWLOCK_OP_CNT(prwlock) ? __STATE_RDLOCK_HAS_WAITER :
                                                             __STATE_WRLOCK_PREPARE,
                            LW_OPTION_VUTEX_FLAG_DEEPWAKE);
        } else {
            API_VutexPostEx(sys_atomic_vutex(plock), __STATE_RDLOCK_PREPARE, LW_OPTION_VUTEX_FLAG_DEEPWAKE);
        }
        break;

    case __STATE_RDLOCK_PREPARE:
        __DEC_RWLOCK_OP_CNT(prwlock);
        break;

    default:
        break;
    }

    __RWLOCK_CRITICAL_EXIT(prwlock);

    return  (ERROR_NONE);
}

/*
 * rwlock_getinfo
 */
int pthread_rwlock_getinfo (pthread_rwlock_t  *prwlock, pthread_rwlock_info_t  *info)
{
    int idx;

    if ((prwlock == LW_NULL) || info == LW_NULL) {
        return  (PX_ERROR);
    }

    lib_bzero(info, sizeof(pthread_rwlock_info_t));

    __RWLOCK_CRITICAL_ENTER(prwlock);

    idx = __GET_OWN(prwlock);
    if (idx) {
        info->owner = _MakeObjectId(_OBJECT_THREAD, LW_CFG_PROCESSOR_NUMBER, idx);
    }

    info->opcnt = __GET_RWLOCK_OP_CNT(prwlock);
    info->rpend = __GET_RWLOCK_RD_PEND_CNT(prwlock);
    info->wpend = __GET_RWLOCK_WR_PEND_CNT(prwlock);

    __RWLOCK_CRITICAL_EXIT(prwlock);

    return  (ERROR_NONE);
}
/*
 * end
 */
