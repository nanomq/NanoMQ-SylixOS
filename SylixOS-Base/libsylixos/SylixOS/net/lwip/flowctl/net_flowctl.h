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

#ifndef __NET_FLOWCTL_H
#define __NET_FLOWCTL_H

#include "ip4_flowctl.h"
#include "ip6_flowctl.h"

/* flow control netif */
struct fc_netif {
  /* common member */
  struct fc_blk fcblk;
  
  /* netif member */
  u_char enable;
};

/* flow control for each device */
struct fc_dev {
  LW_LIST_LINE list;
  char ifname[IF_NAMESIZE];
  struct netif *netif;
  
  /* save old netif funcs */
  netif_input_fn o_input;
  netif_output_fn o_output;
#if LWIP_IPV6
  netif_output_ip6_fn o_output_ip6;
#endif
  
  /* ipv4, ipv6 flow control */
  LW_LIST_LINE_HEADER fcd_list[FC_TYPE_MAX - 1];
  struct fc_netif fcnetif; /* netif flow control */
};

#define NETIF_FCDEV(netif) ((struct fc_dev *)(netif)->flowctl)
#define NETIF_FCDEV_SET(netif, fcdev) (netif->flowctl = (void *)(fcdev))

/* flow control tcpip input */
err_t fcnet_netif_tcpip_input(struct fc_dev *fcdev, struct pbuf *p, struct netif *netif);

/* flow control funcs */
int fcnet_netif_add(const char *ifname);
int fcnet_netif_delete(const char *ifname);
struct fc_dev *fcnet_netif_searh(const char *ifname);

/* netif flow control set parameter */
int fcnet_netif_set(const char *ifname, u_char enable, fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize);
int fcnet_netif_get(const char *ifname, u_char *enable, fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize);
void fcnet_rule_traversal(VOIDFUNCPTR func, void *arg0, void *arg1, 
                          void *arg2, void *arg3, void *arg4, void *arg5);
void fcnet_total_entry(unsigned int *cnt);

/* flow control netif hook funcs */
void fcnet_netif_attach(struct netif *netif);
void fcnet_netif_detach(struct netif *netif);

#endif /* __NET_FLOWCTL_H */
/*
 * end
 */
