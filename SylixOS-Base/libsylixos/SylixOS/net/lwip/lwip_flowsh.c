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
** 文   件   名: lwip_flowsh.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2017 年 12 月 21 日
**
** 描        述: 流量控制命令.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪控制
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_SHELL_EN > 0 && LW_CFG_NET_FLOWCTL_EN > 0
#include "net/flowctl.h"
/*********************************************************************************************************
  默认缓冲
*********************************************************************************************************/
#define FC_DEFAULT_BUF_SIZE_KB  (LW_CFG_NET_FLOWCTL_DEF_BSIZE >> 10)
/*********************************************************************************************************
** 函数名称: __fc_show_ipv4
** 功能描述: 显示 IPv4 流控信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fc_show_ipv4 (VOID)
{
    INT                  iRet, i;
    INT                  iSock;
    CHAR                 cStrSSrc[IP4ADDR_STRLEN_MAX];
    CHAR                 cStrESrc[IP4ADDR_STRLEN_MAX];
    PCHAR                pcProto;
    INT                  iSPort, iEPort;
    struct fcentry_list  fcentrylist;
    struct fcentry      *pfcentry;
    
    
    fcentrylist.fcl_type  = FCT_IP;
    fcentrylist.fcl_bcnt  = 0;
    fcentrylist.fcl_num   = 0;
    fcentrylist.fcl_total = 0;
    fcentrylist.fcl_buf   = LW_NULL;
    
    printf("IPv4 Flow Control Table:\n");
    printf("Source(s)       Source(e)       Proto   Port(s)  Port(e)  Uplink (KB/s)   Downlink (KB/s) Stat Iface\n");
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTFC, &fcentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (fcentrylist.fcl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }

    fcentrylist.fcl_buf = (struct fcentry *)__SHEAP_ALLOC(sizeof(struct fcentry) * fcentrylist.fcl_total);
    if (!fcentrylist.fcl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    fcentrylist.fcl_bcnt = fcentrylist.fcl_total;
    iRet = ioctl(iSock, SIOCLSTFC, &fcentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTFC' error: %s!\n", lib_strerror(errno));
        __SHEAP_FREE(fcentrylist.fcl_buf);
        close(iSock);
        return;
    }
    close(iSock);
    
    for (i = 0; i < fcentrylist.fcl_num; i++) {
        pfcentry = &fcentrylist.fcl_buf[i];
        inet_ntoa_r(((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr, cStrSSrc, IP4ADDR_STRLEN_MAX);
        inet_ntoa_r(((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr,   cStrESrc, IP4ADDR_STRLEN_MAX);
        
        switch (pfcentry->fc_proto) {
        
        case 0:
            pcProto = "all";
            iSPort  = -1;
            iEPort  = -1;
            break;
        
        case IPPROTO_TCP:
            pcProto = "tcp";
            iSPort  = ntohs(pfcentry->fc_sport);
            iEPort  = ntohs(pfcentry->fc_eport);
            break;
        
        case IPPROTO_UDP:
        case IPPROTO_UDPLITE:
            pcProto = "udp";
            iSPort  = ntohs(pfcentry->fc_sport);
            iEPort  = ntohs(pfcentry->fc_eport);
            break;
        
        case IPPROTO_ICMP:
            pcProto = "icmp";
            iSPort  = -1;
            iEPort  = -1;
            break;
        
        default:
            pcProto = "unknown";
            iSPort  = -1;
            iEPort  = -1;
            break;
        }
        
        printf("%-15s %-15s %-7s %-8d %-8d %-15qd %-15qd %-4s %s\n",
               cStrSSrc, cStrESrc, pcProto, iSPort, iEPort,
               pfcentry->fc_uprate >> 10, 
               pfcentry->fc_downrate >> 10, 
               pfcentry->fc_enable ? "En" : "Dis",
               pfcentry->fc_ifname);
    }
    
    __SHEAP_FREE(fcentrylist.fcl_buf);
    printf("\n");
}
/*********************************************************************************************************
** 函数名称: __fc_show_ipv6
** 功能描述: 显示 IPv6 流控信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
#if LWIP_IPV6

static VOID  __fc_show_ipv6 (VOID)
{
    INT                  iRet, i;
    INT                  iSock;
    CHAR                 cStrSSrc[IP6ADDR_STRLEN_MAX];
    CHAR                 cStrESrc[IP6ADDR_STRLEN_MAX];
    PCHAR                pcProto;
    INT                  iSPort, iEPort;
    struct fcentry_list  fcentrylist;
    struct fcentry      *pfcentry;
    
    fcentrylist.fcl_type  = FCT_IP;
    fcentrylist.fcl_bcnt  = 0;
    fcentrylist.fcl_num   = 0;
    fcentrylist.fcl_total = 0;
    fcentrylist.fcl_buf   = LW_NULL;

    printf("IPv6 Flow Control Table:\n");
    printf("Source(s)                       Source(e)                       "
           "Proto   Port(s)  Port(e)  Uplink (KB/s)   Downlink (KB/s) Stat Iface\n");

    iSock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTFC, &fcentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (fcentrylist.fcl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }

    fcentrylist.fcl_buf = (struct fcentry *)__SHEAP_ALLOC(sizeof(struct fcentry) * fcentrylist.fcl_total);
    if (!fcentrylist.fcl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    fcentrylist.fcl_bcnt = fcentrylist.fcl_total;
    iRet = ioctl(iSock, SIOCLSTFC, &fcentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTFC' error: %s!\n", lib_strerror(errno));
        __SHEAP_FREE(fcentrylist.fcl_buf);
        close(iSock);
        return;
    }
    close(iSock);

    for (i = 0; i < fcentrylist.fcl_num; i++) {
        pfcentry = &fcentrylist.fcl_buf[i];
        inet6_ntoa_r(((struct sockaddr_in6 *)&pfcentry->fc_start)->sin6_addr, cStrSSrc, IP6ADDR_STRLEN_MAX);
        inet6_ntoa_r(((struct sockaddr_in6 *)&pfcentry->fc_end)->sin6_addr,   cStrESrc, IP6ADDR_STRLEN_MAX);

        switch (pfcentry->fc_proto) {
        
        case 0:
            pcProto = "all";
            iSPort  = -1;
            iEPort  = -1;
            break;
        
        case IPPROTO_TCP:
            pcProto = "tcp";
            iSPort  = ntohs(pfcentry->fc_sport);
            iEPort  = ntohs(pfcentry->fc_eport);
            break;
        
        case IPPROTO_UDP:
        case IPPROTO_UDPLITE:
            pcProto = "udp";
            iSPort  = ntohs(pfcentry->fc_sport);
            iEPort  = ntohs(pfcentry->fc_eport);
            break;
        
        case IPPROTO_ICMP:
            pcProto = "icmp";
            iSPort  = -1;
            iEPort  = -1;
            break;
        
        default:
            pcProto = "unknown";
            iSPort  = -1;
            iEPort  = -1;
            break;
        }
        
        printf("%-32s %-32s %-7s %-8d %-8d %-15qd %-15qd %-4s %s\n",
               cStrSSrc, cStrESrc, pcProto, iSPort, iEPort,
               pfcentry->fc_uprate >> 10, 
               pfcentry->fc_downrate >> 10, 
               pfcentry->fc_enable ? "En" : "Dis",
               pfcentry->fc_ifname);
    }

    __SHEAP_FREE(fcentrylist.fcl_buf);
    printf("\n");
}

#endif                                                                  /*  LWIP_IPV6                   */
/*********************************************************************************************************
** 函数名称: __fc_show_if
** 功能描述: 显示网络接口流控信息
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static VOID  __fc_show_if (VOID)
{
    INT                  iRet, i;
    INT                  iSock;
    struct fcentry_list  fcentrylist;
    struct fcentry      *pfcentry;
    
    fcentrylist.fcl_type  = FCT_IF;
    fcentrylist.fcl_bcnt  = 0;
    fcentrylist.fcl_num   = 0;
    fcentrylist.fcl_total = 0;
    fcentrylist.fcl_buf   = LW_NULL;
    
    printf("Net Interface Flow Control Table:\n");
    printf("Uplink (KB/s)   Downlink (KB/s) Stat Iface\n");
    
    iSock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return;
    }
    
    iRet = ioctl(iSock, SIOCLSTFC, &fcentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    if (fcentrylist.fcl_total == 0) {
        close(iSock);
        printf("\n");
        return;
    }

    fcentrylist.fcl_buf = (struct fcentry *)__SHEAP_ALLOC(sizeof(struct fcentry) * fcentrylist.fcl_total);
    if (!fcentrylist.fcl_buf) {
        fprintf(stderr, "error: %s!\n", lib_strerror(errno));
        close(iSock);
        return;
    }
    
    fcentrylist.fcl_bcnt = fcentrylist.fcl_total;
    iRet = ioctl(iSock, SIOCLSTFC, &fcentrylist);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCLSTFC' error: %s!\n", lib_strerror(errno));
        __SHEAP_FREE(fcentrylist.fcl_buf);
        close(iSock);
        return;
    }
    close(iSock);
    
    for (i = 0; i < fcentrylist.fcl_num; i++) {
        pfcentry = &fcentrylist.fcl_buf[i];
        printf("%-15qd %-15qd %-4s %s\n",
               pfcentry->fc_uprate >> 10, 
               pfcentry->fc_downrate >> 10, 
               pfcentry->fc_enable ? "En" : "Dis",
               pfcentry->fc_ifname);
    }
    
    __SHEAP_FREE(fcentrylist.fcl_buf);
    printf("\n");
}
/*********************************************************************************************************
** 函数名称: __fc_add_if
** 功能描述: 添加网络接口流控信息
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fc_add_if (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             iRet;
    INT             iSock;
    struct fcentry  fcentry;
    size_t          stBufSize = FC_DEFAULT_BUF_SIZE_KB;
    
    lib_bzero(&fcentry, sizeof(struct fcentry));
    fcentry.fc_type   = FCT_IF;
    fcentry.fc_enable = 1;
    
    if ((iArgC < 7) || lib_strcmp(ppcArgV[3], "dev")) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(fcentry.fc_ifname, ppcArgV[4], IF_NAMESIZE);
    
    if (sscanf(ppcArgV[5], "%qu", &fcentry.fc_uprate) != 1) {
        goto    __arg_error;
    }
    
    if (sscanf(ppcArgV[6], "%qu", &fcentry.fc_downrate) != 1) {
        goto    __arg_error;
    }
    
    if (iArgC > 7) {
        if (sscanf(ppcArgV[7], "%zu", &stBufSize) != 1) {
            goto    __arg_error;
        }
    }
    
    fcentry.fc_uprate   <<= 10;
    fcentry.fc_downrate <<= 10;
    fcentry.fc_bufsize    = (stBufSize << 10);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCADDFC, &fcentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCADDFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fc_add_ip
** 功能描述: 添加 IPv4 流控信息
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fc_add_ip (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             iRet, iBuf, iIndex;
    INT             iSPort, iEPort;
    INT             iSock;
    struct fcentry  fcentry;
    size_t          stBufSize = FC_DEFAULT_BUF_SIZE_KB;
    
    lib_bzero(&fcentry, sizeof(struct fcentry));
    fcentry.fc_type   = FCT_IP;
    fcentry.fc_enable = 1;
    
    if (iArgC < 10) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!lib_strcmp(ppcArgV[5], "all")) {
        fcentry.fc_proto = 0;
        iBuf = 10;
     
    } else if (!lib_strcmp(ppcArgV[5], "tcp") && (iArgC >= 12)) {
        fcentry.fc_proto = IPPROTO_TCP;
        iBuf = 12;
    
    } else if (!lib_strcmp(ppcArgV[5], "udp") && (iArgC >= 12)) {
        fcentry.fc_proto = IPPROTO_UDP;
        iBuf = 12;
    
    } else {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&fcentry.fc_start)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&fcentry.fc_end)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (fcentry.fc_proto) {
        if (sscanf(ppcArgV[6], "%d", &iSPort) != 1) {
            goto    __arg_error;
        }
        
        if (sscanf(ppcArgV[7], "%d", &iEPort) != 1) {
            goto    __arg_error;
        }
        
        fcentry.fc_sport = htons(iSPort);
        fcentry.fc_eport = htons(iEPort);
        
        iIndex = 9;
        
    } else {
        iIndex = 7;
    }
    
    lib_strlcpy(fcentry.fc_ifname, ppcArgV[iIndex], IF_NAMESIZE);
    iIndex++;
    
    if (sscanf(ppcArgV[iIndex], "%qu", &fcentry.fc_uprate) != 1) {
        goto    __arg_error;
    }
    iIndex++;
    
    if (sscanf(ppcArgV[iIndex], "%qu", &fcentry.fc_downrate) != 1) {
        goto    __arg_error;
    }
    iIndex++;
    
    if (iArgC > iBuf) {
        if (sscanf(ppcArgV[iIndex], "%zu", &stBufSize) != 1) {
            goto    __arg_error;
        }
    }
    
    fcentry.fc_uprate   <<= 10;
    fcentry.fc_downrate <<= 10;
    fcentry.fc_bufsize    = (stBufSize << 10);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCADDFC, &fcentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCADDFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fc_delete_if
** 功能描述: 删除网络接口流控信息
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fc_delete_if (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             iRet;
    INT             iSock;
    struct fcentry  fcentry;
    
    lib_bzero(&fcentry, sizeof(struct fcentry));
    fcentry.fc_type   = FCT_IF;
    fcentry.fc_enable = 1;
    
    if ((iArgC < 5) && lib_strcmp(ppcArgV[3], "dev")) {
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(fcentry.fc_ifname, ppcArgV[4], IF_NAMESIZE);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCDELFC, &fcentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCDELFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fc_delete_ip
** 功能描述: 删除 IPv4 流控信息
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fc_delete_ip (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             iRet, iIndex;
    INT             iSPort, iEPort;
    INT             iSock;
    struct fcentry  fcentry;
    
    lib_bzero(&fcentry, sizeof(struct fcentry));
    fcentry.fc_type   = FCT_IP;
    fcentry.fc_enable = 1;
    
    if (iArgC < 8) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!lib_strcmp(ppcArgV[5], "all")) {
        fcentry.fc_proto = 0;
     
    } else if (!lib_strcmp(ppcArgV[5], "tcp") && (iArgC >= 10)) {
        fcentry.fc_proto = IPPROTO_TCP;
    
    } else if (!lib_strcmp(ppcArgV[5], "udp") && (iArgC >= 10)) {
        fcentry.fc_proto = IPPROTO_UDP;
    
    } else {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&fcentry.fc_start)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&fcentry.fc_end)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (fcentry.fc_proto) {
        if (sscanf(ppcArgV[6], "%d", &iSPort) != 1) {
            goto    __arg_error;
        }
        
        if (sscanf(ppcArgV[7], "%d", &iEPort) != 1) {
            goto    __arg_error;
        }
        
        fcentry.fc_sport = htons(iSPort);
        fcentry.fc_eport = htons(iEPort);
        
        iIndex = 9;
        
    } else {
        iIndex = 7;
    }
    
    lib_strlcpy(fcentry.fc_ifname, ppcArgV[iIndex], IF_NAMESIZE);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCDELFC, &fcentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCDELFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fc_change_if
** 功能描述: 修改网络接口流控信息
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fc_change_if (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             iRet;
    INT             iSock;
    struct fcentry  fcentry;
    size_t          stBufSize = FC_DEFAULT_BUF_SIZE_KB;
    
    lib_bzero(&fcentry, sizeof(struct fcentry));
    fcentry.fc_type   = FCT_IF;
    fcentry.fc_enable = 1;
    
    if ((iArgC < 7) || lib_strcmp(ppcArgV[3], "dev")) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    lib_strlcpy(fcentry.fc_ifname, ppcArgV[4], IF_NAMESIZE);
    
    if (sscanf(ppcArgV[5], "%qu", &fcentry.fc_uprate) != 1) {
        goto    __arg_error;
    }
    
    if (sscanf(ppcArgV[6], "%qu", &fcentry.fc_downrate) != 1) {
        goto    __arg_error;
    }
    
    if (iArgC > 7) {
        if (sscanf(ppcArgV[6], "%zu", &stBufSize) != 1) {
            goto    __arg_error;
        }
    }
    
    fcentry.fc_uprate   <<= 10;
    fcentry.fc_downrate <<= 10;
    fcentry.fc_bufsize    = (stBufSize << 10);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCCHGFC, &fcentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCCHGFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __fc_change_ip
** 功能描述: 修改 IPv4 流控信息
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __fc_change_ip (INT  iArgC, PCHAR  *ppcArgV)
{
    INT             iRet, iBuf, iIndex;
    INT             iSPort, iEPort;
    INT             iSock;
    struct fcentry  fcentry;
    size_t          stBufSize = FC_DEFAULT_BUF_SIZE_KB;
    
    lib_bzero(&fcentry, sizeof(struct fcentry));
    fcentry.fc_type   = FCT_IP;
    fcentry.fc_enable = 1;
    
    if (iArgC < 10) {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    if (!lib_strcmp(ppcArgV[5], "all")) {
        fcentry.fc_proto = 0;
        iBuf = 10;
     
    } else if (!lib_strcmp(ppcArgV[5], "tcp") && (iArgC >= 12)) {
        fcentry.fc_proto = IPPROTO_TCP;
        iBuf = 12;
    
    } else if (!lib_strcmp(ppcArgV[5], "udp") && (iArgC >= 12)) {
        fcentry.fc_proto = IPPROTO_UDP;
        iBuf = 12;
    
    } else {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[3], &((struct sockaddr_in *)&fcentry.fc_start)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (!inet_aton(ppcArgV[4], &((struct sockaddr_in *)&fcentry.fc_end)->sin_addr)) {
        goto    __arg_error;
    }
    
    if (fcentry.fc_proto) {
        if (sscanf(ppcArgV[6], "%d", &iSPort) != 1) {
            goto    __arg_error;
        }
        
        if (sscanf(ppcArgV[7], "%d", &iEPort) != 1) {
            goto    __arg_error;
        }
        
        fcentry.fc_sport = htons(iSPort);
        fcentry.fc_eport = htons(iEPort);
        
        iIndex = 9;
        
    } else {
        iIndex = 7;
    }
    
    lib_strlcpy(fcentry.fc_ifname, ppcArgV[iIndex], IF_NAMESIZE);
    iIndex++;
    
    if (sscanf(ppcArgV[iIndex], "%qu", &fcentry.fc_uprate) != 1) {
        goto    __arg_error;
    }
    iIndex++;
    
    if (sscanf(ppcArgV[iIndex], "%qu", &fcentry.fc_downrate) != 1) {
        goto    __arg_error;
    }
    iIndex++;
    
    if (iArgC > iBuf) {
        if (sscanf(ppcArgV[iIndex], "%zu", &stBufSize) != 1) {
            goto    __arg_error;
        }
    }
    
    fcentry.fc_uprate   <<= 10;
    fcentry.fc_downrate <<= 10;
    fcentry.fc_bufsize    = (stBufSize << 10);
    
    iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s!\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    iRet = ioctl(iSock, SIOCCHGFC, &fcentry);
    if (iRet < 0) {
        fprintf(stderr, "command 'SIOCCHGFC' error: %s!\n", lib_strerror(errno));
        close(iSock);
        return  (PX_ERROR);
    }
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __tshellFlowctl
** 功能描述: 系统命令 "flowctl"
** 输　入  : iArgC         参数个数
**           ppcArgV       参数表
** 输　出  : ERROR
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __tshellFlowctl (INT  iArgC, PCHAR  *ppcArgV)
{
    INT  iRet;

    if (iArgC == 1) {
        __fc_show_ipv4();
#if LWIP_IPV6
        __fc_show_ipv6();
#endif                                                                  /*  LWIP_IPV6                   */
        __fc_show_if();
        return  (ERROR_NONE);
    
    } else if (iArgC > 2) {
        if (!lib_strcmp(ppcArgV[1], "add")) {
            if (!lib_strcmp(ppcArgV[2], "if")) {
                iRet = __fc_add_if(iArgC, ppcArgV);
                
            } else if (!lib_strcmp(ppcArgV[2], "ip")) {
                iRet = __fc_add_ip(iArgC, ppcArgV);
            
            } else {
                goto    __arg_error;
            }
            
        } else if (!lib_strcmp(ppcArgV[1], "del") || !lib_strcmp(ppcArgV[1], "delete")) {
            if (!lib_strcmp(ppcArgV[2], "if")) {
                iRet = __fc_delete_if(iArgC, ppcArgV);
                
            } else if (!lib_strcmp(ppcArgV[2], "ip")) {
                iRet = __fc_delete_ip(iArgC, ppcArgV);
            
            } else {
                goto    __arg_error;
            }
            
        } else if (!lib_strcmp(ppcArgV[1], "chg") || !lib_strcmp(ppcArgV[1], "change")) {
            if (!lib_strcmp(ppcArgV[2], "if")) {
                iRet = __fc_change_if(iArgC, ppcArgV);
                
            } else if (!lib_strcmp(ppcArgV[2], "ip")) {
                iRet = __fc_change_ip(iArgC, ppcArgV);
            
            } else {
                goto    __arg_error;
            }
            
        } else {
            goto    __arg_error;
        }
        
    } else {
__arg_error:
        fprintf(stderr, "arguments error!\n");
        return  (-ERROR_TSHELL_EPARAM);
    }
    
    return  (iRet);
}
/*********************************************************************************************************
** 函数名称: __tshellFlowctlInit
** 功能描述: 注册流量控制命令
** 输　入  : NONE
** 输　出  : NONE
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID __tshellFlowctlInit (VOID)
{
    API_TShellKeywordAdd("flowctl", __tshellFlowctl);
    API_TShellFormatAdd("flowctl", " [add | del | chg] [ip | if] [...]");
    API_TShellHelpAdd("flowctl",   "show, add, delete, change flow control status.\n"
    "eg. flowctl\n"
    "    flowctl add ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1 50 100 64\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is 64KBytes\n\n"
    
    "    flowctl add ip 192.168.1.1 192.168.1.10 udp 20 80 dev en1 50 100\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 udp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl add ip 192.168.1.1 192.168.1.10 all dev en1 50 100\n"
    "       add a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 all protocol\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl chg ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1 50 100\n"
    "       change flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl del ip 192.168.1.1 192.168.1.10 tcp 20 80 dev en1\n"
    "       delete a flow control rule: iface: en1(LAN Port) ip frome 192.168.1.1 to 192.168.1.10 tcp protocol port(20 ~ 80)\n\n"
    
    "    flowctl add if dev en1 50 100 64\n"
    "       add flow control rule: iface: en1(LAN Port)\n"
    "       uplink 50KBytes downlink 100KBytes buffer 64K\n\n"
    
    "    flowctl chg if dev en1 50 100\n"
    "       change flow control rule: iface: en1(LAN Port)\n"
    "       uplink 50KBytes downlink 100KBytes buffer is default size\n\n"
    
    "    flowctl del if dev en1\n"
    "       delete a flow control rule: iface: en1(LAN Port)\n");
}

#endif                                                                  /*  LW_CFG_NET_EN               */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
                                                                        /*  LW_CFG_NET_FLOWCTL_EN > 0   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
