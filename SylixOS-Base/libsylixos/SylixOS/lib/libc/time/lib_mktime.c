/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: lib_mktime.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2008 �� 08 �� 30 ��
**
** ��        ��: ϵͳ��.

** BUG:
2009.09.04  mktime() Ӧ�÷��ش���ʱ����Ϣ�� time_t.
2011.04.23  mktime() ��ڲ�������ʱ����Ϣ, ���ز���Ӧ��Ϊ UTC ʱ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "lib_local.h"
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
extern INT __daysSinceYear(INT year, INT month, INT mday);
extern INT __daysSinceEpoch(INT year, INT yday);
/*********************************************************************************************************
** ��������: __tmNormalize
** ��������: This function is used to reduce units to a range [0,base]
** �䡡��  : tens      tens
**           units     units
**           base      base
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: __tmValidate
** ��������: validate the broken-down structure.
** �䡡��  : tmp  broken-down structure
** �䡡��  : 
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: lib_timegm
** ��������: ����һ�� time_t ����ʱ��
** �䡡��  : tmp       UTC time
** �䡡��  : UTC time_t
** ȫ�ֱ���: 
** ����ģ��: 
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
** ��������: lib_mktime
** ��������: ����һ�� time_t ����ʱ��
** �䡡��  : tmp       local time
** �䡡��  : UTC time_t
** ȫ�ֱ���: 
** ����ģ��: 
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
