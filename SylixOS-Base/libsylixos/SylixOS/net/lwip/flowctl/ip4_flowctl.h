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

#ifndef __IP4_FLOWCTL_H
#define __IP4_FLOWCTL_H

#include "cor_flowctl.h"

struct fc_dev;

/* flow control ipv4 */
struct fc_ipv4 {
  /* common member */
  struct fc_blk fcblk;
  
  /* ipv4 member */
  LW_LIST_LINE list;
  u_char enable;
  ip4_addr_t start_hbo; /* ip range (NOTICE: this is host byte order) */
  ip4_addr_t end_hbo;
  u_char proto; /* IPPROTO_TCP / IPPROTO_UDP / IPPROTO_IP ... 0 is all protocol */
  u_short s_port_hbo; /* port range (NOTICE: this is host byte order) */
  u_short e_port_hbo;
};

/* ipv4 netif flow control funcs */
err_t fcipv4_netif_output(struct netif *netif, struct pbuf *p,
                          const ip4_addr_t *ipaddr);
err_t fcipv4_netif_input(struct pbuf *p, struct netif *netif, s16_t ip_hdr_offset);

/* ipv4 netif flow control remove */
void fcipv4_rule_delif(struct fc_dev *fcdev);

/* ipv4 flow control funcs */
int fcipv4_rule_create(const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                       u_char proto, u_short s_port, u_short e_port, u_char enable,
                       fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize);
int fcipv4_rule_delete(const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                       u_char proto, u_short s_port, u_short e_port);
int fcipv4_rule_change(const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                       u_char proto, u_short s_port, u_short e_port, u_char enable,
                       fc_rate_t s_rate, fc_rate_t r_rate, size_t bsize);
int fcipv4_rule_get(const char *ifname, const ip4_addr_t *start, const ip4_addr_t *end, 
                    u_char proto, u_short s_port, u_short e_port, u_char *enable,
                    fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize);
int fcipv4_rule_search(const char *ifname, const ip4_addr_t *ipaddr, 
                       u_char proto, u_short port, u_char *enable,
                       fc_rate_t *s_rate, fc_rate_t *r_rate, size_t *bsize);

/* ipv4 flow control walk */
void fcipv4_rule_traversal(VOIDFUNCPTR func, void *arg0, void *arg1, 
                           void *arg2, void *arg3, void *arg4);
void fcipv4_total_entry(unsigned int *cnt);

#endif /* __IP4_FLOWCTL_H */
/*
 * end
 */
