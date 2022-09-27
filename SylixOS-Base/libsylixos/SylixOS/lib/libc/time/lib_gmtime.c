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
** 文   件   名: lib_gmttime.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 30 日
**
** 描        述: 系统库.

BUG:
2009.02.07  使用自己的 lldiv 函数.
2010.07.10  修正 lib_gmtime_r 函数的返回值.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
** 函数名称: __daysSinceYear
** 功能描述: 一年中第几天 (days since Jan 1)
** 输　入  : year  年 (1900 以来)
**           month 月
**           mday  月中的日
** 输　出  : 一年中的第几天
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

INT  __daysSinceYear (INT  year, INT  month, INT  mday)
{
    static INT  days[13] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };

    INT  leap;

    if (month > 1 && isleap(year + TM_YEAR_BASE)) {
        leap = 1;
    } else {
        leap = 0;
    }

    return  (days[month] + mday + leap);
}
/*********************************************************************************************************
** 函数名称: __daysSinceEpoch
** 功能描述: 计算新纪元以来的天数
** 输　入  : year  新纪元以来的年数
**           yday  一年中第几天
** 输　出  : 新纪元以来的天数
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  __daysSinceEpoch (INT  year, INT  yday)
{
    INT  adjust;

    if (year >= 0) {                                                    /*  1970 +                      */
        adjust = ((year + 69) / 100 - (year + 369) / 400);
        return ((DAYSPERYEAR * year) + (year + 1) / 4 - adjust + yday);

    } else {                                                            /*  1969 -                      */
        adjust = ((year - 30) / 100 - (year - 30) / 400);
        return ((DAYSPERYEAR * year) + (year - 2) / 4 - adjust + yday);
    }
}
/*********************************************************************************************************
** 函数名称: gmtime_r
** 功能描述:
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
struct tm *lib_gmtime_r (const time_t *timerp, struct tm *tmp)
{
    time_t      timer;
    INT64       days;
    INT64       timeOfDay;
    INT64       year;
    INT64       mon;
    lib_lldiv_t result;

    if (!timerp || !tmp) {
        return  (LW_NULL);
    }

    timer = *timerp;

    /*
     * Calulate number of days since epoch
     */
    days      = timer / SECSPERDAY;
    timeOfDay = timer % SECSPERDAY;

    /*
     * If time of day is negative, subtract one day, and add SECSPERDAY
     * to make it positive.
     */
    if (timeOfDay < 0) {
        timeOfDay += SECSPERDAY;
        days -= 1;
    }

    /*
     * Calulate number of years since epoch
     */
    year = days / DAYSPERYEAR;
    while (__daysSinceEpoch((INT)year, 0) > days) {
        year--;
    }

    /*
     * Calulate the day of the week
     */
    tmp->tm_wday = (INT)((days + EPOCH_WDAY) % DAYSPERWEEK);

    /*
     * If there is a negative weekday, add DAYSPERWEEK to make it positive
     */
    if (tmp->tm_wday < 0) {
        tmp->tm_wday += DAYSPERWEEK;
    }

    /*
     * Find year and remaining days
     */
    days -= __daysSinceEpoch((INT)year, 0);
    year += EPOCH_YEAR;

    /*
     * Find month
     */
    for (mon = 0;
         (mon < 11) && (days >= __daysSinceYear((INT)(year - TM_YEAR_BASE), (INT)(mon + 1), 0));
         mon++);

    /*
     * Initialize tm structure
     */
    tmp->tm_year = (INT)(year - TM_YEAR_BASE); /* years since 1900 */
    tmp->tm_mon  = (INT)mon;
    tmp->tm_mday = (INT)(days - __daysSinceYear(tmp->tm_year, (INT)mon, 0)) + 1;
    tmp->tm_yday = (INT)__daysSinceYear(tmp->tm_year, (INT)mon, tmp->tm_mday) - 1;
    tmp->tm_hour = (INT)(timeOfDay / SECSPERHOUR);

    timeOfDay %= SECSPERHOUR;
    result     = lib_lldiv(timeOfDay, SECSPERMIN);

    tmp->tm_min   = (int)result.quot;
    tmp->tm_sec   = (int)result.rem;
    tmp->tm_isdst = 0;

    return  (tmp);
}
/*********************************************************************************************************
** 函数名称: lib_gmtime
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
struct tm  *lib_gmtime (const time_t *timer)
{
    static struct tm  timeBuffer;
    
    return  (lib_gmtime_r(timer, &timeBuffer));
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
