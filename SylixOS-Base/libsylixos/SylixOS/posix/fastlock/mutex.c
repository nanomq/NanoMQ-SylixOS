/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE FAST LOCK IN USER PROCESS
 * this file is support pthread_mutex fast mode.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 */

#define  __SYLIXOS_KERNEL
#include "fastlock.h"

/* Owner */
#define __GET_OWN(pmutex)  \
        (((pmutex)->PMUTEX_ulMutex & 0xffff0000) >> 16)
#define __SET_OWN(pmutex, own)  \
        { ((pmutex)->PMUTEX_ulMutex |= (((unsigned long)own) << 16)); \
          KN_SMP_WMB(); }
#define __CLR_OWN(pmutex)  \
        { (pmutex)->PMUTEX_ulMutex = 0ul; \
          KN_SMP_WMB(); }

/* Recursion */
#define __GET_RECURSION(pmutex) ((pmutex)->PMUTEX_ulMutex & 0xffff)
#define __INC_RECURSION(pmutex) ((pmutex)->PMUTEX_ulMutex++)
#define __DEC_RECURSION(pmutex) ((pmutex)->PMUTEX_ulMutex--)

/* Get atomic lock */
#define __GET_LOCK(pmutex)  ((atomic_t *)&((pmutex)->PMUTEX_iType))

/* Lock state */
#define __STATE_STEP             0x00010000
#define __STATE_UNLOCK           0x00000002
#define __STATE_LOCK_NO_WAITER   (__STATE_UNLOCK + __STATE_STEP)
#define __STATE_LOCK_HAS_WAITER  (__STATE_LOCK_NO_WAITER + __STATE_STEP)

/*
 * mutex_init
 */
int pthread_mutex_init (pthread_mutex_t  *pmutex, const pthread_mutexattr_t *pmutexattr)
{
    atomic_t *plock = __GET_LOCK(pmutex);

    pmutex->PMUTEX_ulMutex = 0ul;
    sys_atomic_set(plock, __STATE_UNLOCK);

    return  (ERROR_NONE);
}

/*
 * mutex_destroy
 */
int pthread_mutex_destroy (pthread_mutex_t  *pmutex)
{
    atomic_t *plock = __GET_LOCK(pmutex);

    pmutex->PMUTEX_ulMutex = 0ul;
    sys_atomic_set(plock, __STATE_UNLOCK);

    return  (ERROR_NONE);
}

/*
 * mutex_lock
 */
int pthread_mutex_lock (pthread_mutex_t  *pmutex)
{
    atomic_t *plock = __GET_LOCK(pmutex);
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_LOCK_NO_WAITER);
        if (c == __STATE_UNLOCK) {
            __SET_OWN(pmutex, idx);
            break;

        } else if (__GET_OWN(pmutex) == idx) {
            __INC_RECURSION(pmutex);
            break;

        } else if ((c == __STATE_LOCK_HAS_WAITER) ||
                   (sys_atomic_cas(plock, __STATE_LOCK_NO_WAITER, __STATE_LOCK_HAS_WAITER) != __STATE_UNLOCK)) {
            API_VutexPendEx(sys_atomic_vutex(plock),
                            LW_OPTION_VUTEX_LESS_EQU,
                            __STATE_LOCK_NO_WAITER, LW_OPTION_WAIT_INFINITE);
        }
    } while (1);

    return  (ERROR_NONE);
}

/*
 * mutex_trylock
 */
int pthread_mutex_trylock (pthread_mutex_t  *pmutex)
{
    atomic_t *plock = __GET_LOCK(pmutex);
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;

    c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_LOCK_NO_WAITER);
    if (c == __STATE_UNLOCK) {
        __SET_OWN(pmutex, idx);
        return  (ERROR_NONE);

    } else if (__GET_OWN(pmutex) == idx) {
        __INC_RECURSION(pmutex);
        return  (ERROR_NONE);
    }

    errno = EBUSY;
    return  (EBUSY);
}

/*
 * mutex_timedlock
 */
int pthread_mutex_timedlock (pthread_mutex_t  *pmutex, const struct timespec *abs_timeout)
{
    atomic_t *plock = __GET_LOCK(pmutex);
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;
    unsigned long timeout, timesave;

    if ((abs_timeout == LW_NULL) ||
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    timeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_LOCK_NO_WAITER);
        if (c == __STATE_UNLOCK) {
            __SET_OWN(pmutex, idx);
            return  (ERROR_NONE);

        } else if (__GET_OWN(pmutex) == idx) {
            __INC_RECURSION(pmutex);
            return  (ERROR_NONE);

        } else if ((c == __STATE_LOCK_HAS_WAITER) ||
                   (sys_atomic_cas(plock, __STATE_LOCK_NO_WAITER, __STATE_LOCK_HAS_WAITER) != __STATE_UNLOCK)) {
            timesave = sys_time_get();
            API_VutexPendEx(sys_atomic_vutex(plock),
                            LW_OPTION_VUTEX_LESS_EQU,
                            __STATE_LOCK_NO_WAITER, timeout);
            timeout = sys_time_recal(timesave, timeout);
        }
    } while (timeout);

    errno = ETIMEDOUT;
    return  (ETIMEDOUT);
}

/*
 * mutex_reltimedlock_np
 */
int pthread_mutex_reltimedlock_np (pthread_mutex_t  *pmutex, const struct timespec *rel_timeout)
{
    atomic_t *plock = __GET_LOCK(pmutex);
    pthread_t me = pthread_self();
    int c, idx = me & 0xffff;
    unsigned long timeout, timesave;

    if ((rel_timeout == LW_NULL) ||
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    timeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);

    do {
        c = sys_atomic_cas(plock, __STATE_UNLOCK, __STATE_LOCK_NO_WAITER);
        if (c == __STATE_UNLOCK) {
            __SET_OWN(pmutex, idx);
            return  (ERROR_NONE);

        } else if (__GET_OWN(pmutex) == idx) {
            __INC_RECURSION(pmutex);
            return  (ERROR_NONE);

        } else if ((c == __STATE_LOCK_HAS_WAITER) ||
                   (sys_atomic_cas(plock, __STATE_LOCK_NO_WAITER, __STATE_LOCK_HAS_WAITER) != __STATE_UNLOCK)) {
            timesave = sys_time_get();
            API_VutexPendEx(sys_atomic_vutex(plock),
                            LW_OPTION_VUTEX_LESS_EQU,
                            __STATE_LOCK_NO_WAITER, timeout);
            timeout = sys_time_recal(timesave, timeout);
        }
    } while (timeout);

    errno = ETIMEDOUT;
    return  (ETIMEDOUT);
}

/*
 * mutex_unlock
 */
int pthread_mutex_unlock (pthread_mutex_t  *pmutex)
{
    atomic_t *plock = __GET_LOCK(pmutex);
    pthread_t me = pthread_self();
    int o, idx = me & 0xffff;

    if (__GET_OWN(pmutex) != idx) {
        errno = EPERM;
        return  (EPERM);
    }

    if (__GET_RECURSION(pmutex)) {
        __DEC_RECURSION(pmutex);
        return  (ERROR_NONE);

    } else {
        __CLR_OWN(pmutex);
        o = sys_atomic_swp(plock, __STATE_UNLOCK);
        if (o == __STATE_LOCK_HAS_WAITER) {
            API_VutexPostEx(sys_atomic_vutex(plock), __STATE_UNLOCK,
                            LW_OPTION_VUTEX_FLAG_DONTSET | LW_OPTION_VUTEX_FLAG_DEEPWAKE);
        }
        return  (ERROR_NONE);
    }
}

/*
 * mutex_getinfo
 */
int pthread_mutex_getinfo (pthread_mutex_t  *pmutex, pthread_mutex_info_t  *info)
{
    int idx;

    lib_bzero(info, sizeof(pthread_mutex_info_t));

    idx = __GET_OWN(pmutex);
    if (idx) {
        info->ownner = _MakeObjectId(_OBJECT_THREAD, LW_CFG_PROCESSOR_NUMBER, idx);
    } else {
        info->value = 1;
    }

    return  (ERROR_NONE);
}

/*
 * mutex_setprioceiling
 */
int pthread_mutex_setprioceiling (pthread_mutex_t  *pmutex, int  prioceiling)
{
    errno = ENOSYS;
    return  (ENOSYS);
}

/*
 * mutex_getprioceiling
 */
int  pthread_mutex_getprioceiling (pthread_mutex_t  *pmutex, int  *prioceiling)
{
    errno = ENOSYS;
    return  (ENOSYS);
}

/*
 * mutex_show
 */
int pthread_mutex_show (pthread_mutex_t  *pmutex, int  level)
{
    errno = ENOSYS;
    return  (ENOSYS);
}
/*
 * end
 */
