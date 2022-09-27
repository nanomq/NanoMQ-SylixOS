/**
 * @file
 * IEEE 802.15.4 frame aes ccm security support. 
 *
 * CCM over AES-128 (RFC3610, http://www.ietf.org/rfc/rfc3610.txt).
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Ren.Haibo <habbyren@qq.com>
 *
 */

#ifndef __IEEE802154_AES_CCM_H__
#define __IEEE802154_AES_CCM_H__

#include "lwip/ethip6.h"
#include "netif/etharp.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LOWPAN_AES_CRYPT

int ieee802154_aes_ccm_encrypt(const u8_t *key, const u8_t *nonce,
                               const u8_t *adata, u32_t adata_len,
                               const u8_t *payload, u32_t payload_len,
                               int mic_len, u8_t *outbuf);
int ieee802154_aes_ccm_decrypt(const u8_t *key, const u8_t *nonce,
                               const u8_t *adata, u32_t adata_len,
                               const u8_t *ciphermic, u32_t ciphermic_len,
                               int mic_len, u8_t *outbuf);
int ieee802154_aes_ccm_verify(const u8_t *key, const u8_t *nonce,
                              const u8_t *adata, u32_t adata_len,
                              const u8_t *ciphermic, u32_t ciphermic_len,
                              int mic_len);

#endif /* LOWPAN_AES_CRYPT */
  
#ifdef __cplusplus
}
#endif

#endif /* __IEEE802154_AES_CCM_H__ */
