/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE FAST SEMAPHORE IN USER PROCESS
 * this file is support semaphore fast mode.
 *
 * Author: Han.hui <hanhui@acoinfo.com>
 */

#define  __SYLIXOS_KERNEL
#include "fastlock.h"

/* SylixOS kernel symbol */
static int  (*os_sem_destroy)(sem_t  *psem);
static int  (*os_sem_close)(sem_t  *psem);
static int  (*os_sem_wait)(sem_t  *psem);
static int  (*os_sem_trywait)(sem_t  *psem);
static int  (*os_sem_timedwait)(sem_t  *psem, const struct timespec *abs_timeout);
static int  (*os_sem_reltimedwait_np)(sem_t  *psem, const struct timespec *rel_timeout);
static int  (*os_sem_post)(sem_t  *psem);
static int  (*os_sem_getvalue)(sem_t  *psem, int  *pivalue);
static int  (*os_sem_flush)(sem_t  *psem);
static int  (*os_sem_getinfo)(sem_t  *psem, sem_info_t  *info);
static int  (*os_sem_show)(sem_t  *psem, int  level);

/* Sem type */
static int sem_type_fastlock = 1;

/* Init invisible */
static pthread_mutex_t sem_init_lock = PTHREAD_MUTEX_INITIALIZER;

/* Get atomic lock */
#define __GET_ATOMIC(psem)  ((atomic_t *)&((psem)->SEM_presraw))

/* Get if is fast lock semaphore */
#define __IS_FASTLOCK_SEM(psem)  (psem->SEM_pvPxSem == &sem_type_fastlock)

/*
 * sem_symbol_init
 */
LW_CONSTRUCTOR_BEGIN
void sem_symbol_init (void)
{
    os_sem_destroy = (int (*)())symbolFind("sem_destroy", LW_SYMBOL_FLAG_XEN);
    os_sem_close = (int (*)())symbolFind("sem_close", LW_SYMBOL_FLAG_XEN);
    os_sem_wait = (int (*)())symbolFind("sem_wait", LW_SYMBOL_FLAG_XEN);
    os_sem_trywait = (int (*)())symbolFind("sem_trywait", LW_SYMBOL_FLAG_XEN);
    os_sem_timedwait = (int (*)())symbolFind("sem_timedwait", LW_SYMBOL_FLAG_XEN);
    os_sem_reltimedwait_np = (int (*)())symbolFind("sem_reltimedwait_np", LW_SYMBOL_FLAG_XEN);
    os_sem_post = (int (*)())symbolFind("sem_post", LW_SYMBOL_FLAG_XEN);
    os_sem_getvalue = (int (*)())symbolFind("sem_getvalue", LW_SYMBOL_FLAG_XEN);
    os_sem_flush = (int (*)())symbolFind("sem_flush", LW_SYMBOL_FLAG_XEN);
    os_sem_getinfo = (int (*)())symbolFind("sem_getinfo", LW_SYMBOL_FLAG_XEN);
    os_sem_show = (int (*)())symbolFind("sem_show", LW_SYMBOL_FLAG_XEN);
}
LW_CONSTRUCTOR_END(sem_symbol_init)

/*
 * __sem_init_invisible
 */
static void __sem_init_invisible (sem_t  *psem)
{
    if (unlikely(psem && psem->SEM_pvPxSem == LW_NULL)) {
        pthread_mutex_lock(&sem_init_lock);
        if (psem->SEM_pvPxSem == LW_NULL) {
            sem_init(psem, 0, 0);
        }
        pthread_mutex_unlock(&sem_init_lock);
    }
}

/*
 * sem_init
 */
int sem_init (sem_t  *psem, int  pshared, unsigned int  value)
{
    atomic_t *patomic;

    if (psem == LW_NULL || value > SEM_VALUE_MAX) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    psem->SEM_pvPxSem = &sem_type_fastlock;
    patomic = __GET_ATOMIC(psem);
    sys_atomic_set(patomic, value);

    return  (ERROR_NONE);
}

/*
 * sem_destroy
 */
int sem_destroy (sem_t  *psem)
{
    atomic_t *patomic;

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_destroy ? os_sem_destroy(psem) : PX_ERROR;
    }

    patomic = __GET_ATOMIC(psem);
    sys_atomic_set(patomic, 0);

    return  (ERROR_NONE);
}

/*
 * sem_close
 */
int sem_close (sem_t  *psem)
{
    atomic_t *patomic;

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_close ? os_sem_close(psem) : PX_ERROR;
    }

    patomic = __GET_ATOMIC(psem);
    sys_atomic_set(patomic, 0);

    return  (ERROR_NONE);
}

/*
 * sem_wait
 */
int sem_wait (sem_t  *psem)
{
    atomic_t *patomic;
    int cur, value, ret;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_wait ? os_sem_wait(psem) : PX_ERROR;
    }

    patomic = __GET_ATOMIC(psem);
    cur     = sys_atomic_get(patomic);
    value   = cur < 1 ? 1 : cur;

    while ((cur = sys_atomic_cas(patomic, value, value - 1)) != value) {
        if (cur) {
            value = cur;
        } else {
            ret = API_VutexPendEx(sys_atomic_vutex(patomic),
                                  LW_OPTION_VUTEX_GREATER_EQU, 1, LW_OPTION_WAIT_INFINITE);
            if (ret > 0) {
                break;
            } else if (ret == 0) {
                cur   = sys_atomic_get(patomic);
                value = cur < 1 ? 1 : cur;
            } else {
                if (errno == ERROR_THREAD_WAIT_TIMEOUT) {
                    break;
                } else {
                    return  (PX_ERROR);
                }
            }
        }
    }

    return  (ERROR_NONE);
}

/*
 * sem_trywait
 */
int sem_trywait (sem_t  *psem)
{
    atomic_t *patomic;
    int cur, value;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_trywait ? os_sem_trywait(psem) : PX_ERROR;
    }

    patomic = __GET_ATOMIC(psem);
    cur     = sys_atomic_get(patomic);
    value   = cur < 1 ? 1 : cur;

    while ((cur = sys_atomic_cas(patomic, value, value - 1)) != value) {
        if (cur) {
            value = cur;
        } else {
            errno = EAGAIN;
            return  (PX_ERROR);
        }
    }

    return  (ERROR_NONE);
}

/*
 * sem_timedwait
 */
int sem_timedwait (sem_t  *psem, const struct timespec *abs_timeout)
{
    atomic_t *patomic;
    int cur, value, ret;
    unsigned long timeout, timesave;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_timedwait ? os_sem_timedwait(psem, abs_timeout) : PX_ERROR;
    }

    if ((abs_timeout == LW_NULL) ||
        LW_NSEC_INVALD(abs_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    timeout = LW_TS_TIMEOUT_TICK(LW_FALSE, abs_timeout);
    patomic = __GET_ATOMIC(psem);
    cur     = sys_atomic_get(patomic);
    value   = cur < 1 ? 1 : cur;
    ret     = 0;

    while ((cur = sys_atomic_cas(patomic, value, value - 1)) != value) {
        if (cur) {
            value = cur;
        } else {
            timesave = sys_time_get();
            ret = API_VutexPendEx(sys_atomic_vutex(patomic),
                                  LW_OPTION_VUTEX_GREATER_EQU, 1, timeout);
            if (ret > 0) {
                break;
            } else if (ret == 0) {
                timeout = sys_time_recal(timesave, timeout);
                cur     = sys_atomic_get(patomic);
                value   = cur < 1 ? 1 : cur;
            } else {
                if (errno == ERROR_THREAD_WAIT_TIMEOUT) {
                    errno = ETIMEDOUT;
                    break;
                } else {
                    return  (PX_ERROR);
                }
            }
        }
    }

    return  (ret);
}

/*
 * reltimedwait_np
 */
int sem_reltimedwait_np (sem_t  *psem, const struct timespec *rel_timeout)
{
    atomic_t *patomic;
    int cur, value, ret;
    unsigned long timeout, timesave;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_reltimedwait_np ? os_sem_reltimedwait_np(psem, rel_timeout) : PX_ERROR;
    }

    if ((rel_timeout == LW_NULL) ||
        LW_NSEC_INVALD(rel_timeout->tv_nsec)) {
        errno = EINVAL;
        return  (EINVAL);
    }

    timeout = LW_TS_TIMEOUT_TICK(LW_TRUE, rel_timeout);
    patomic = __GET_ATOMIC(psem);
    cur     = sys_atomic_get(patomic);
    value   = cur < 1 ? 1 : cur;
    ret     = 0;

    while ((cur = sys_atomic_cas(patomic, value, value - 1)) != value) {
        if (cur) {
            value = cur;
        } else {
            timesave = sys_time_get();
            ret = API_VutexPendEx(sys_atomic_vutex(patomic),
                                  LW_OPTION_VUTEX_GREATER_EQU, 1, timeout);
            if (ret > 0) {
                break;
            } else if (ret == 0) {
                timeout = sys_time_recal(timesave, timeout);
                cur     = sys_atomic_get(patomic);
                value   = cur < 1 ? 1 : cur;
            } else {
                if (errno == ERROR_THREAD_WAIT_TIMEOUT) {
                    errno = ETIMEDOUT;
                    break;
                } else {
                    return  (PX_ERROR);
                }
            }
        }
    }

    return  (ret);
}

/*
 * sem_post
 */
int sem_post (sem_t  *psem)
{
    atomic_t *patomic;
    int cur;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_post ? os_sem_post(psem) : PX_ERROR;
    }

    patomic = __GET_ATOMIC(psem);

    cur = sys_atomic_inc(patomic);
    if (cur < 0) {
        sys_atomic_set(patomic, SEM_VALUE_MAX);
    } else if (cur == 1) {
        API_VutexPostEx(sys_atomic_vutex(patomic), SEM_VALUE_MAX, LW_OPTION_VUTEX_FLAG_DONTSET);
    }

    return  (ERROR_NONE);
}

/*
 * sem_getvalue
 */
int sem_getvalue (sem_t  *psem, int  *pivalue)
{
    atomic_t *patomic;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_getvalue ? os_sem_getvalue(psem, pivalue) : PX_ERROR;
    }

    patomic  = __GET_ATOMIC(psem);
    *pivalue = sys_atomic_get(patomic);

    return  (ERROR_NONE);
}

/*
 * sem_getvalue
 */
int  sem_flush (sem_t  *psem)
{
    atomic_t *patomic;

    __sem_init_invisible(psem);

    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_flush ? os_sem_flush(psem) : PX_ERROR;
    }

    patomic = __GET_ATOMIC(psem);

    API_VutexPostEx(sys_atomic_vutex(patomic), SEM_VALUE_MAX,
                    LW_OPTION_VUTEX_FLAG_WAKEALL | LW_OPTION_VUTEX_FLAG_DONTSET);

    return  (ERROR_NONE);
}

/*
 * sem_getinfo
 */
int sem_getinfo (sem_t  *psem, sem_info_t  *info)
{
    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_getinfo ? os_sem_getinfo(psem, info) : PX_ERROR;
    }

    errno = ENOSYS;
    return  (PX_ERROR);
}

/*
 * sem_show
 */
int sem_show (sem_t  *psem, int  level)
{
    if (!__IS_FASTLOCK_SEM(psem)) {
        return  os_sem_show ? os_sem_show(psem, level) : PX_ERROR;
    }

    errno = ENOSYS;
    return  (PX_ERROR);
}
/*
 * end
 */
