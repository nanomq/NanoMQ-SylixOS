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
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/prot/ip.h"
#include "net_flowctl.h"

/* ipv4 flow control netif hook funcs */
extern void fcipv4_netif_attach(struct fc_dev *fcdev);
extern void fcipv4_netif_detach(struct fc_dev *fcdev);

#if LWIP_IPV6
extern void fcipv6_netif_attach(struct fc_dev *fcdev);
extern void fcipv6_netif_detach(struct fc_dev *fcdev);
#endif

/* netif flow control netif table */
static LW_LIST_LINE_HEADER fcdev_list;

/* netif flow control packet recv */
static err_t fcnet_netif_recv_e (struct fc_dev *fcdev, struct pbuf *p)
{
  struct netif *netif = fcdev->netif;
  
  if (fcdev->fcd_list[FC_TYPE_IPV4] || fcdev->fcd_list[FC_TYPE_IPV6]) {
    s16_t ip_hdr_offset;
    
    if (netif->flags & (NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET)) {
      struct eth_hdr *ethhdr;
      u16_t type;
      
      ip_hdr_offset = SIZEOF_ETH_HDR;
      ethhdr = (struct eth_hdr *)p->payload;
      type = ethhdr->type;
      
      if (type == PP_HTONS(ETHTYPE_VLAN)) {
        struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr*)(((char*)ethhdr) + SIZEOF_ETH_HDR);
        
        type = vlan->tpid;
        ip_hdr_offset += SIZEOF_VLAN_HDR;
      }
      
      if (p->len < ip_hdr_offset) {
        return (ERR_BUF);
      }
      
      if ((type == PP_HTONS(ETHTYPE_IP)) && fcdev->fcd_list[FC_TYPE_IPV4]) {
        return (fcipv4_netif_input(p, netif, ip_hdr_offset));
      
#if LWIP_IPV6
      } else if ((type == PP_HTONS(ETHTYPE_IPV6)) && fcdev->fcd_list[FC_TYPE_IPV6]) {
        return (fcipv6_netif_input(p, netif, ip_hdr_offset));
#endif
      }
    
    } else {
      if ((IP_HDR_GET_VERSION(p->payload) == 4) && fcdev->fcd_list[FC_TYPE_IPV4]) {
        return (fcipv4_netif_input(p, netif, 0));
        
#if LWIP_IPV6
      } else if ((IP_HDR_GET_VERSION(p->payload) == 6) && fcdev->fcd_list[FC_TYPE_IPV6]) {
        return (fcipv6_netif_input(p, netif, 0));
#endif
      }
    }
  }
  
  return (fcnet_netif_tcpip_input(fcdev, p, netif));
}

/* netif flow control packet send */
static err_t fcnet_netif_send_e (struct fc_dev *fcdev, struct pbuf *p, ip_addr_t *addr)
{
#if LWIP_IPV6
  if (IP_GET_TYPE(addr) == IPADDR_TYPE_V6) {
    return (fcdev->o_output_ip6(fcdev->netif, p, ip_2_ip6(addr)));
  } else 
#endif
  {
    return (fcdev->o_output(fcdev->netif, p, ip_2_ip4(addr)));
  }
}

/* flow control tcpip input 
 * called in fc locking state, 
 * so we must unlock fc then call input function */
err_t fcnet_netif_tcpip_input (struct fc_dev *fcdev, struct pbuf *p, struct netif *netif)
{
  /* NOTICE: o_input always tcpip_input() it only send a mbox so there is no deadlock possible */
  return (fcdev->o_input(p, netif));
}

/* flow control input 
 * called in fc locking state */
static err_t fcnet_netif_flowctl_input (struct pbuf *p, struct netif *netif)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  err_t err;

  if (fcdev->fcnetif.enable) {
    err = fcblk_input(&fcdev->fcnetif.fcblk, p, fcdev); /* input flow control input queue */
    
  } else {
    err = fcnet_netif_recv_e(fcdev, p); /* directly input */
  }
  
  if (err) {
    pbuf_free(p);
  }
  
  return (ERR_OK);
}

/* flow control call back recv */
static void fcnet_netif_recv (void *priv, struct pbuf *p)
{
  struct fc_dev *fcdev = (struct fc_dev *)priv;
  
  if (fcnet_netif_recv_e(fcdev, p)) {
    pbuf_free(0);
  }
}

/* flow control call back send */
static void fcnet_netif_send (void *priv, struct pbuf *p, ip_addr_t *addr)
{
  fcnet_netif_send_e((struct fc_dev *)priv, p, addr);
}

/* flow control input 
 * called by netdev driver 
 * use flow control thread input */
static err_t fcnet_netif_input (struct pbuf *p, struct netif *netif)
{
  return (fcblk_inpkt(p, netif, fcnet_netif_flowctl_input));
}

/* flow control output 
 * called by tcpip stack */
err_t fcnet_netif_output (struct netif *netif, struct pbuf *p,
                          const ip4_addr_t *ipaddr)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  
  if (fcdev->fcnetif.enable) {
    return (fcblk_output(&fcdev->fcnetif.fcblk, p, fcdev, ipaddr)); /* output flow control output queue */
  
  } else {
    return (fcdev->o_output(fcdev->netif, p, ipaddr)); /* directly output */
  }
}

#if LWIP_IPV6
/* flow control output_ip6 
 * called by tcpip stack */
err_t fcnet_netif_output_ip6 (struct netif *netif, struct pbuf *p,
                              const ip6_addr_t *ipaddr)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  
  if (fcdev->fcnetif.enable) {
    return (fcblk_output_ip6(&fcdev->fcnetif.fcblk, p, fcdev, ipaddr)); /* output flow control output queue */
  
  } else {
    return (fcdev->o_output_ip6(fcdev->netif, p, ipaddr)); /* directly output */
  }
}
#endif /* LWIP_IPV6 */

/* flow control attach to netif no lock */
static void fcnet_netif_attach_nolock (struct netif *netif)
{
  static const struct fc_funcs fcnet_funcs = {
    fcnet_netif_send,
    fcnet_netif_recv
  };

  struct fc_dev *fcdev;
  char ifname[IF_NAMESIZE];
  PLW_LIST_LINE pline;
  
  netif_get_name(netif, ifname);
  
  for (pline = fcdev_list; pline != NULL; pline = _list_line_get_next(pline)) {
    fcdev = (struct fc_dev *)pline;
    if (!fcdev->netif && !lib_strcmp(fcdev->ifname, ifname)) {
      fcdev->o_input = netif->input;
      fcdev->o_output = netif->output;
#if LWIP_IPV6
      fcdev->o_output_ip6 = netif->output_ip6;
#endif
      netif->input = fcnet_netif_input;
      netif->output = fcipv4_netif_output;
#if LWIP_IPV6
      netif->output_ip6 = fcipv6_netif_output;
#endif
      NETIF_FCDEV_SET(netif, fcdev);
      fcdev->netif = netif;
      fcdev->fcnetif.fcblk.funcs = &fcnet_funcs;
      
      fcblk_start(&fcdev->fcnetif.fcblk);
      fcipv4_netif_attach(fcdev);
#if LWIP_IPV6
      fcipv6_netif_attach(fcdev);
#endif
      break;
    }
  }
}

/* flow control attach to netif (call back function) */
void fcnet_netif_attach (struct netif *netif)
{
  FC_LOCK();
  fcnet_netif_attach_nolock(netif);
  FC_UNLOCK();
}

/* flow control detach from netif (call back function) */
static void fcnet_netif_detach_nolock (struct netif *netif)
{
  struct fc_dev *fcdev = NETIF_FCDEV(netif);
  
  if (fcdev) {
    netif->input = fcdev->o_input;
    netif->output = fcdev->o_output;
#if LWIP_IPV6
    netif->output_ip6 = fcdev->o_output_ip6;
#endif
    NETIF_FCDEV_SET(netif, NULL);
    fcdev->netif = NULL;
    
    fcipv4_netif_detach(fcdev);
#if LWIP_IPV6
    fcipv6_netif_detach(fcdev);
#endif
    fcblk_stop(&fcdev->fcnetif.fcblk, 1);
  }
}

/* flow control detach from netif (call back function) */
void fcnet_netif_detach (struct netif *netif)
{
  FC_LOCK();
  fcnet_netif_detach_nolock(netif);
  FC_UNLOCK();
}

/* netif flow control enable */
int fcnet_netif_add (const char *ifname)
{
  struct fc_dev *fcdev;
  struct netif *netif;
  
  netif = netif_find(ifname);
  if (netif && NETIF_FCDEV(netif)) {
    return (0); /* already have flow control */
  }
  
  fcdev = (struct fc_dev *)mem_malloc(sizeof(struct fc_dev));
  if (fcdev == NULL) {
    return (-1);
  }
  lib_bzero(fcdev, sizeof(struct fc_dev));
  
  fcdev->fcnetif.fcblk.type = FC_TYPE_NETIF;
  lib_strlcpy(fcdev->ifname, ifname, IF_NAMESIZE);
  
  _List_Line_Add_Ahead(&fcdev->list, &fcdev_list);
  if (netif) {
    fcnet_netif_attach_nolock(netif);
  }
  
  return (0);
}

/* netif flow control disable */
int fcnet_netif_delete (const char *ifname)
{
  struct fc_dev *fcdev;
  struct netif *netif;
  PLW_LIST_LINE pline;
  
  for (pline = fcdev_list; pline != NULL; pline = _list_line_get_next(pline)) {
    fcdev = (struct fc_dev *)pline;
    if (!lib_strcmp(fcdev->ifname, ifname)) {
      break;
    }
  }
  
  if (pline != NULL) {
    fcipv4_rule_delif(fcdev);
#if LWIP_IPV6
    fcipv6_rule_delif(fcdev);
#endif
    _List_Line_Del(&fcdev->list, &fcdev_list);
    
    netif = fcdev->netif;
    if (netif) {
      fcnet_netif_detach_nolock(netif);
    }
    mem_free(fcdev);
    return (0);
  }
  
  return (-1);
}

/* netif flow control enable */
struct fc_dev *fcnet_netif_searh (const char *ifname)
{
  struct fc_dev *fcdev;
  PLW_LIST_LINE pline;
  
  for (pline = fcdev_list; pline != NULL; pline = _list_line_get_next(pline)) {
    fcdev = (struct fc_dev *)pline;
    if (!lib_strcmp(fcdev->ifname, ifname)) {
      break;
    }
  }
  
  if (pline != NULL) {
    return (fcdev);
  }
  
  return (NULL);
}

/* netif flow control set parameter */
int fcnet_netif_set (const char *ifname, u_char enable, fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize)
{
  struct fc_dev *fcdev = fcnet_netif_searh(ifname);
  
  if (fcdev == NULL) {
    return (-1);
  }
  
  fcdev->fcnetif.enable = enable;
  fcdev->fcnetif.fcblk.s_rate = s_rate;
  fcdev->fcnetif.fcblk.r_rate = r_rate;
  fcdev->fcnetif.fcblk.buf_size = bsize;

  return (0);
}

/* netif flow control get parameter */
int fcnet_netif_get (const char *ifname, u_char *enable, fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize)
{
  struct fc_dev *fcdev = fcnet_netif_searh(ifname);
  
  if (fcdev == NULL) {
    return (-1);
  }
  
  if (enable) {
    *enable = fcdev->fcnetif.enable;
  }
  if (s_rate) {
    *s_rate = fcdev->fcnetif.fcblk.s_rate;
  }
  if (r_rate) {
    *r_rate = fcdev->fcnetif.fcblk.r_rate;
  }
  if (bsize) {
    *bsize = fcdev->fcnetif.fcblk.buf_size;
  }

  return (0);
}

/* netif flow control walk */
void fcnet_rule_traversal (VOIDFUNCPTR func, void *arg0, void *arg1, 
                           void *arg2, void *arg3, void *arg4, void *arg5)
{
  struct fc_dev *fcdev;
  PLW_LIST_LINE pline;
  
  for (pline = fcdev_list; pline != NULL; pline = _list_line_get_next(pline)) {
    fcdev = (struct fc_dev *)pline;
    func(fcdev, arg0, arg1, arg2, arg3, arg4, arg5);
  }
}

/* fcnet flow control cnt */
static void fcnet_counter (struct fc_dev *fcdev, int *cnt)
{
  (*cnt) += 1;
}

/* fcnet flow control total num */
void fcnet_total_entry (unsigned int *cnt)
{
  int count = 0;
  
  fcnet_rule_traversal(fcnet_counter, &count, NULL, NULL, NULL, NULL, NULL);
  if (cnt) {
    *cnt = count;
  }
}

#endif /* LW_CFG_NET_FLOWCTL_EN > 0 */
/*
 * end
 */
