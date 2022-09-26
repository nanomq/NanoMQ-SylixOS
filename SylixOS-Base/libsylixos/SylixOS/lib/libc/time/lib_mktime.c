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
** 文   件   名: lib_mktime.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2008 年 08 月 30 日
**
** 描        述: 系统库.

** BUG:
2009.09.04  mktime() 应该返回带有时区信息的 time_t.
2011.04.23  mktime() 入口参数带有时区信息, 返回参数应该为 UTC 时间.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
  函数声明
*********************************************************************************************************/
extern INT __daysSinceYear(INT year, INT month, INT mday);
extern INT __daysSinceEpoch(INT year, INT yday);
/*********************************************************************************************************
** 函数名称: __tmNormalize
** 功能描述: This function is used to reduce units to a range [0,base]
** 输　入  : tens      tens
**           units     units
**           base      base
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID __tmNormalize (INT  *tens, INT  *units, INT  base)
{
    *tens  += *units / base;
    *units %= base;

    if ((*units % base) < 0) {
    	(*tens)--;
    	*units += base;
    }
}
/*********************************************************************************************************
** 函数名称: __tmValidate
** 功能描述: validate the broken-down structure.
** 输　入  : tmp  broken-down structure
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __tmValidate (struct tm  *tmp)
{
    struct tm tmStruct;
    int       jday;
    int       mon;

    /*
     * Adjust timeptr to reflect a legal time
     * Is it within range 1970-2038?
     */
    tmStruct = *tmp;

    __tmNormalize(&tmStruct.tm_min, &tmStruct.tm_sec, SECSPERMIN);
    __tmNormalize(&tmStruct.tm_hour, &tmStruct.tm_min, MINSPERHOUR);
    __tmNormalize(&tmStruct.tm_mday, &tmStruct.tm_hour, HOURSPERDAY);
    __tmNormalize(&tmStruct.tm_year, &tmStruct.tm_mon, MONSPERYEAR);

    /*
     * tm_mday may not be in the correct range - check
     */
    jday = __daysSinceYear(tmStruct.tm_year, tmStruct.tm_mon , tmStruct.tm_mday);
    if (jday < 0) {
    	tmStruct.tm_year--;
    	jday += DAYSPERYEAR;
    }

    for (mon = 0; 
         (jday > __daysSinceYear(tmStruct.tm_year, mon+1, 0)) && (mon < 11);
         mon++);                                                        /*  Calulate month and day      */

    tmStruct.tm_mon  = mon;
    tmStruct.tm_mday = jday - __daysSinceYear(tmStruct.tm_year, mon, 0);
    tmStruct.tm_wday = 0;
    tmStruct.tm_yday = 0;

    *tmp = tmStruct;
}
/*********************************************************************************************************
** 函数名称: lib_timegm
** 功能描述: 生成一个 time_t 类型时间
** 输　入  : tmp       UTC time
** 输　出  : UTC time_t
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
time_t  lib_timegm (struct tm  *tmp)
{
    time_t timeIs = 0;
    int    days   = 0;

    if (!tmp) {
        _ErrorHandle(EINVAL);
        return  ((time_t)PX_ERROR);
    }

    __tmValidate(tmp);                                                  /*  Validate tm structure       */

    timeIs += (tmp->tm_sec +
    	      (time_t)(tmp->tm_min * SECSPERMIN) +
    	      (time_t)(tmp->tm_hour * SECSPERHOUR));                    /*  Calulate time_t value       */
    days   += __daysSinceYear(tmp->tm_year, tmp->tm_mon, tmp->tm_mday);

    tmp->tm_yday = (days - 1);

    if ((tmp->tm_year + TM_YEAR_BASE) < EPOCH_YEAR) {
        _ErrorHandle(ENOTSUP);
    	return  ((time_t)PX_ERROR);
    }

    days = __daysSinceEpoch(tmp->tm_year - (EPOCH_YEAR - TM_YEAR_BASE),
                            tmp->tm_yday);                              /*  days in previous years      */

    tmp->tm_wday = (days + EPOCH_WDAY) % DAYSPERWEEK;

    timeIs += (days * SECSPERDAY);

    return  (timeIs);
}
/*********************************************************************************************************
** 函数名称: lib_mktime
** 功能描述: 生成一个 time_t 类型时间
** 输　入  : tmp       local time
** 输　出  : UTC time_t
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
time_t  lib_mktime (struct tm  *tmp)
{
    time_t timeIs = 0;
    int    days   = 0;

    if (!tmp) {
        _ErrorHandle(EINVAL);
        return  ((time_t)PX_ERROR);
    }

    __tmValidate(tmp);                                                  /*  Validate tm structure       */

    timeIs += (tmp->tm_sec +
    	      (time_t)(tmp->tm_min * SECSPERMIN) +
    	      (time_t)(tmp->tm_hour * SECSPERHOUR));                    /*  Calulate time_t value       */
    days   += __daysSinceYear(tmp->tm_year, tmp->tm_mon, tmp->tm_mday);

    tmp->tm_yday = (days - 1);

    if ((tmp->tm_year + TM_YEAR_BASE) < EPOCH_YEAR) {
        _ErrorHandle(EINVAL);
    	return  ((time_t)PX_ERROR);
    }

    days = __daysSinceEpoch(tmp->tm_year - (EPOCH_YEAR - TM_YEAR_BASE),
                            tmp->tm_yday );                             /*  days in previous years      */

    tmp->tm_wday = (days + EPOCH_WDAY) % DAYSPERWEEK;

    timeIs += (days * SECSPERDAY);
    timeIs  = LOCAL2UTC(timeIs);

    return  (timeIs);
}

#endif                                                                  /*  LW_CFG_RTC_EN               */
/*********************************************************************************************************
  END
*********************************************************************************************************/
