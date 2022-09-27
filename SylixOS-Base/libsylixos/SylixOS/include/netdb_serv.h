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
** 文   件   名: netdb_serv.h
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2022 年 03 月 23 日
**
** 描        述: DNS server set / get.
*********************************************************************************************************/

#ifndef __NETDB_SERV_H
#define __NETDB_SERV_H

#include <SylixOS.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif                                                                  /*  __cplusplus                 */

LW_API INT  get_dns_server_info_4(UINT iIndex, struct in_addr *inaddr);
LW_API INT  set_dns_server_info_4(UINT iIndex, UINT iIfidx, const struct in_addr *inaddr);

#if LW_CFG_NET_IPV6 > 0
LW_API INT  get_dns_server_info_6(UINT iIndex, struct in6_addr *in6addr);
LW_API INT  set_dns_server_info_6(UINT iIndex, UINT iIfidx, const struct in6_addr *in6addr);
#endif

#ifdef __cplusplus
}
#endif                                                                  /*  __cplusplus                 */

#endif                                                                  /*  __NETDB_SERV_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
