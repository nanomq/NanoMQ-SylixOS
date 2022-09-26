/**
 * @file
 * net flow control
 * as much as possible compatible with different versions of LwIP
 * Verification using sylixos(tm) real-time operating system
 */

/*
 * Copyright (c) 2006-2017 SylixOS Group.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 * 4. This code has been or is applying for intellectual property protection 
 *    and can only be used with acoinfo software products.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 * 
 * Author: Han.hui <hanhui@acoinfo.com>
 *
 */

#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#if LW_CFG_NET_FLOWCTL_EN > 0

#include "net/if.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "net_flowctl.h"

#if LWIP_IPV6

/* netif flow control funcs */
extern err_t fcnet_netif_output_ip6(struct netif *netif, struct pbuf *p,
                                    const ip6_addr_t *ipaddr);

/* ipv6 address cmp */
static int fcipv6_addr_cmp (const ip6_addr_t *ipaddr1_hbo, const ip6_addr_t *ipaddr2_hbo)
{
  if (ipaddr1_hbo->addr[0] < ipaddr2_hbo->addr[0]) {
    return (-1);
  } else if (ipaddr1_hbo->addr[0] > ipaddr2_hbo->addr[0]) {
    return (1);
  } else {
    if (ipaddr1_hbo->addr[1] < ipaddr2_hbo->addr[1]) {
      return (-1);
    } else if (ipaddr1_hbo->addr[1] > ipaddr2_hbo->addr[1]) {
      return (1);
    } else {
      if (ipaddr1_hbo->addr[2] < ipaddr2_hbo->addr[2]) {
        return (-1);
      } else if (ipaddr1_hbo->addr[2] > ipaddr2_hbo->addr[2]) {
        return (1);
      } else {
        if (ipaddr1_hbo->addr[3] < ipaddr2_hbo->addr[3]) {
          return (-1);
        } else if (ipaddr1_hbo->addr[3] > ipaddr2_hbo->addr[3]) {
          return (1);
        } else {
          return (0);
        }
      }
    }
  }
}

/* ipv6 flow control match */
static struct fc_blk *fcipv6_match (struct fc_dev *fcdev, struct pbuf *p, int send, s16_t ip_hdr_offset)
{
  u8_t *hdr;
  int hlen, nexth;
  u16_t ip6hdr_len;
  u8_t proto;
  struct ip6_hdr *ip6hdr;
  struct tcp_hdr *tcphdr;
  struct udp_hdr *udphdr;
  struct fc_ipv6 *fcipv6;
  ip6_addr_t ip6addr_hbo;
  u16_t port_hbo;
  PLW_LIST_LINE pline;
  
  ip6hdr = (struct ip6_hdr *)((char *)p->payload + ip_hdr_offset);
  if (p->len < (ip_hdr_offset + IP6_HLEN)) {
    return (NULL);
  }
  
  if (send) {
    fcipv6_addr_ntoh(&ip6addr_hbo, &ip6hdr->dest);

  } else {
    fcipv6_addr_ntoh(&ip6addr_hbo, &ip6hdr->src);
  }
  
  ip6hdr_len = IP6_HLEN;
  hdr = (u8_t *)ip6hdr + IP6_HLEN;
  nexth = IP6H_NEXTH(ip6hdr);
  
  while (nexth != IP6_NEXTH_NONE) {
    if (p->len < (ip_hdr_offset + ip6hdr_len)) {
      return (NULL);
    }
    switch (nexth) {
    
    case IP6_NEXTH_HOPBYHOP:
    case IP6_NEXTH_DESTOPTS:
    case IP6_NEXTH_ROUTING:
      nexth = *hdr;
      hlen = 8 * (1 + *(hdr + 1));
      ip6hdr_len += hlen;
      hdr += hlen;
      break;
      
    case IP6_NEXTH_FRAGMENT:
      nexth = *hdr;
      hlen = 8;
      ip6hdr_len += hlen;
      hdr += hlen;
      break;
      
    default:
      goto __out;
      break;
    }
  }
  
__out:
  proto = nexth;
  switch (proto) {
  
  case IP_PROTO_UDP:
  case IP_PROTO_UDPLITE:
    proto = IP_PROTO_UDP;
    udphdr = (struct udp_hdr *)((char *)p->payload + ip_hdr_offset + ip6hdr_len);
    if (p->len < (ip_hdr_offset + ip6hdr_len + UDP_HLEN)) {
      return (NULL);
    }
    port_hbo = (send) ? PP_NTOHS(udphdr->src) : PP_NTOHS(udphdr->dest);
    break;
    
  case IP_PROTO_TCP:
    tcphdr = (struct tcp_hdr *)((char *)p->payload + ip_hdr_offset + ip6hdr_len);
    if (p->len < (ip_hdr_offset + ip6hdr_len + TCP_HLEN)) {
      return (NULL);
    }
    port_hbo = (send) ? PP_NTOHS(tcphdr->src) : PP_NTOHS(tcphdr->dest);
    break;
    
  default:
    proto = 0;
    port_hbo = 0;
    break;
  }
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    if (!fcipv6->enable) {
      continue;
    }
    if ((fcipv6_addr_cmp(&ip6addr_hbo, &fcipv6->start_hbo) >= 0) &&
        (fcipv6_addr_cmp(&ip6addr_hbo, &fcipv6->end_hbo) <= 0)) {
      if (fcipv6->proto == 0) {
        return (&fcipv6->fcblk);
      
      } else if (fcipv6->proto == proto) {
        if ((proto == IP_PROTO_UDP) || (proto == IP_PROTO_TCP)) {
          if ((port_hbo >= fcipv6->s_port_hbo) && (port_hbo <= fcipv6->e_port_hbo)) {
            return (&fcipv6->fcblk);
          }
        
        } else {
          return (&fcipv6->fcblk);
        }
      }
    }
  }
  
  return (NULL);
}

/* ipv6 flow control send */
static void fcipv6_netif_send (void *priv, struct pbuf *p, ip_addr_t *addr)
{
  struct fc_dev *fcdev = (struct fc_dev *)priv;

  fcnet_netif_output_ip6(fcdev->netif, p, ip_2_ip6(addr)); /* send to fcnet layer */
}

/* ipv6 flow control recv */
static void fcipv6_netif_recv (void *priv, struct pbuf *p)
{
  struct fc_dev *fcdev = (struct fc_dev *)priv;
  
  if (fcnet_netif_tcpip_input(fcdev, p, fcdev->netif)) {
    pbuf_free(p);
  }
}

/* ipv6 netif flow control output */
err_t fcipv6_netif_output (struct netif *netif, struct pbuf *p,
                           const ip6_addr_t *ipaddr)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  struct fc_blk *fcblk;
  
  fcblk = fcipv6_match(fcdev, p, 1, 0);
  if (fcblk) {
    return (fcblk_output_ip6(fcblk, p, fcdev, ipaddr)); /* output flow control output queue */
  
  } else {
    return (fcnet_netif_output_ip6(netif, p, ipaddr)); /* send to fcnet layer */
  }
}

/* ipv6 flow control input */
err_t fcipv6_netif_input (struct pbuf *p, struct netif *netif, s16_t ip_hdr_offset)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  struct fc_blk *fcblk;

  fcblk = fcipv6_match(fcdev, p, 0, ip_hdr_offset);
  if (fcblk) {
    return (fcblk_input(fcblk, p, fcdev)); /* input flow control input queue */
  
  } else {
    return (fcnet_netif_tcpip_input(fcdev, p, netif));
  }
}

/* ipv6 flow control create */
int fcipv6_rule_create (const char *ifname, const ip6_addr_t *start, const ip6_addr_t *end, 
                        u_char proto, u_short s_port, u_short e_port, u_char enable,
                        fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize)
{
  static const struct fc_funcs fcipv6_funcs = {
    fcipv6_netif_send,
    fcipv6_netif_recv
  };
  
  struct fc_dev *fcdev;
  struct fc_ipv6 *fcipv6;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcipv6 = (struct fc_ipv6 *)mem_malloc(sizeof(struct fc_ipv6));
  if (fcipv6 == NULL) {
    return (-2);
  }
  lib_bzero(fcipv6, sizeof(struct fc_ipv6));
  
  fcipv6->enable = enable;
  fcipv6_addr_ntoh(&fcipv6->start_hbo, start);
  fcipv6_addr_ntoh(&fcipv6->end_hbo, end);
  fcipv6->proto = proto;
  fcipv6->s_port_hbo = PP_NTOHS(s_port);
  fcipv6->e_port_hbo = PP_NTOHS(e_port);
  
  fcipv6->fcblk.type = FC_TYPE_IPV6;
  fcipv6->fcblk.s_rate = s_rate;
  fcipv6->fcblk.r_rate = r_rate;
  fcipv6->fcblk.buf_size = bsize;
  fcipv6->fcblk.funcs = &fcipv6_funcs;
  
  _List_Line_Add_Ahead(&fcipv6->list, &fcdev->fcd_list[FC_TYPE_IPV6]);
  if (fcdev->netif) {
    fcblk_start(&fcipv6->fcblk);
  }
  
  return (0);
}

/* ipv6 flow control delete */
int fcipv6_rule_delete (const char *ifname, const ip6_addr_t *start, const ip6_addr_t *end, 
                        u_char proto, u_short s_port, u_short e_port)
{
  struct fc_dev *fcdev;
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  ip6_addr_t start_hbo, end_hbo;
  u_short s_port_hbo, e_port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcipv6_addr_ntoh(&start_hbo, start);
  fcipv6_addr_ntoh(&end_hbo, end);
  s_port_hbo = PP_NTOHS(s_port);
  e_port_hbo = PP_NTOHS(e_port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    if (!fcipv6_addr_cmp(&fcipv6->start_hbo, &start_hbo) &&
        !fcipv6_addr_cmp(&fcipv6->end_hbo, &end_hbo) && (fcipv6->proto == proto) &&
        (fcipv6->s_port_hbo == s_port_hbo) && (fcipv6->e_port_hbo == e_port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    _List_Line_Del(&fcipv6->list, &fcdev->fcd_list[FC_TYPE_IPV6]);
    if (fcdev->netif) {
      fcblk_stop(&fcipv6->fcblk, 0);
    }
    mem_free(fcipv6);
    return (0);
  }
  
  return (-1);
}

/* ipv6 flow control change */
int fcipv6_rule_change (const char *ifname, const ip6_addr_t *start, const ip6_addr_t *end, 
                        u_char proto, u_short s_port, u_short e_port, u_char enable,
                        fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize)
{
  struct fc_dev *fcdev;
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  ip6_addr_t start_hbo, end_hbo;
  u_short s_port_hbo, e_port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcipv6_addr_ntoh(&start_hbo, start);
  fcipv6_addr_ntoh(&end_hbo, end);
  s_port_hbo = PP_NTOHS(s_port);
  e_port_hbo = PP_NTOHS(e_port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    if (!fcipv6_addr_cmp(&fcipv6->start_hbo, &start_hbo) &&
        !fcipv6_addr_cmp(&fcipv6->end_hbo, &end_hbo) && (fcipv6->proto == proto) &&
        (fcipv6->s_port_hbo == s_port_hbo) && (fcipv6->e_port_hbo == e_port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    fcipv6->enable = enable;
    fcipv6->fcblk.s_rate = s_rate;
    fcipv6->fcblk.r_rate = r_rate;
    fcipv6->fcblk.buf_size = bsize;
    return (0);
  }
  
  return (-1);
}

/* ipv6 flow control get */
int fcipv6_rule_get (const char *ifname, const ip6_addr_t *start, const ip6_addr_t *end, 
                     u_char proto, u_short s_port, u_short e_port, u_char *enable,
                     fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize)
{
  struct fc_dev *fcdev;
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  ip6_addr_t start_hbo, end_hbo;
  u_short s_port_hbo, e_port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcipv6_addr_ntoh(&start_hbo, start);
  fcipv6_addr_ntoh(&end_hbo, end);
  s_port_hbo = PP_NTOHS(s_port);
  e_port_hbo = PP_NTOHS(e_port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    if (!fcipv6_addr_cmp(&fcipv6->start_hbo, &start_hbo) &&
        !fcipv6_addr_cmp(&fcipv6->end_hbo, &end_hbo) && (fcipv6->proto == proto) &&
        (fcipv6->s_port_hbo == s_port_hbo) && (fcipv6->e_port_hbo == e_port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    if (enable) {
      *enable = fcipv6->enable;
    }
    if (s_rate) {
      *s_rate = fcipv6->fcblk.s_rate;
    }
    if (r_rate) {
      *r_rate = fcipv6->fcblk.r_rate;
    }
    if (bsize) {
      *bsize = fcipv6->fcblk.buf_size;
    }
    return (0);
  }
  
  return (-1);
}

/* ipv6 flow control search */
int fcipv6_rule_search (const char *ifname, const ip6_addr_t *ip6addr, 
                        u_char proto, u_short port, u_char *enable,
                        fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize)
{
  struct fc_dev *fcdev;
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  ip6_addr_t ip6addr_hbo;
  u_short port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcipv6_addr_ntoh(&ip6addr_hbo, ip6addr);
  port_hbo = PP_NTOHS(port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    if ((fcipv6_addr_cmp(&fcipv6->start_hbo, &ip6addr_hbo) <= 0)  &&
        (fcipv6_addr_cmp(&fcipv6->end_hbo, &ip6addr_hbo) >= 0) && 
        (fcipv6->proto == proto) &&
        (fcipv6->s_port_hbo <= port_hbo) && (fcipv6->e_port_hbo >= port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    if (enable) {
      *enable = fcipv6->enable;
    }
    if (s_rate) {
      *s_rate = fcipv6->fcblk.s_rate;
    }
    if (r_rate) {
      *r_rate = fcipv6->fcblk.r_rate;
    }
    if (bsize) {
      *bsize = fcipv6->fcblk.buf_size;
    }
    return (0);
  }
  
  return (-1);
}

/* ipv6 flow control delete */
void fcipv6_rule_delif (struct fc_dev *fcdev)
{
  struct fc_ipv6 *fcipv6;
  
  while (fcdev->fcd_list[FC_TYPE_IPV6]) {
    fcipv6 = _LIST_ENTRY(fcdev->fcd_list[FC_TYPE_IPV6], struct fc_ipv6, list);
    _List_Line_Del(&fcipv6->list, &fcdev->fcd_list[FC_TYPE_IPV6]);
    if (fcdev->netif) {
      fcblk_stop(&fcipv6->fcblk, 0);
    }
    mem_free(fcipv6);
  }
}

/* ipv6 flow control walk call back */
static void fcipv6_rule_traversal_net (struct fc_dev *fcdev, 
                                       VOIDFUNCPTR func, void *arg0, void *arg1, 
                                       void *arg2, void *arg3, void *arg4)
{
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    func(fcdev, fcipv6, arg0, arg1, arg2, arg3, arg4);
  }
}

/* ipv6 flow control walk */
void fcipv6_rule_traversal (VOIDFUNCPTR func, void *arg0, void *arg1, 
                            void *arg2, void *arg3, void *arg4)
{
  fcnet_rule_traversal(fcipv6_rule_traversal_net, (void *)func, arg0, arg1, arg2, arg3, arg4);
}

/* ipv6 flow control cnt */
static void fcipv6_counter (struct fc_dev *fcdev, struct fc_ipv6 *fcipv6, int *cnt)
{
  (*cnt) += 1;
}

/* ipv6 flow control total num */
void fcipv6_total_entry (unsigned int *cnt)
{
  int count = 0;
  
  fcipv6_rule_traversal(fcipv6_counter, &count, NULL, NULL, NULL, NULL);
  if (cnt) {
    *cnt = count;
  }
}

/* ipv6 flow control netif attach */
void fcipv6_netif_attach (struct fc_dev *fcdev)
{
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    fcblk_start(&fcipv6->fcblk);
  }
}

/* ipv6 flow control netif detach */
void fcipv6_netif_detach (struct fc_dev *fcdev)
{
  struct fc_ipv6 *fcipv6;
  PLW_LIST_LINE pline;
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV6];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv6 = _LIST_ENTRY(pline, struct fc_ipv6, list);
    fcblk_stop(&fcipv6->fcblk, 1);
  }
}

#endif /* LWIP_IPV6 */
#endif /* LW_CFG_NET_FLOWCTL_EN > 0 */
/*
 * end
 */
