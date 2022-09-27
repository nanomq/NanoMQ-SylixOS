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
** ��   ��   ��: lwip_shell6.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2013 �� 09 �� 27 ��
**
** ��        ��: lwip ipv6 shell ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if (LW_CFG_NET_EN > 0) && (LW_CFG_SHELL_EN > 0)
#include "socket.h"
#include "net/if.h"
#include "netinet6/in6.h"
/*********************************************************************************************************
  ipv6 ������Ϣ
*********************************************************************************************************/
#if LWIP_IPV6
static const CHAR   _G_cIpv6Help[] = {
    "add/delete IPv6 address\n"
    "address   [ifname [address%prefixlen]]  add an ipv6 address for given interface\n"
    "noaddress [ifname [address%prefixlen]]  delete an ipv6 address for given interface\n"
    "autocfg   [ifname yes | no]             enable or disable auto configure for given interface\n"
};
/*********************************************************************************************************
** ��������: __ifreq6Init
** ��������: ͨ��������ʼ�� in6_ifreq �ṹ
** �䡡��  : pifeq6        ��Ҫ��ʼ���Ľṹ
**           pcIf          ����ӿ�
**           pcIpv6        IPv6 ��ַ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __ifreq6Init (struct in6_ifreq *pifeq6, CPCHAR pcIf, PCHAR pcIpv6)
{
    PCHAR pcDiv;

    pifeq6->ifr6_ifindex = if_nametoindex(pcIf);
    if (!pifeq6->ifr6_ifindex) {
        return  (PX_ERROR);
    }
    
    pcDiv = lib_strchr(pcIpv6, '%');
    if (pcDiv) {
        *pcDiv = PX_EOS;
        pcDiv++;
        inet6_aton(pcIpv6, &pifeq6->ifr6_addr_array->ifr6a_addr);
        pifeq6->ifr6_addr_array->ifr6a_prefixlen = lib_atoi(pcDiv);
    
    } else {
        inet6_aton(pcIpv6, &pifeq6->ifr6_addr_array->ifr6a_addr);
        pifeq6->ifr6_addr_array->ifr6a_prefixlen = 64;                  /*  default prefixlen           */
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIpv6Address
** ��������: ϵͳ���� "ipv6 address"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6Address (INT  iArgC, PCHAR  *ppcArgV)
{
    struct in6_ifreq    ifeq6;
    struct in6_ifr_addr ifraddr6;
    
    INT iSock;
    
    if (iArgC < 4) {
        fprintf(stderr, "%s", _G_cIpv6Help);
        return  (PX_ERROR);
    }
    
    ifeq6.ifr6_len = sizeof(struct in6_ifr_addr);
    ifeq6.ifr6_addr_array = &ifraddr6;
    
    if (__ifreq6Init(&ifeq6, ppcArgV[2], ppcArgV[3])) {
        fprintf(stderr, "invalid interface.\n");
        return  (PX_ERROR);
    }
    
    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    if (ioctl(iSock, SIOCSIFADDR6, &ifeq6)) {
        INT   iErrNo = errno;
        close(iSock);
        fprintf(stderr, "can not set/add ipv6 address error: %s\n", lib_strerror(iErrNo));
        return  (PX_ERROR);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIpv6Noaddress
** ��������: ϵͳ���� "ipv6 noaddress"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6Noaddress (INT  iArgC, PCHAR  *ppcArgV)
{
    struct in6_ifreq    ifeq6;
    struct in6_ifr_addr ifraddr6;
    
    INT iSock;
    
    if (iArgC < 4) {
        fprintf(stderr, "%s", _G_cIpv6Help);
        return  (PX_ERROR);
    }
    
    ifeq6.ifr6_len = sizeof(struct in6_ifr_addr);
    ifeq6.ifr6_addr_array = &ifraddr6;
    
    if (__ifreq6Init(&ifeq6, ppcArgV[2], ppcArgV[3])) {
        fprintf(stderr, "invalid interface.\n");
        return  (PX_ERROR);
    }
    
    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }
    
    if (ioctl(iSock, SIOCDIFADDR6, &ifeq6)) {
        INT   iErrNo = errno;
        close(iSock);
        fprintf(stderr, "can not delete ipv6 address error: %s\n", lib_strerror(iErrNo));
        return  (PX_ERROR);
    }
    
    close(iSock);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __tshellIpv6Autocfg
** ��������: ϵͳ���� "ipv6 autocfg"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6Autocfg (INT  iArgC, PCHAR  *ppcArgV)
{
    struct ifreq ifreq;

    INT iSock;

    if (iArgC < 3) {
        fprintf(stderr, "%s", _G_cIpv6Help);
        return  (PX_ERROR);
    }

    if (strnlen(ppcArgV[2], IFNAMSIZ) >= IFNAMSIZ) {
        fprintf(stderr, "ifname to long.\n");
        return  (PX_ERROR);
    }

    lib_strcpy(ifreq.ifr_name, ppcArgV[2]);

    iSock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (iSock < 0) {
        fprintf(stderr, "can not create socket error: %s\n", lib_strerror(errno));
        return  (PX_ERROR);
    }

    if (iArgC == 3) {
        if (ioctl(iSock, SIOCGIFAUTOCFG, &ifreq)) {
            INT   iErrNo = errno;
            close(iSock);
            fprintf(stderr, "can not get IPv6 auto configure setting: %s\n", lib_strerror(iErrNo));
            return  (PX_ERROR);
        }

        close(iSock);
        printf("interface %s IPv6 auto configure: %s\n",
               ppcArgV[2], ifreq.ifr_autocfg ? "yes" : "no");
        return  (ERROR_NONE);

    } else {
        if (*ppcArgV[3] == 'y' || *ppcArgV[3] == 'Y') {
            ifreq.ifr_autocfg = 1;
        } else {
            ifreq.ifr_autocfg = 0;
        }
        if (ioctl(iSock, SIOCSIFAUTOCFG, &ifreq)) {
            INT   iErrNo = errno;
            close(iSock);
            fprintf(stderr, "can not set IPv6 auto configure setting: %s\n", lib_strerror(iErrNo));
            return  (PX_ERROR);
        }

        close(iSock);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __tshellIpv6
** ��������: ϵͳ���� "ipv6"
** �䡡��  : iArgC         ��������
**           ppcArgV       ������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __tshellIpv6 (INT  iArgC, PCHAR  *ppcArgV)
{
    if (iArgC < 2) {
        fprintf(stderr, "%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
    
    if (lib_strcmp(ppcArgV[1], "address") == 0) {                       /*  ���� ipv6 ��ַ              */
        return  (__tshellIpv6Address(iArgC, ppcArgV));
    
    } else if (lib_strcmp(ppcArgV[1], "noaddress") == 0) {              /*  ɾ�� ipv6 ��ַ              */
        return  (__tshellIpv6Noaddress(iArgC, ppcArgV));
    
    } else if (lib_strcmp(ppcArgV[1], "autocfg") == 0) {                /*  ���� auto config ѡ��       */
        return  (__tshellIpv6Autocfg(iArgC, ppcArgV));

    } else {
        fprintf(stderr, "%s", _G_cIpv6Help);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
** ��������: __tshellNet6Init
** ��������: ע�� IPv6 ר������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __tshellNet6Init (VOID)
{
    API_TShellKeywordAdd("ipv6", __tshellIpv6);
    API_TShellFormatAdd("ipv6",  " ...");
    API_TShellHelpAdd("ipv6",    _G_cIpv6Help);
}

#endif                                                                  /*  LWIP_IPV6                   */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
