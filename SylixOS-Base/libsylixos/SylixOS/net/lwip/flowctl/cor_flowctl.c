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

#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/tcpip.h"
#include "lwip/etharp.h"
#include "cor_flowctl.h"

#if (LW_CFG_NET_FLOWCTL_HZ < 2) || (LW_CFG_NET_FLOWCTL_HZ > 10)
#error "LW_CFG_NET_FLOWCTL_HZ must set 2 ~ 10!"
#endif

/* packet input msg type */
struct fcblk_inpkt_msg {
  struct pbuf *p;
  struct netif *inp;
  netif_input_fn input;
};

/* all flow control block list */
static LW_LIST_LINE_HEADER  fcblk_list[FC_TYPE_MAX];

/* packet input msg */
static LW_HANDLE fcblk_msg;

/* flow control block thread is init? */
static LW_HANDLE fcblk_thd;

/* flow control send queue */
static void fcblk_send_q (struct fc_blk *fcblk, int all, int drop)
{
  PLW_LIST_RING pring;
  struct fc_q *fcq;
  
  while (fcblk->s_q && 
         (all || (fcblk->s_rate == 0) ||
          (fcblk->s_cur < fcblk->s_rate))) {
    pring = _list_ring_get_prev(fcblk->s_q);
    fcq = (struct fc_q *)pring;
    fcblk->s_cur += fcq->p.pbuf.tot_len;
    fcblk->cur_size -= fcq->p.pbuf.tot_len;
    _List_Ring_Del(&fcq->ring, &fcblk->s_q);
    if (!drop) {
      fcblk->funcs->s_packet(fcq->priv, &fcq->p.pbuf, &fcq->addr);
    }
    pbuf_free(&fcq->p.pbuf);
  }
  fcblk->s_cur = 0; /* clear */
}

/* flow control recv queue */
static void fcblk_recv_q (struct fc_blk *fcblk, int all, int drop)
{
  PLW_LIST_RING pring;
  struct fc_q *fcq;
  
  while (fcblk->r_q && 
         (all || (fcblk->r_rate == 0) || 
          (fcblk->r_cur < fcblk->r_rate))) {
    pring = _list_ring_get_prev(fcblk->r_q);
    fcq = (struct fc_q *)pring;
    fcblk->r_cur += fcq->p.pbuf.tot_len;
    fcblk->cur_size -= fcq->p.pbuf.tot_len;
    _List_Ring_Del(&fcq->ring, &fcblk->r_q);
    if (!drop) {
      fcblk->funcs->r_packet(fcq->priv, &fcq->p.pbuf);
    } else {
      pbuf_free(&fcq->p.pbuf);
    }
  }
  fcblk->r_cur = 0; /* clear */
}

/* flow control block buffer flush */
static void fcblk_flush (void *id, int drop)
{
  int i;
  PLW_LIST_LINE pline;
  struct fc_blk *fcblk;
  
  /* chang to pypass mode */
  if (!drop) {
    for (i = FC_TYPE_IPV4; i <= FC_TYPE_NETIF; i++) {
      for (pline = fcblk_list[i]; pline != NULL; pline = _list_line_get_next(pline)) {
        fcblk = (struct fc_blk *)pline;
        if (fcblk->id == id) {
          fcblk->bypass = 1;
        }
      }
    }
  }
  
  /* tiger send packet */
  for (i = FC_TYPE_NETIF; i >= FC_TYPE_IPV4; i--) {
    for (pline = fcblk_list[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      fcblk = (struct fc_blk *)pline;
      if (fcblk->id == id) {
        fcblk_send_q(fcblk, 1, drop);
      }
    }
  }
  
  /* tiger recv packet */
  for (i = FC_TYPE_IPV4; i <= FC_TYPE_NETIF; i++) {
    for (pline = fcblk_list[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      fcblk = (struct fc_blk *)pline;
      if (fcblk->id == id) {
        fcblk_recv_q(fcblk, 1, drop);
      }
    }
  }
  
  /* chang to !pypass mode */
  if (!drop) {
    for (i = FC_TYPE_IPV4; i <= FC_TYPE_NETIF; i++) {
      for (pline = fcblk_list[i]; pline != NULL; pline = _list_line_get_next(pline)) {
        fcblk = (struct fc_blk *)pline;
        if (fcblk->id == id) {
          fcblk->bypass = 0;
        }
      }
    }
  }
}

/* flow control block thread timer */
static void fcblk_timer (void)
{
  int i;
  PLW_LIST_LINE pline;
  struct fc_blk *fcblk;

  FC_LOCK();
  
  /* for low layer to high layer tiger send packet */
  for (i = FC_TYPE_NETIF; i >= FC_TYPE_IPV4; i--) {
    for (pline = fcblk_list[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      fcblk = (struct fc_blk *)pline;
      fcblk_send_q(fcblk, 0, 0);
    }
  }

  /* for high layer to low layer tiger recv packet */
  for (i = FC_TYPE_IPV4; i <= FC_TYPE_NETIF; i++) {
    for (pline = fcblk_list[i]; pline != NULL; pline = _list_line_get_next(pline)) {
      fcblk = (struct fc_blk *)pline;
      fcblk_recv_q(fcblk, 0, 0);
    }
  }

  FC_UNLOCK();
}

/* flow control block thread 
 * Prevent tcpip queue data congestion, we use a thread instead a timer */
static void fcblk_thread (void)
{
  struct fcblk_inpkt_msg msg;
  ULONG err;
  ULONG now, to = LW_TICK_HZ / LW_CFG_NET_FLOWCTL_HZ;
  ULONG last = API_TimeGet();
  
  for (;;) {
    err = API_MsgQueueReceive(fcblk_msg, &msg, sizeof(struct fcblk_inpkt_msg), NULL, to);
    if (err) {
      LWIP_ASSERT("fcblk_thread msg error!", (err == ERROR_THREAD_WAIT_TIMEOUT));
      fcblk_timer();
      last = API_TimeGet();
      continue;
    }
    
    /* check if we must call timer */
    now = API_TimeGet();
    if (now > last) {
      if ((now - last) >= to) {
        fcblk_timer();
        last = now;
      }
    
    } else { /* timer overflow */
      if (((ULONG_MAX - last) + now) >= to) {
        fcblk_timer();
        last = now;
      }
    }
    
    FC_LOCK();
    
    /* call input function */
    if (msg.input(msg.p, msg.inp)) {
      pbuf_free(msg.p);
    }
    
    FC_UNLOCK();
  }
}

/* flow control pbuf free hook */
static void fcblk_pbuf_free (struct pbuf *p)
{
  struct fc_q *fcq = _LIST_ENTRY(p, struct fc_q, p);
  
  mem_free(fcq);
}

/* flow control pbuf alloc */
static struct fc_q *fcblk_pbuf_alloc (struct pbuf *p)
{
  struct fc_q *fcq;
  struct pbuf *ret;
  u16_t reserve = ETH_PAD_SIZE + SIZEOF_VLAN_HDR;
  u16_t tot_len = (u16_t)(reserve + p->tot_len);
  
  fcq = (struct fc_q *)mem_malloc(ROUND_UP(sizeof(struct fc_q), MEM_ALIGNMENT) + tot_len);
  if (fcq == NULL) {
    return (NULL);
  }
  
  fcq->p.custom_free_function = fcblk_pbuf_free;
  
  ret = pbuf_alloced_custom(PBUF_RAW, tot_len, PBUF_POOL, &fcq->p,
                            (char *)fcq + ROUND_UP(sizeof(struct fc_q), MEM_ALIGNMENT), 
                            tot_len);
  if (ret) {
    pbuf_header(ret, (u16_t)-reserve);
    pbuf_copy(ret, p);
  }
  
  return (fcq);
}

/* flow control block output */
err_t fcblk_output (struct fc_blk *fcblk, struct pbuf *p, void *priv, const ip4_addr_t *ipaddr)
{
  struct fc_q *fcq;
  ip_addr_t addr;
  
  if (fcblk->bypass) {
    ip_addr_copy_from_ip4(addr, *(ipaddr));
    fcblk->funcs->s_packet(priv, p, &addr); /* send directly */
    return (ERR_OK);
  }
  
  if (fcblk->s_rate == 0) { /* stop network output */
    return (ERR_OK);
  }
  
  if (fcblk->s_q ||
      ((fcblk->s_cur + p->tot_len) > fcblk->s_rate)) {
    if ((fcblk->cur_size + p->tot_len) > fcblk->buf_size) {
      return (ERR_MEM); /* buffer full, drop! */
    }
    
    fcq = fcblk_pbuf_alloc(p);
    if (fcq == NULL) {
      return (ERR_MEM);
    }
    
    fcq->priv = priv;
    ip_addr_copy_from_ip4(fcq->addr, *(ipaddr));
    _List_Ring_Add_Ahead(&fcq->ring, &fcblk->s_q); /* put to send queue */
    fcblk->cur_size += p->tot_len;
  
  } else {
    ip_addr_copy_from_ip4(addr, *(ipaddr));
    fcblk->funcs->s_packet(priv, p, &addr); /* send directly */
    fcblk->s_cur += p->tot_len;
  }
  
  return (ERR_OK);
}

#if LWIP_IPV6
/* flow control block output ipv6 */
err_t fcblk_output_ip6 (struct fc_blk *fcblk, struct pbuf *p, void *priv, const ip6_addr_t *ipaddr)
{
  struct fc_q *fcq;
  ip_addr_t addr;
  
  if (fcblk->bypass) {
    ip_addr_copy_from_ip6(addr, *(ipaddr));
    fcblk->funcs->s_packet(priv, p, &addr); /* send directly */
    return (ERR_OK);
  }
  
  if (fcblk->s_rate == 0) { /* stop network output */
    return (ERR_OK);
  }
  
  if (fcblk->s_q ||
      ((fcblk->s_cur + p->tot_len) > fcblk->s_rate)) {
    if ((fcblk->cur_size + p->tot_len) > fcblk->buf_size) {
      return (ERR_MEM); /* buffer full, drop! */
    }
    
    fcq = fcblk_pbuf_alloc(p);
    if (fcq == NULL) {
      return (ERR_MEM);
    }
    
    fcq->priv = priv;
    ip_addr_copy_from_ip6(fcq->addr, *(ipaddr));
    _List_Ring_Add_Ahead(&fcq->ring, &fcblk->s_q); /* put to send queue */
    fcblk->cur_size += p->tot_len;
  
  } else {
    ip_addr_copy_from_ip6(addr, *(ipaddr));
    fcblk->funcs->s_packet(priv, p, &addr); /* send directly */
    fcblk->s_cur += p->tot_len;
  }
  
  return (ERR_OK);
}
#endif /* LWIP_IPV6 */

/* flow control block recv */
err_t fcblk_input (struct fc_blk *fcblk, struct pbuf *p, void *priv)
{
  struct fc_q *fcq;
  
  if (fcblk->bypass) {
    fcblk->funcs->r_packet(priv, p); /* recv directly */
    return (ERR_OK);
  }
  
  if (fcblk->r_rate == 0) { /* stop network input */
    pbuf_free(p); /* free input pbuf */
    return (ERR_OK);
  }
  
  if (fcblk->r_q || 
      ((fcblk->r_cur + p->tot_len) > fcblk->r_rate)) {
    if ((fcblk->cur_size + p->tot_len) > fcblk->buf_size) {
      return (ERR_MEM); /* buffer full, drop! */
    }
    
    fcq = fcblk_pbuf_alloc(p);
    if (fcq == NULL) {
      return (ERR_MEM);
    }
    
    pbuf_free(p); /* free input pbuf */
    fcq->priv = priv;
    _List_Ring_Add_Ahead(&fcq->ring, &fcblk->r_q);
    fcblk->cur_size += p->tot_len;
  
  } else {
    fcblk->funcs->r_packet(priv, p); /* recv directly */
    fcblk->r_cur += p->tot_len;
  }
  
  return (ERR_OK);
}

/* flow control init */
static void fcblk_init (void)
{
  if (fcblk_msg == LW_HANDLE_INVALID) {
    fcblk_msg = API_MsgQueueCreate("fc_msg", LW_CFG_LWIP_POOL_SIZE,
                                   sizeof(struct fcblk_inpkt_msg), LW_OPTION_OBJECT_GLOBAL, NULL);
  }

  if (fcblk_thd == LW_HANDLE_INVALID) {
    fcblk_thd = sys_thread_new("t_flowctl", (lwip_thread_fn)fcblk_thread,
                               NULL, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);
  }
}

/* flow control packet input */
err_t fcblk_inpkt (struct pbuf *p, struct netif *inp, netif_input_fn input_fn)
{
  struct fcblk_inpkt_msg msg;
  
  msg.p = p;
  msg.inp = inp;
  msg.input = input_fn;
  if (API_MsgQueueSend(fcblk_msg, &msg, sizeof(struct fcblk_inpkt_msg))) {
    return (ERR_BUF);
  }
  return (ERR_OK);
}

/* flow control block start */
void fcblk_start (struct fc_blk *fcblk)
{
  _List_Line_Add_Ahead(&fcblk->list, &fcblk_list[fcblk->type]); /* add to timer list */
  fcblk_init();
}

/* flow control stop */
void fcblk_stop (struct fc_blk *fcblk, int drop)
{
  _List_Line_Del(&fcblk->list, &fcblk_list[fcblk->type]); /* delete from timer list */
  fcblk_flush(fcblk->id, drop);
}

#endif /* LW_CFG_NET_FLOWCTL_EN > 0 */
/*
 * end
 */
