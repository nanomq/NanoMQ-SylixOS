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

/* netif flow control funcs */
extern err_t fcnet_netif_output(struct netif *netif, struct pbuf *p,
                                const ip4_addr_t *ipaddr);

/* ipv4 flow control match */
static struct fc_blk *fcipv4_match (struct fc_dev *fcdev, struct pbuf *p, int send, s16_t ip_hdr_offset)
{
  u16_t iphdr_len;
  u8_t proto;
  struct ip_hdr *iphdr;
  struct tcp_hdr *tcphdr;
  struct udp_hdr *udphdr;
  struct fc_ipv4 *fcipv4;
  ip4_addr_t ipaddr_hbo;
  u16_t port_hbo;
  PLW_LIST_LINE pline;
  
  iphdr = (struct ip_hdr *)((char *)p->payload + ip_hdr_offset);
  iphdr_len = (u16_t)(IPH_HL(iphdr) << 2);
  if (p->len < (ip_hdr_offset + iphdr_len)) {
    return (NULL);
  }
  
  ipaddr_hbo.addr = (send) ? PP_NTOHL(iphdr->dest.addr) : PP_NTOHL(iphdr->src.addr);
  
  proto = IPH_PROTO(iphdr);
  switch (proto) {
  
  case IP_PROTO_UDP:
  case IP_PROTO_UDPLITE:
    proto = IP_PROTO_UDP;
    udphdr = (struct udp_hdr *)((char *)p->payload + ip_hdr_offset + iphdr_len);
    if (p->len < (ip_hdr_offset + iphdr_len + UDP_HLEN)) {
      return (NULL);
    }
    port_hbo = (send) ? PP_NTOHS(udphdr->src) : PP_NTOHS(udphdr->dest);
    break;
    
  case IP_PROTO_TCP:
    tcphdr = (struct tcp_hdr *)((char *)p->payload + ip_hdr_offset + iphdr_len);
    if (p->len < (ip_hdr_offset + iphdr_len + TCP_HLEN)) {
      return (NULL);
    }
    port_hbo = (send) ? PP_NTOHS(tcphdr->src) : PP_NTOHS(tcphdr->dest);
    break;
    
  default:
    proto = 0;
    port_hbo = 0;
    break;
  }
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    if (!fcipv4->enable) {
      continue;
    }
    if ((ipaddr_hbo.addr >= fcipv4->start_hbo.addr) && 
        (ipaddr_hbo.addr <= fcipv4->end_hbo.addr)) {
      if (fcipv4->proto == 0) {
        return (&fcipv4->fcblk);
      
      } else if (fcipv4->proto == proto) {
        if ((proto == IP_PROTO_UDP) || (proto == IP_PROTO_TCP)) {
          if ((port_hbo >= fcipv4->s_port_hbo) && (port_hbo <= fcipv4->e_port_hbo)) {
            return (&fcipv4->fcblk);
          }
        
        } else {
          return (&fcipv4->fcblk);
        }
      }
    }
  }
  
  return (NULL);
}

/* ipv4 flow control send */
static void fcipv4_netif_send (void *priv, struct pbuf *p, ip_addr_t *addr)
{
  struct fc_dev *fcdev = (struct fc_dev *)priv;

  fcnet_netif_output(fcdev->netif, p, ip_2_ip4(addr)); /* send to fcnet layer */
}

/* ipv4 flow control recv */
static void fcipv4_netif_recv (void *priv, struct pbuf *p)
{
  struct fc_dev *fcdev = (struct fc_dev *)priv;
  
  if (fcnet_netif_tcpip_input(fcdev, p, fcdev->netif)) {
    pbuf_free(p);
  }
}

/* ipv4 netif flow control output */
err_t fcipv4_netif_output (struct netif *netif, struct pbuf *p,
                           const ip4_addr_t *ipaddr)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  struct fc_blk *fcblk;
  
  fcblk = fcipv4_match(fcdev, p, 1, 0);
  if (fcblk) {
    return (fcblk_output(fcblk, p, fcdev, ipaddr)); /* output flow control output queue */
  
  } else {
    return (fcnet_netif_output(netif, p, ipaddr)); /* send to fcnet layer */
  }
}

/* ipv4 flow control input */
err_t fcipv4_netif_input (struct pbuf *p, struct netif *netif, s16_t ip_hdr_offset)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  struct fc_blk *fcblk;

  fcblk = fcipv4_match(fcdev, p, 0, ip_hdr_offset);
  if (fcblk) {
    return (fcblk_input(fcblk, p, fcdev)); /* input flow control input queue */
  
  } else {
    return (fcnet_netif_tcpip_input(fcdev, p, netif));
  }
}

/* ipv4 flow control create */
int fcipv4_rule_create (const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                        u_char proto, u_short s_port, u_short e_port, u_char enable,
                        fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize)
{
  static const struct fc_funcs fcipv4_funcs = {
    fcipv4_netif_send,
    fcipv4_netif_recv
  };
  
  struct fc_dev *fcdev;
  struct fc_ipv4 *fcipv4;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcipv4 = (struct fc_ipv4 *)mem_malloc(sizeof(struct fc_ipv4));
  if (fcipv4 == NULL) {
    return (-2);
  }
  lib_bzero(fcipv4, sizeof(struct fc_ipv4));
  
  fcipv4->enable = enable;
  fcipv4->start_hbo.addr = PP_NTOHL(start->addr);
  fcipv4->end_hbo.addr = PP_NTOHL(end->addr);
  fcipv4->proto = proto;
  fcipv4->s_port_hbo = PP_NTOHS(s_port);
  fcipv4->e_port_hbo = PP_NTOHS(e_port);
  
  fcipv4->fcblk.type = FC_TYPE_IPV4;
  fcipv4->fcblk.s_rate = s_rate;
  fcipv4->fcblk.r_rate = r_rate;
  fcipv4->fcblk.buf_size = bsize;
  fcipv4->fcblk.funcs = &fcipv4_funcs;
  
  _List_Line_Add_Ahead(&fcipv4->list, &fcdev->fcd_list[FC_TYPE_IPV4]);
  if (fcdev->netif) {
    fcblk_start(&fcipv4->fcblk);
  }
  
  return (0);
}

/* ipv4 flow control delete */
int fcipv4_rule_delete (const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                        u_char proto, u_short s_port, u_short e_port)
{
  struct fc_dev *fcdev;
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  ip4_addr_t start_hbo, end_hbo;
  u_short s_port_hbo, e_port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  start_hbo.addr = PP_NTOHL(start->addr);
  end_hbo.addr = PP_NTOHL(end->addr);
  s_port_hbo = PP_NTOHS(s_port);
  e_port_hbo = PP_NTOHS(e_port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    if ((fcipv4->start_hbo.addr == start_hbo.addr) &&
        (fcipv4->end_hbo.addr == end_hbo.addr) && (fcipv4->proto == proto) &&
        (fcipv4->s_port_hbo == s_port_hbo) && (fcipv4->e_port_hbo == e_port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    _List_Line_Del(&fcipv4->list, &fcdev->fcd_list[FC_TYPE_IPV4]);
    if (fcdev->netif) {
      fcblk_stop(&fcipv4->fcblk, 0);
    }
    mem_free(fcipv4);
    return (0);
  }
  
  return (-1);
}

/* ipv4 flow control change */
int fcipv4_rule_change (const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                        u_char proto, u_short s_port, u_short e_port, u_char enable,
                        fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize)
{
  struct fc_dev *fcdev;
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  ip4_addr_t start_hbo, end_hbo;
  u_short s_port_hbo, e_port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  start_hbo.addr = PP_NTOHL(start->addr);
  end_hbo.addr = PP_NTOHL(end->addr);
  s_port_hbo = PP_NTOHS(s_port);
  e_port_hbo = PP_NTOHS(e_port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    if ((fcipv4->start_hbo.addr == start_hbo.addr) &&
        (fcipv4->end_hbo.addr == end_hbo.addr) && (fcipv4->proto == proto) &&
        (fcipv4->s_port_hbo == s_port_hbo) && (fcipv4->e_port_hbo == e_port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    fcipv4->enable = enable;
    fcipv4->fcblk.s_rate = s_rate;
    fcipv4->fcblk.r_rate = r_rate;
    fcipv4->fcblk.buf_size = bsize;
    return (0);
  }
  
  return (-1);
}

/* ipv4 flow control get */
int fcipv4_rule_get (const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                     u_char proto, u_short s_port, u_short e_port, u_char *enable,
                     fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize)
{
  struct fc_dev *fcdev;
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  ip4_addr_t start_hbo, end_hbo;
  u_short s_port_hbo, e_port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  start_hbo.addr = PP_NTOHL(start->addr);
  end_hbo.addr = PP_NTOHL(end->addr);
  s_port_hbo = PP_NTOHS(s_port);
  e_port_hbo = PP_NTOHS(e_port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    if ((fcipv4->start_hbo.addr == start_hbo.addr) &&
        (fcipv4->end_hbo.addr == end_hbo.addr) && (fcipv4->proto == proto) &&
        (fcipv4->s_port_hbo == s_port_hbo) && (fcipv4->e_port_hbo == e_port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    if (enable) {
      *enable = fcipv4->enable;
    }
    if (s_rate) {
      *s_rate = fcipv4->fcblk.s_rate;
    }
    if (r_rate) {
      *r_rate = fcipv4->fcblk.r_rate;
    }
    if (bsize) {
      *bsize = fcipv4->fcblk.buf_size;
    }
    return (0);
  }
  
  return (-1);
}

/* ipv4 flow control search */
int fcipv4_rule_search (const char *ifname, const ip4_addr_t *ipaddr, 
                        u_char proto, u_short port, u_char *enable,
                        fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize)
{
  struct fc_dev *fcdev;
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  ip4_addr_t ipaddr_hbo;
  u_short port_hbo;
  
  fcdev = fcnet_netif_searh(ifname);
  if (fcdev == NULL) {
    return (-1);
  }
  
  ipaddr_hbo.addr = PP_NTOHL(ipaddr->addr);
  port_hbo = PP_NTOHS(port);
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    if ((fcipv4->start_hbo.addr <= ipaddr_hbo.addr) &&
        (fcipv4->end_hbo.addr >= ipaddr_hbo.addr) && (fcipv4->proto == proto) &&
        (fcipv4->s_port_hbo <= port_hbo) && (fcipv4->e_port_hbo >= port_hbo)) {
      break;
    }
  }
  
  if (pline) {
    if (enable) {
      *enable = fcipv4->enable;
    }
    if (s_rate) {
      *s_rate = fcipv4->fcblk.s_rate;
    }
    if (r_rate) {
      *r_rate = fcipv4->fcblk.r_rate;
    }
    if (bsize) {
      *bsize = fcipv4->fcblk.buf_size;
    }
    return (0);
  }
  
  return (-1);
}

/* ipv4 flow control delete all in netif */
void fcipv4_rule_delif (struct fc_dev *fcdev)
{
  struct fc_ipv4 *fcipv4;
  
  while (fcdev->fcd_list[FC_TYPE_IPV4]) {
    fcipv4 = _LIST_ENTRY(fcdev->fcd_list[FC_TYPE_IPV4], struct fc_ipv4, list);
    _List_Line_Del(&fcipv4->list, &fcdev->fcd_list[FC_TYPE_IPV4]);
    if (fcdev->netif) {
      fcblk_stop(&fcipv4->fcblk, 0);
    }
    mem_free(fcipv4);
  }
}

/* ipv4 flow control walk call back */
static void fcipv4_rule_traversal_net (struct fc_dev *fcdev, 
                                       VOIDFUNCPTR func, void *arg0, void *arg1, 
                                       void *arg2, void *arg3, void *arg4)
{
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    func(fcdev, fcipv4, arg0, arg1, arg2, arg3, arg4);
  }
}

/* ipv4 flow control walk */
void fcipv4_rule_traversal (VOIDFUNCPTR func, void *arg0, void *arg1, 
                            void *arg2, void *arg3, void *arg4)
{
  fcnet_rule_traversal(fcipv4_rule_traversal_net, (void *)func, arg0, arg1, arg2, arg3, arg4);
}

/* ipv4 flow control cnt */
static void fcipv4_counter (struct fc_dev *fcdev, struct fc_ipv4 *fcipv4, int *cnt)
{
  (*cnt) += 1;
}

/* ipv4 flow control total num */
void fcipv4_total_entry (unsigned int *cnt)
{
  int count = 0;
  
  fcipv4_rule_traversal(fcipv4_counter, &count, NULL, NULL, NULL, NULL);
  if (cnt) {
    *cnt = count;
  }
}

/* ipv4 flow control netif attach */
void fcipv4_netif_attach (struct fc_dev *fcdev)
{
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    fcblk_start(&fcipv4->fcblk);
  }
}

/* ipv4 flow control netif detach */
void fcipv4_netif_detach (struct fc_dev *fcdev)
{
  struct fc_ipv4 *fcipv4;
  PLW_LIST_LINE pline;
  
  for (pline  = fcdev->fcd_list[FC_TYPE_IPV4];
       pline != NULL;
       pline  = _list_line_get_next(pline)) {
    fcipv4 = _LIST_ENTRY(pline, struct fc_ipv4, list);
    fcblk_stop(&fcipv4->fcblk, 1);
  }
}

#endif /* LW_CFG_NET_FLOWCTL_EN > 0 */
/*
 * end
 */
