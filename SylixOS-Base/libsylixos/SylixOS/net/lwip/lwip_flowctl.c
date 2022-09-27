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
** ��   ��   ��: lwip_flowctl.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2017 �� 12 �� 08 ��
**
** ��        ��: ioctl ��������.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  �ü�����
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0 && LW_CFG_NET_FLOWCTL_EN > 0
#include "lwip/inet.h"
#include "net/flowctl.h"
#include "flowctl/cor_flowctl.h"
#include "flowctl/ip4_flowctl.h"
#include "flowctl/ip6_flowctl.h"
#include "flowctl/net_flowctl.h"
/*********************************************************************************************************
  �����С
*********************************************************************************************************/
#define LWIP_FC_MIN_BUF_SIZE    (16  * LW_CFG_KB_SIZE)
#define LWIP_FC_MAX_BUF_SIZE    (128 * LW_CFG_MB_SIZE)
/*********************************************************************************************************
  �ٶȲ���
*********************************************************************************************************/
#define LWIP_FC_U_TO_K(rate)    (rate / LW_CFG_NET_FLOWCTL_HZ)
#define LWIP_FC_K_TO_U(rate)    (rate * LW_CFG_NET_FLOWCTL_HZ)
/*********************************************************************************************************
** ��������: __fcAdd4
** ��������: ���һ�� IPv4 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcAdd4 (const struct fcentry  *pfcentry)
{
    INT         iRet;
    ip4_addr_t  ipstart, ipend;
    
    if (pfcentry->fc_bufsize < (LWIP_FC_MIN_BUF_SIZE) ||
        pfcentry->fc_bufsize > (LWIP_FC_MAX_BUF_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    inet_addr_to_ip4addr(&ipstart, &((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr);
    inet_addr_to_ip4addr(&ipend,   &((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr);
    
    FC_LOCK();
    iRet = fcipv4_rule_get(pfcentry->fc_ifname, &ipstart, &ipend, pfcentry->fc_proto, 
                           pfcentry->fc_sport, pfcentry->fc_eport, LW_NULL,
                           LW_NULL, LW_NULL, LW_NULL);
    if (!iRet) {
        FC_UNLOCK();
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    iRet = fcnet_netif_add(pfcentry->fc_ifname);                        /*  ����֧������                */
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    iRet = fcipv4_rule_create(pfcentry->fc_ifname, &ipstart, &ipend, pfcentry->fc_proto, 
                              pfcentry->fc_sport, pfcentry->fc_eport, pfcentry->fc_enable,
                              LWIP_FC_U_TO_K(pfcentry->fc_downrate),
                              LWIP_FC_U_TO_K(pfcentry->fc_uprate),
                              pfcentry->fc_bufsize);
    if (iRet < -1) {
        FC_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
        
    } else if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcAdd6
** ��������: ���һ�� IPv6 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __fcAdd6 (const struct fcentry  *pfcentry)
{
    INT         iRet;
    ip6_addr_t  ip6start, ip6end;
    
    if (pfcentry->fc_bufsize < (LWIP_FC_MIN_BUF_SIZE) ||
        pfcentry->fc_bufsize > (LWIP_FC_MAX_BUF_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    inet6_addr_to_ip6addr(&ip6start, &((struct sockaddr_in6 *)&pfcentry->fc_start)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6end,   &((struct sockaddr_in6 *)&pfcentry->fc_end)->sin6_addr);
    
    FC_LOCK();
    iRet = fcipv6_rule_get(pfcentry->fc_ifname, &ip6start, &ip6end, pfcentry->fc_proto, 
                           pfcentry->fc_sport, pfcentry->fc_eport, LW_NULL,
                           LW_NULL, LW_NULL, LW_NULL);
    if (!iRet) {
        FC_UNLOCK();
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    iRet = fcnet_netif_add(pfcentry->fc_ifname);                        /*  ����֧������                */
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    iRet = fcipv6_rule_create(pfcentry->fc_ifname, &ip6start, &ip6end, pfcentry->fc_proto, 
                              pfcentry->fc_sport, pfcentry->fc_eport, pfcentry->fc_enable,
                              LWIP_FC_U_TO_K(pfcentry->fc_downrate), 
                              LWIP_FC_U_TO_K(pfcentry->fc_uprate), 
                              pfcentry->fc_bufsize);
    if (iRet < -1) {
        FC_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
        
    } else if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __fcAddIf
** ��������: ���һ������ӿ�����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcAddIf (const struct fcentry  *pfcentry)
{
    INT  iRet;
    
    if (pfcentry->fc_bufsize < (LWIP_FC_MIN_BUF_SIZE) ||
        pfcentry->fc_bufsize > (LWIP_FC_MAX_BUF_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    FC_LOCK();
    iRet = fcnet_netif_add(pfcentry->fc_ifname);                        /*  ����֧������                */
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    
    iRet = fcnet_netif_set(pfcentry->fc_ifname, pfcentry->fc_enable, 
                           LWIP_FC_U_TO_K(pfcentry->fc_downrate), 
                           LWIP_FC_U_TO_K(pfcentry->fc_uprate), 
                           pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    FC_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcDel4
** ��������: ɾ��һ�� IPv4 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcDel4 (const struct fcentry  *pfcentry)
{
    INT         iRet;
    ip4_addr_t  ipstart, ipend;
    
    inet_addr_to_ip4addr(&ipstart, &((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr);
    inet_addr_to_ip4addr(&ipend,   &((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr);
    
    FC_LOCK();
    iRet = fcipv4_rule_delete(pfcentry->fc_ifname, &ipstart, &ipend, pfcentry->fc_proto, 
                              pfcentry->fc_sport, pfcentry->fc_eport);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcDel6
** ��������: ɾ��һ�� IPv6 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __fcDel6 (const struct fcentry  *pfcentry)
{
    INT         iRet;
    ip6_addr_t  ip6start, ip6end;
    
    inet6_addr_to_ip6addr(&ip6start, &((struct sockaddr_in6 *)&pfcentry->fc_start)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6end,   &((struct sockaddr_in6 *)&pfcentry->fc_end)->sin6_addr);
    
    FC_LOCK();
    iRet = fcipv6_rule_delete(pfcentry->fc_ifname, &ip6start, &ip6end, pfcentry->fc_proto, 
                              pfcentry->fc_sport, pfcentry->fc_eport);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __fcDelIf
** ��������: ɾ��һ������ӿ�����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcDelIf (const struct fcentry  *pfcentry)
{
    INT  iRet;
    
    FC_LOCK();
    iRet = fcnet_netif_delete(pfcentry->fc_ifname);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcChg4
** ��������: �޸�һ�� IPv4 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcChg4 (const struct fcentry  *pfcentry)
{
    INT         iRet;
    ip4_addr_t  ipstart, ipend;
    
    if (pfcentry->fc_bufsize < (LWIP_FC_MIN_BUF_SIZE) ||
        pfcentry->fc_bufsize > (LWIP_FC_MAX_BUF_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    inet_addr_to_ip4addr(&ipstart, &((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr);
    inet_addr_to_ip4addr(&ipend,   &((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr);
    
    FC_LOCK();
    iRet = fcipv4_rule_change(pfcentry->fc_ifname, &ipstart, &ipend, pfcentry->fc_proto, 
                              pfcentry->fc_sport, pfcentry->fc_eport, pfcentry->fc_enable,
                              LWIP_FC_U_TO_K(pfcentry->fc_downrate), 
                              LWIP_FC_U_TO_K(pfcentry->fc_uprate), 
                              pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcChg6
** ��������: �޸�һ�� IPv6 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __fcChg6 (const struct fcentry  *pfcentry)
{
    INT         iRet;
    ip6_addr_t  ip6start, ip6end;
    
    if (pfcentry->fc_bufsize < (LWIP_FC_MIN_BUF_SIZE) ||
        pfcentry->fc_bufsize > (LWIP_FC_MAX_BUF_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    inet6_addr_to_ip6addr(&ip6start, &((struct sockaddr_in6 *)&pfcentry->fc_start)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6end,   &((struct sockaddr_in6 *)&pfcentry->fc_end)->sin6_addr);
    
    FC_LOCK();
    iRet = fcipv6_rule_change(pfcentry->fc_ifname, &ip6start, &ip6end, pfcentry->fc_proto, 
                              pfcentry->fc_sport, pfcentry->fc_eport, pfcentry->fc_enable,
                              LWIP_FC_U_TO_K(pfcentry->fc_downrate), 
                              LWIP_FC_U_TO_K(pfcentry->fc_uprate),
                              pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __fcChgIf
** ��������: �޸�һ������ӿ�����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcChgIf (const struct fcentry  *pfcentry)
{
    INT  iRet;
    
    if (pfcentry->fc_bufsize < (LWIP_FC_MIN_BUF_SIZE) ||
        pfcentry->fc_bufsize > (LWIP_FC_MAX_BUF_SIZE)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    FC_LOCK();
    iRet = fcnet_netif_set(pfcentry->fc_ifname, pfcentry->fc_enable, 
                           LWIP_FC_U_TO_K(pfcentry->fc_downrate), 
                           LWIP_FC_U_TO_K(pfcentry->fc_uprate), 
                           pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcGet4
** ��������: ��ȡһ�� IPv4 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcGet4 (struct fcentry  *pfcentry)
{
    INT         iRet;
    ip4_addr_t  ipstart, ipend;
    fc_rate_t   uprate, downrate;
    
    inet_addr_to_ip4addr(&ipstart, &((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr);
    inet_addr_to_ip4addr(&ipend,   &((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr);
    
    FC_LOCK();
    iRet = fcipv4_rule_get(pfcentry->fc_ifname, &ipstart, &ipend, pfcentry->fc_proto, 
                           pfcentry->fc_sport, pfcentry->fc_eport, &pfcentry->fc_enable,
                           &downrate, &uprate, &pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    pfcentry->fc_uprate   = LWIP_FC_K_TO_U(uprate);
    pfcentry->fc_downrate = LWIP_FC_K_TO_U(downrate);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcGet6
** ��������: ��ȡһ�� IPv6 ����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static INT  __fcGet6 (struct fcentry  *pfcentry)
{
    INT         iRet;
    ip6_addr_t  ip6start, ip6end;
    fc_rate_t   uprate, downrate;
    
    inet6_addr_to_ip6addr(&ip6start, &((struct sockaddr_in6 *)&pfcentry->fc_start)->sin6_addr);
    inet6_addr_to_ip6addr(&ip6end,   &((struct sockaddr_in6 *)&pfcentry->fc_end)->sin6_addr);
    
    FC_LOCK();
    iRet = fcipv6_rule_get(pfcentry->fc_ifname, &ip6start, &ip6end, pfcentry->fc_proto, 
                           pfcentry->fc_sport, pfcentry->fc_eport, &pfcentry->fc_enable,
                           &downrate, &uprate, &pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    pfcentry->fc_uprate   = LWIP_FC_K_TO_U(uprate);
    pfcentry->fc_downrate = LWIP_FC_K_TO_U(downrate);
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __fcGetIf
** ��������: ��ȡһ������ӿ�����ָ��
** �䡡��  : pfcentry  ����ָ��
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcGetIf (struct fcentry  *pfcentry)
{
    INT         iRet;
    fc_rate_t   uprate, downrate;
    
    FC_LOCK();
    iRet = fcnet_netif_get(pfcentry->fc_ifname, &pfcentry->fc_enable, 
                           &downrate, &uprate, &pfcentry->fc_bufsize);
    if (iRet < 0) {
        FC_UNLOCK();
        _ErrorHandle(ESRCH);
        return  (PX_ERROR);
    }
    FC_UNLOCK();
    
    pfcentry->fc_uprate   = LWIP_FC_K_TO_U(uprate);
    pfcentry->fc_downrate = LWIP_FC_K_TO_U(downrate);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcLst4Walk
** ��������: ��ȡ���� IPv4 ������Ϣ�ص�
** �䡡��  : fcdev     �����豸���ƿ�
**           fcipv4    IPv4 ���ؿ��ƿ�
**           pfcelist  ������Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  __fcLst4Walk (struct fc_dev *fcdev, struct fc_ipv4 *fcipv4, struct fcentry_list *pfcelist)
{
    if (pfcelist->fcl_num < pfcelist->fcl_bcnt) {
        struct fcentry *pfcentry = &pfcelist->fcl_buf[pfcelist->fcl_num];
        ip4_addr_t      ipsaddr, ipeaddr;
        
        ipsaddr.addr = PP_HTONL(fcipv4->start_hbo.addr);
        ipeaddr.addr = PP_HTONL(fcipv4->end_hbo.addr);
        
        pfcentry->fc_start.sa_len    = sizeof(struct sockaddr_in);
        pfcentry->fc_start.sa_family = AF_INET;
        pfcentry->fc_end.sa_len      = sizeof(struct sockaddr_in);
        pfcentry->fc_end.sa_family   = AF_INET;
        
        inet_addr_from_ip4addr(&((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr, &ipsaddr);
        inet_addr_from_ip4addr(&((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr,   &ipeaddr);
        
        pfcentry->fc_type   = FCT_IP;
        pfcentry->fc_enable = fcipv4->enable;
        pfcentry->fc_proto  = fcipv4->proto;
        pfcentry->fc_sport  = PP_HTONS(fcipv4->s_port_hbo);
        pfcentry->fc_eport  = PP_HTONS(fcipv4->e_port_hbo);
        
        lib_strlcpy(pfcentry->fc_ifname, fcdev->ifname, IF_NAMESIZE);
        
        pfcentry->fc_uprate   = LWIP_FC_K_TO_U(fcipv4->fcblk.r_rate);
        pfcentry->fc_downrate = LWIP_FC_K_TO_U(fcipv4->fcblk.s_rate);
        pfcentry->fc_bufsize  = fcipv4->fcblk.buf_size;
        pfcelist->fcl_num++;
    }
}
/*********************************************************************************************************
** ��������: __fcLst4
** ��������: ��ȡ���� IPv4 ������Ϣ
** �䡡��  : pfcelist  ������Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcLst4 (struct fcentry_list *pfcelist)
{
    UINT    uiTotal;
    
    pfcelist->fcl_num = 0;
    
    FC_LOCK();
    fcipv4_total_entry(&uiTotal);
    pfcelist->fcl_total = uiTotal;
    if (!pfcelist->fcl_bcnt || !pfcelist->fcl_buf) {
        FC_UNLOCK();
        return  (ERROR_NONE);
    }
    fcipv4_rule_traversal(__fcLst4Walk, pfcelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcLst6Walk
** ��������: ��ȡ���� IPv6 ������Ϣ�ص�
** �䡡��  : fcdev     �����豸���ƿ�
**           fcipv6    IPv6 ���ؿ��ƿ�
**           pfcelist  ������Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
#if LWIP_IPV6

static void  __fcLst6Walk (struct fc_dev *fcdev, struct fc_ipv6 *fcipv6, struct fcentry_list *pfcelist)
{
    if (pfcelist->fcl_num < pfcelist->fcl_bcnt) {
        struct fcentry *pfcentry = &pfcelist->fcl_buf[pfcelist->fcl_num];
        ip6_addr_t      ip6saddr, ip6eaddr;
        
        fcipv6_addr_hton(&ip6saddr, &fcipv6->start_hbo);
        fcipv6_addr_hton(&ip6eaddr, &fcipv6->end_hbo);
        
        pfcentry->fc_start.sa_len    = sizeof(struct sockaddr_in6);
        pfcentry->fc_start.sa_family = AF_INET6;
        pfcentry->fc_end.sa_len      = sizeof(struct sockaddr_in6);
        pfcentry->fc_end.sa_family   = AF_INET6;
        
        inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&pfcentry->fc_start)->sin6_addr, &ip6saddr);
        inet6_addr_from_ip6addr(&((struct sockaddr_in6 *)&pfcentry->fc_end)->sin6_addr,   &ip6eaddr);
        
        pfcentry->fc_type   = FCT_IP;
        pfcentry->fc_enable = fcipv6->enable;
        pfcentry->fc_proto  = fcipv6->proto;
        pfcentry->fc_sport  = PP_HTONS(fcipv6->s_port_hbo);
        pfcentry->fc_eport  = PP_HTONS(fcipv6->e_port_hbo);
        
        lib_strlcpy(pfcentry->fc_ifname, fcdev->ifname, IF_NAMESIZE);
        
        pfcentry->fc_uprate   = LWIP_FC_K_TO_U(fcipv6->fcblk.r_rate);
        pfcentry->fc_downrate = LWIP_FC_K_TO_U(fcipv6->fcblk.s_rate);
        pfcentry->fc_bufsize  = fcipv6->fcblk.buf_size;
        pfcelist->fcl_num++;
    }
}
/*********************************************************************************************************
** ��������: __fcLst6
** ��������: ��ȡ���� IPv6 ������Ϣ
** �䡡��  : pfcelist  ������Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcLst6 (struct fcentry_list *pfcelist)
{
    UINT    uiTotal;
    
    pfcelist->fcl_num = 0;
    
    FC_LOCK();
    fcipv6_total_entry(&uiTotal);
    pfcelist->fcl_total = uiTotal;
    if (!pfcelist->fcl_bcnt || !pfcelist->fcl_buf) {
        FC_UNLOCK();
        return  (ERROR_NONE);
    }
    fcipv6_rule_traversal(__fcLst6Walk, pfcelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}

#endif
/*********************************************************************************************************
** ��������: __fcLstIfWalk
** ��������: ��ȡ��������ӿ�������Ϣ�ص�
** �䡡��  : fcdev   �����豸���ƿ�
**           pfcelist  ������Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static void  __fcLstIfWalk (struct fc_dev *fcdev, struct fcentry_list *pfcelist)
{
    if (pfcelist->fcl_num < pfcelist->fcl_bcnt) {
        struct fcentry *pfcentry = &pfcelist->fcl_buf[pfcelist->fcl_num];
        ip4_addr_t      ipsaddr, ipeaddr;
        
        ipsaddr.addr = 0;
        ipeaddr.addr = 0;
        
        pfcentry->fc_start.sa_len    = sizeof(struct sockaddr_in);
        pfcentry->fc_start.sa_family = AF_INET;
        pfcentry->fc_end.sa_len      = sizeof(struct sockaddr_in);
        pfcentry->fc_end.sa_family   = AF_INET;
        
        inet_addr_from_ip4addr(&((struct sockaddr_in *)&pfcentry->fc_start)->sin_addr, &ipsaddr);
        inet_addr_from_ip4addr(&((struct sockaddr_in *)&pfcentry->fc_end)->sin_addr,   &ipeaddr);
        
        pfcentry->fc_type   = FCT_IP;
        pfcentry->fc_enable = fcdev->fcnetif.enable;
        pfcentry->fc_proto  = 0;
        pfcentry->fc_sport  = 0;
        pfcentry->fc_eport  = 0;
        
        lib_strlcpy(pfcentry->fc_ifname, fcdev->ifname, IF_NAMESIZE);
        
        pfcentry->fc_uprate   = LWIP_FC_K_TO_U(fcdev->fcnetif.fcblk.r_rate);
        pfcentry->fc_downrate = LWIP_FC_K_TO_U(fcdev->fcnetif.fcblk.s_rate);
        pfcentry->fc_bufsize  = fcdev->fcnetif.fcblk.buf_size;
        pfcelist->fcl_num++;
    }
}
/*********************************************************************************************************
** ��������: __fcLstIf
** ��������: ��ȡ��������ӿ�������Ϣ
** �䡡��  : pfcelist  ������Ϣ����
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __fcLstIf (struct fcentry_list *pfcelist)
{
    UINT    uiTotal;
    
    pfcelist->fcl_num = 0;
    
    FC_LOCK();
    fcnet_total_entry(&uiTotal);
    pfcelist->fcl_total = uiTotal;
    if (!pfcelist->fcl_bcnt || !pfcelist->fcl_buf) {
        FC_UNLOCK();
        return  (ERROR_NONE);
    }
    fcnet_rule_traversal(__fcLstIfWalk, pfcelist, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    FC_UNLOCK();
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __fcIoctlInet
** ��������: SIOCADDRT / SIOCDELRT �����ӿ�
** �䡡��  : iFamily    AF_INET / AF_INET6
**           iCmd       SIOCADDRT / SIOCDELRT
**           pvArg      struct arpreq
** �䡡��  : ERROR or OK
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
INT  __fcIoctlInet (INT  iFamily, INT  iCmd, PVOID  pvArg)
{
    struct fcentry  *pfcentry = (struct fcentry  *)pvArg;
    INT              iRet     = PX_ERROR;

    if (!pfcentry) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    switch (iCmd) {
    
    case SIOCADDFC:                                                     /*  ���һ����������            */
        if (pfcentry->fc_type == FCT_IP) {
            if (iFamily == AF_INET) {
                iRet = __fcAdd4(pfcentry);
            }
#if LWIP_IPV6
              else {
                iRet = __fcAdd6(pfcentry);
            }
#endif
        } else {
            iRet = __fcAddIf(pfcentry);
        }
        break;
        
    case SIOCDELFC:                                                     /*  ɾ��һ����������            */
        if (pfcentry->fc_type == FCT_IP) {
            if (iFamily == AF_INET) {
                iRet = __fcDel4(pfcentry);
            }
#if LWIP_IPV6
              else {
                iRet = __fcDel6(pfcentry);
            }
#endif
        } else {
            iRet = __fcDelIf(pfcentry);
        }
        break;
    
    case SIOCCHGFC:                                                     /*  �ı�һ����������            */
        if (pfcentry->fc_type == FCT_IP) {
            if (iFamily == AF_INET) {
                iRet = __fcChg4(pfcentry);
            }
#if LWIP_IPV6
              else {
                iRet = __fcChg6(pfcentry);
            }
#endif
        } else {
            iRet = __fcChgIf(pfcentry);
        }
        break;
    
    case SIOCGETFC:                                                     /*  ���һ����������            */
        if (pfcentry->fc_type == FCT_IP) {
            if (iFamily == AF_INET) {
                iRet = __fcGet4(pfcentry);
            }
#if LWIP_IPV6
              else {
                iRet = __fcGet6(pfcentry);
            }
#endif
        } else {
            iRet = __fcGetIf(pfcentry);
        }
        break;
    
    case SIOCLSTFC:                                                     /*  �б�������������            */
        if (pfcentry->fc_type == FCT_IP) {
            if (iFamily == AF_INET) {
                iRet = __fcLst4((struct fcentry_list *)pvArg);
            }
#if LWIP_IPV6
              else {
                iRet = __fcLst6((struct fcentry_list *)pvArg);
            }
#endif
        } else {
            iRet = __fcLstIf((struct fcentry_list *)pvArg);
        }
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        break;
    }
    
    return  (iRet);
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
                                                                        /*  LW_CFG_NET_FLOWCTL_EN > 0   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
