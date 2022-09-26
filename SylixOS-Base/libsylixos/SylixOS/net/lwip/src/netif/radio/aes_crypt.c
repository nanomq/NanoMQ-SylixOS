/**
 * @file
 * IEEE 802.15.4 aes crypt driver. 
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

#include "lwip/sys.h"

#include "lowpan_if.h"
#include "ieee802154_frame.h"
#include "ieee802154_aes_ccm.h"

#if LOWPAN_AES_CRYPT

/**
 * This is actually protocol dependent, but this setting works for IPv6 
 * with 6lowpan adaptation over 802.15.4.
 */
#define HEADER_LENGTH    16
 
/**
 * We use the 802.15.4 header as a nonce, fixed at 13 bytes.
 */
#define NONCE_LEN        13

/**
 * Define the buffer which used to help handle.
 * lwip send is 
 */
static u8_t aes_buffer[IEEE802154_MAX_MTU];
static sys_mutex_t aes_mutex = SYS_MUTEX_NULL;

/**
 * This function initialize AES crypt driver
 */
static void aes_init (struct lowpanif *lowpanif)
{
  if (!sys_mutex_valid(&aes_mutex)) {
    sys_mutex_new(&aes_mutex);
  }
  
  if (lowpanif->mfl > (IEEE802154_MAX_MTU - LOWPAN_AES_MIC_LEN)) {
    lowpanif->mfl = (IEEE802154_MAX_MTU - LOWPAN_AES_MIC_LEN);
  }
}

/**
 * This function is called by the network layer to send packets. The 
 * packet will be encrypted before send.
 *
 * @param lowpanif The lowpanif to set encrypt or decrypt
 * @param p The pbuf to be encrypted.
 * @return The encrypted pbuf.
 */
static struct pbuf *aes_encrypt (struct lowpanif *lowpanif, struct pbuf *p)
{
  u8_t nonce[NONCE_LEN];
  int len;
  int new_len;
  struct pbuf *q;
  
  /* If the packet is too short. */
  if (p->tot_len <= HEADER_LENGTH) {
     pbuf_ref(p); /* return pbuf is same as argument, ref it */
     return p;
  }
  
  sys_mutex_lock(&aes_mutex); /* lock aes buffer */
  
  q = p;
  len = 0;
  while (q != NULL) {
    SMEMCPY(&(aes_buffer[len]), q->payload, q->len);
    len += q->len;
    q = q->next;
  }
  SMEMCPY(nonce, aes_buffer, NONCE_LEN);

  new_len = ieee802154_aes_ccm_encrypt((u8_t *)lowpanif->crypt_key, nonce, aes_buffer,
                        HEADER_LENGTH,
                        &((u8_t *)aes_buffer)[HEADER_LENGTH],
                        len - HEADER_LENGTH, LOWPAN_AES_MIC_LEN,
                        aes_buffer);
  if (new_len <= 0) { /* failure */
    sys_mutex_unlock(&aes_mutex); /* unlock aes buffer */
    return NULL;
  }
  
  new_len = len + LOWPAN_AES_MIC_LEN;
  p = pbuf_alloc(PBUF_RAW, ((u16_t)(new_len)), PBUF_POOL);
  if (p == NULL) {
    sys_mutex_unlock(&aes_mutex); /* unlock aes buffer */
    return NULL;
  }
  
  q = p;
  len = 0;
  while (len < new_len) {
    SMEMCPY(q->payload, &aes_buffer[len], q->len);
    len += q->len;
    q = q->next;
  }
  
  sys_mutex_unlock(&aes_mutex); /* unlock aes buffer */
  
  return p; /* return a new pbuf */
}

/**
 * This is called from the RDC layer. We decrypt the incoming packet. 
 *
 * @param lowpanif The lowpanif to set encrypt or decrypt
 * @param p The pbuf to be decrypted.
 * @return The decrypted pbuf.
 */
static struct pbuf *aes_decrypt (struct lowpanif *lowpanif, struct pbuf *p)
{
  u8_t nonce[NONCE_LEN];
  int len;
  int ret;
  struct pbuf *q;

  pbuf_ref(p); /* return pbuf is same as argument, ref it */

  /* If the packet is too short. */
  if (p->tot_len <= HEADER_LENGTH + LOWPAN_AES_MIC_LEN) {
     return p;
  }
  
  sys_mutex_lock(&aes_mutex); /* lock aes buffer */
  
  q = p;
  len = 0;
  while (q != NULL) {
     SMEMCPY(&(aes_buffer[len]), q->payload, q->len);
     len += q->len;
     q = q->next;
  }
  SMEMCPY(nonce, aes_buffer, NONCE_LEN);
  
  ret = ieee802154_aes_ccm_decrypt((u8_t *)lowpanif->crypt_key, nonce, aes_buffer, HEADER_LENGTH,
                          &aes_buffer[HEADER_LENGTH],
                          len - HEADER_LENGTH, LOWPAN_AES_MIC_LEN,
                          &((u8_t *)aes_buffer)[HEADER_LENGTH]);
  if (ret <= 0) { /* decryption failed, dropping packet! */
    sys_mutex_unlock(&aes_mutex); /* unlock aes buffer */
    pbuf_free(p); /* de-ref() */
    return NULL;
  
  } else {
    len = HEADER_LENGTH + ret;
  }
  
  pbuf_realloc(p, (u16_t)len);
  q = p;
  ret = 0;
  while (ret < len) {
     SMEMCPY(q->payload, &aes_buffer[ret], q->len);
     ret += q->len;
     q = q->next;
  }
  
  sys_mutex_unlock(&aes_mutex); /* unlock aes buffer */
  
  return p; /* return a new pbuf */
}

/**
 * This is called from the RDC layer. We verify the incoming packet. 
 *
 * @param lowpanif The lowpanif to set encrypt or decrypt
 * @param p The pbuf to be verified.
 * @return verify_ret_t
 */
static verify_ret_t aes_verify (struct lowpanif *lowpanif, struct pbuf *p)
{
  u8_t nonce[NONCE_LEN];
  int len;
  int ret;
  struct pbuf *q;

  /* If the packet is too short. */
  if (p->tot_len <= HEADER_LENGTH + LOWPAN_AES_MIC_LEN) {
     return CRYPT_TOOSHORT;
  }
  
  sys_mutex_lock(&aes_mutex); /* lock aes buffer */
  
  q = p;
  len = 0;
  while (q != NULL) {
     SMEMCPY(&(aes_buffer[len]), q->payload, q->len);
     len += q->len;
     q = q->next;
  }
  SMEMCPY(nonce, aes_buffer, NONCE_LEN);
  
  ret = ieee802154_aes_ccm_verify((u8_t *)lowpanif->crypt_key, nonce, aes_buffer, HEADER_LENGTH,
                         &aes_buffer[HEADER_LENGTH],
                         len - HEADER_LENGTH, LOWPAN_AES_MIC_LEN);
  
  sys_mutex_unlock(&aes_mutex); /* unlock aes buffer */
  
  if (ret) {
    return CRYPT_OK;
  } else {
    return CRYPT_FAILED;
  }
}

/** AES crypt driver */
struct crypt_driver aes_crypt_driver = {
  "aes",
  aes_init,
  aes_encrypt,
  aes_decrypt,
  aes_verify
};

#endif /* LOWPAN_AES_CRYPT */
/* end */
