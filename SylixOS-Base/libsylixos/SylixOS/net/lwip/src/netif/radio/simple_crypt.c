/**
 * @file
 * IEEE 802.15.4 simple crypt driver. 
 *
 */

/*
 * Copyright (c) 2006-2014 SylixOS Group.
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
 * Author: Han.hui <sylixos@gmail.com>
 *
 */

#include "lowpan_if.h"

#if LOWPAN_SIMPLE_CRYPT

/**
 * This is actually protocol dependent, but this setting works for IPv6 
 * with 6lowpan adaptation over 802.15.4.
 */
#define HEADER_LENGTH    16

/**
 * This function initialize SIMPLE crypt driver
 */
static void simple_init (struct lowpanif *lowpanif)
{
}

/**
 * This function is called by the network layer/RDC layer to send/recive packets. The 
 * packet will be encrypted/decrypted before send/recive.
 *
 * @param lowpanif The lowpanif to set encrypt or decrypt
 * @param p The pbuf to be encrypted/decrypted.
 * @return The encrypted pbuf.
 */
static struct pbuf *simple_endecrypt (struct lowpanif *lowpanif, struct pbuf *p)
{
  u8_t index;
  int i;
  u8_t *buffer;
  struct pbuf *q;
  
  pbuf_ref(p); /* return pbuf is same as argument, ref it */
  
  if (p->tot_len <= HEADER_LENGTH) {
    return p;
  }
  
  /* First, delete the encrypt header. */
  if (pbuf_header(p, -(s16_t)HEADER_LENGTH)) {
    return p;
  }
  
  q = p;
  index = 0;
  while (q != NULL) {
     buffer = (u8_t *)q->payload;
     for (i = 0; i < q->len; i++) {
        buffer[i] = buffer[i] ^ lowpanif->crypt_key[index];
        index = (index + 1) % LOWPAN_CRYPT_KEY_LEN;
     }
     q = q->next;
  }
  
  pbuf_header(p, HEADER_LENGTH);
  
  return p;
}

/**
 * This is called from the RDC layer. We verify the incoming packet. 
 *
 * @param lowpanif The lowpanif to set encrypt or decrypt
 * @param p The pbuf to be verified.
 * @return verify_ret_t
 */
static verify_ret_t simple_verify (struct lowpanif *lowpanif, struct pbuf *p)
{
  return CRYPT_OK;
}

/** SIMPLE crypt driver */
struct crypt_driver simple_crypt_driver = {
  "simple",
  simple_init,
  simple_endecrypt,
  simple_endecrypt,
  simple_verify
};

#endif /* LOWPAN_SIMPLE_CRYPT */
/* end */
