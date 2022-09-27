/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE FAST COND IN USER PROCESS
 * this file is support pthread_cond fast mode.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 */

#define  __SYLIXOS_KERNEL
#include "fastlock.h"

/* Get atomic lock */
#define __GET_SENDER(pcond) ((atomic_t *)&((pcond)->TCD_ulSignal))
#define __GET_WAITER(pcond) ((atomic_t *)&((pcond)->TCD_ulCounter))

/* Calculate distance */
#define __CALC_DISTANCE(s, w) ((w) >= (s) ? (w) - (s) : \
                               (__ARCH_INT_MAX - (s)) + ((w) - __ARCH_INT_MIN) + 1)

/*
 * cond_init
 */
int pthread_cond_init (pthread_cond_t  *pcond, const pthread_condattr_t  *pcondattr)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);

    sys_atomic_set(psender, 0);
    sys_atomic_set(pwaiter, 0);

    return  (ERROR_NONE);
}

/*
 * cond_destroy
 */
int pthread_cond_destroy (pthread_cond_t  *pcond)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);

    sys_atomic_set(psender, 0);
    sys_atomic_set(pwaiter, 0);

    return  (ERROR_NONE);
}

/*
 * cond_signal
 */
int pthread_cond_signal (pthread_cond_t  *pcond)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);
    int o, s = sys_atomic_get(psender), w = sys_atomic_get(pwaiter);
    int cnt = __CALC_DISTANCE(s, w);

    while (cnt > 0) {
        o = sys_atomic_cas(psender, s, s + 1);
        if (s != o) {
            s = o;
            cnt = __CALC_DISTANCE(s, w);

        } else {
            if (API_VutexPostEx(sys_atomic_vutex(psender), s + 1,
                                LW_OPTION_VUTEX_FLAG_DONTSET |
                                LW_OPTION_VUTEX_FLAG_DEEPWAKE) > 0) {
                break;
            } else {
                s++;
                cnt = __CALC_DISTANCE(s, w);
            }
        }
    }

    return  (ERROR_NONE);
}

/*
 * cond_broadcast
 */
int pthread_cond_broadcast (pthread_cond_t  *pcond)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);
    int w = sys_atomic_get(pwaiter);

    sys_atomic_set(psender, w);

    API_VutexPostEx(sys_atomic_vutex(psender), w,
                    LW_OPTION_VUTEX_FLAG_WAKEALL | LW_OPTION_VUTEX_FLAG_DONTSET);

    return  (ERROR_NONE);
}

/*
 * cond_wait
 */
int pthread_cond_wait (pthread_cond_t  *pcond, pthread_mutex_t  *pmutex)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);
    int c = sys_atomic_inc(pwaiter);

    pthread_mutex_unlock(pmutex);

    API_VutexPend(sys_atomic_vutex(psender), c, LW_OPTION_WAIT_INFINITE);

    pthread_mutex_lock(pmutex);

    return  (ERROR_NONE);
}

/*
 * cond_timedwait
 */
int pthread_cond_timedwait (pthread_cond_t         *pcond,
                            pthread_mutex_t        *pmutex,
                            const struct timespec  *abs_timeout)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);
    int c, ret, error;
    unsigned long timeout;

    if ((abs_timeout == LW_NULL) ||
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    timeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);

    c = sys_atomic_inc(pwaiter);

    pthread_mutex_unlock(pmutex);

    ret = API_VutexPend(sys_atomic_vutex(psender), c, timeout);

    pthread_mutex_lock(pmutex);

    if (ret < 0) {
        error = errno;
        if (error == ERROR_THREAD_WAIT_TIMEOUT) {
            error = ETIMEDOUT;
            errno = ETIMEDOUT;
        }
        return  (error);
    } else {
        return  (ERROR_NONE);
    }
}

/*
 * cond_reltimedwait_np
 */
int pthread_cond_reltimedwait_np (pthread_cond_t         *pcond,
                                  pthread_mutex_t        *pmutex,
                                  const struct timespec  *rel_timeout)
{
    atomic_t *psender = __GET_SENDER(pcond);
    atomic_t *pwaiter = __GET_WAITER(pcond);
    int c, ret, error;
    unsigned long timeout;

    if ((rel_timeout == LW_NULL) ||
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    timeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);

    c = sys_atomic_inc(pwaiter);

    pthread_mutex_unlock(pmutex);

    ret = API_VutexPend(sys_atomic_vutex(psender), c, timeout);

    pthread_mutex_lock(pmutex);

    if (ret < 0) {
        error = errno;
        if (error == ERROR_THREAD_WAIT_TIMEOUT) {
            error = ETIMEDOUT;
            errno = ETIMEDOUT;
        }
        return  (error);
    } else {
        return  (ERROR_NONE);
    }
}

/*
 * cond_getinfo
 */
int pthread_cond_getinfo (pthread_cond_t  *pcond, pthread_cond_info_t  *info)
{
    errno = ENOSYS;
    return  (ENOSYS);
}

/*
 * cond_show
 */
int pthread_cond_show (pthread_cond_t  *pcond, int  level)
{
    errno = ENOSYS;
    return  (ENOSYS);
}
/*
 * end
 */
