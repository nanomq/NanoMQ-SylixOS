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

#ifndef __COR_FLOWCTL_H
#define __COR_FLOWCTL_H

#include "lwip/pbuf.h"
#include "lwip/tcpip.h"

/* flow control lock */
#define FC_LOCK() \
        if (lock_tcpip_core) { \
          LOCK_TCPIP_CORE(); \
        }
#define FC_UNLOCK() \
        if (lock_tcpip_core) { \
          UNLOCK_TCPIP_CORE(); \
        }

/* flow control rate */
typedef UINT64 fc_rate_t;

/* flow control buffer queue node */
struct fc_q {
  LW_LIST_RING ring;/* ring list */
  void *priv; /* send or recv priv struct */
  ip_addr_t addr; /* send addr */
  struct pbuf_custom p; /* packet */
};

#define FC_TYPE_IPV4    0   /* ipv4 flow control type */
#define FC_TYPE_IPV6    1   /* ipv6 flow control type */
#define FC_TYPE_NETIF   2   /* netif flow control type */
#define FC_TYPE_MAX     3   /* max entry of type queue */

/* flow control block functions */
struct fc_funcs {
  /* s_packet function must do not delete pbuf always */
  void (*s_packet)(void *priv, struct pbuf *p, ip_addr_t *addr);
  /* r_packet function must delete pbuf always */
  void (*r_packet)(void *priv, struct pbuf *p);
};

/* flow control block */
struct fc_blk {
  LW_LIST_LINE list; /* flow control timer list */
  u_char type; /* FC_TYPE_IPV4 / FC_TYPE_IPV6 / FC_TYPE_NETIF */
  u_char bypass; /* bypass mode */
  LW_LIST_RING_HEADER s_q; /* send queue */
  LW_LIST_RING_HEADER r_q; /* recv queue */
  fc_rate_t s_cur; /* this sample time send packet bytes */
  fc_rate_t r_cur; /* this sample time recv packet bytes */
  fc_rate_t s_rate; /* send rate (per timeouts tick) */
  fc_rate_t r_rate; /* recv rate (per timeouts tick) */
  size_t cur_size; /* current buffer size */
  size_t buf_size; /* total buffer size of each fcblk */
  const struct fc_funcs *funcs; /* send and recv functions */
  void *id; /* identifier */
};

/* flow control thread input queue */
err_t fcblk_inpkt(struct pbuf *p, struct netif *inp, netif_input_fn input_fn);

/* flow control block input output funcs */
err_t fcblk_output(struct fc_blk *fcblk, struct pbuf *p, void *priv, const ip4_addr_t *ipaddr);
#if LWIP_IPV6
err_t fcblk_output_ip6(struct fc_blk *fcblk, struct pbuf *p, void *priv, const ip6_addr_t *ipaddr);
#endif
err_t fcblk_input(struct fc_blk *fcblk, struct pbuf *p, void *priv);

/* flow control block add or delete */
void fcblk_start(struct fc_blk *fcblk);
void fcblk_stop(struct fc_blk *fcblk, int drop);

#endif /* __COR_FLOWCTL_H */
/*
 * end
 */
