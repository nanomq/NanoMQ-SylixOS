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

#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "netif/etharp.h"
#include "lowpan_if.h"
#include "ieee802154_aes.h"

#include "string.h"

#if LOWPAN_AES_CRYPT  /* don't build if not configured for use in radio_param.h */
/**
 * define some configuration for use.
 */
#define BLOCK_SIZE          16
#define L_SIZELEN           2              /* Size field length: 2 to 8 bytes  */
#define NONCE_LEN          (15-L_SIZELEN)  /* Nonce length: fixed to 13 bytes */

/**
 * The cbcmac xor buffer
 */
static u8_t cbcmac_xor[BLOCK_SIZE];

/**
 * Clear the cbcmac xor buffer.
 *
 * @return none
 */
static void
cbcmac_clear()
{
  /* clear last cipher block */
  memset(cbcmac_xor, 0, sizeof(cbcmac_xor));
}

/**
 * Append cbcmac block.
 *
 * @param key The key to append the buffer
 * @param block The block to append
 * @return none
 */
static void
cbcmac_append(const u8_t *key, u8_t *block)
{
  u8_t i;

  /* xor with last cipher block */
  for(i = 0; i < BLOCK_SIZE; i++) {
    block[i] ^= cbcmac_xor[i];
  }

  /* encrypt */
  ieee802154_aes_encrypt(block, key);
  SMEMCPY(cbcmac_xor, block, BLOCK_SIZE);
}

/**
 * encrypt the ctr next block
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param counter The number of 
 * @param outbuf The Encrypt result
 * @return none
 */
static void
ctr_next_ctr_block(const u8_t *key, const u8_t *nonce,
                  u16_t counter, u8_t *outbuf)
{
  u8_t flags;
  u16_t tmp;

  /* Prepare CTR block */
  memset(outbuf, 0, BLOCK_SIZE);

  /* CTR block: Flags field */
  flags = 1 * (L_SIZELEN - 1); /* size. length */
  SMEMCPY(&outbuf[0], &flags, 1);

  /* CTR block: Nonce */
  SMEMCPY(&outbuf[1], nonce, NONCE_LEN);

  /* CTR block: Counter */
  tmp = PP_HTONS(counter);
  SMEMCPY(&outbuf[BLOCK_SIZE - L_SIZELEN], &tmp, L_SIZELEN); /* MSB. */

  /* Encrypt CTR block */
  ieee802154_aes_encrypt(outbuf, key);
}

/**
 * calc the cbamac to encrypt
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param adata The adata
 * @param adata_len The adata length
 * @param payload The payload
 * @param payload_len The payload length
 * @param mic_len The mic len 
 * @param outbuf The Encrypt result
 * @return  Number of encrypted bytes at success, negative value at failure.
 */
static int
cbcmac_calc(const u8_t *key, const u8_t *nonce,
            const u8_t *adata, u32_t adata_len,
            const u8_t *payload, u32_t payload_len,
            int mic_len, u8_t *outbuf)
{
  u8_t BUF[BLOCK_SIZE];
  u16_t tmp, tmp2;
  u8_t flags;

  cbcmac_clear();

  /* Block B_0 */
  memset(BUF, 0, sizeof(BUF));

  /* B_0: Flags field */
  flags = 0;
  flags += 64 * (adata_len > 0 ? 1 : 0); /* contains associated data */
  flags += 8 * ((mic_len - 2) / 2); /* auth. length */
  flags += 1 * (L_SIZELEN - 1); /* size. length */
  SMEMCPY(&BUF[0], &flags, 1);

  /* B_0: Nonce */
  SMEMCPY(&BUF[1], nonce, NONCE_LEN);

  /* B_0: Size field */
  tmp2 = (u16_t) payload_len; /* XXX Max size supported is 0xFFFF */
  tmp = PP_HTONS(tmp2);
  SMEMCPY(&BUF[BLOCK_SIZE - L_SIZELEN], &tmp, L_SIZELEN); /* MSB */
  cbcmac_append(key, BUF);

  /* B_1..n: auth. blocks */
  if(adata_len > 0) {
    u32_t left, idx;
    int len;

    memset(BUF, 0, sizeof(BUF));
    if(adata_len < 65536) {
      /* 2 bytes data length in first auth. block */
      tmp2 = (u16_t) adata_len;
      tmp = PP_HTONS(tmp2);
      SMEMCPY(&BUF[0], &tmp, 2);
      len = 2;
    } else {
      u8_t sizefield[6];
      sizefield[0] = 0xff;
      sizefield[1] = 0xfe;
      sizefield[2] = (adata_len >> 24) & 0xff;
      sizefield[3] = (adata_len >> 16) & 0xff;
      sizefield[4] = (adata_len >> 8) & 0xff;
      sizefield[5] = (adata_len) & 0xff;
      SMEMCPY(&BUF[0], &sizefield, sizeof(sizefield));
      len = sizeof(sizefield);
    }

    /* 16 - len bytes data in first auth. block */
    left = adata_len;
    idx = 0;
    SMEMCPY(&BUF[len], &adata[idx],
           (left > (BLOCK_SIZE - len) ? (BLOCK_SIZE - len) : left));
    idx += (left > (BLOCK_SIZE - len) ? (BLOCK_SIZE - len) : left);
    left -= (left > (BLOCK_SIZE - len) ? (BLOCK_SIZE - len) : left);

    /* 16 bytes data in subsequent auth. blocks */
    while(left > 0) {
      cbcmac_append(key, BUF);

      /* Auth data + padding with zeroes */
      memset(BUF, 0, sizeof(BUF));
      SMEMCPY(&BUF[0], &adata[idx], (left > BLOCK_SIZE ? BLOCK_SIZE : left));
      idx += (left > BLOCK_SIZE ? BLOCK_SIZE : left);
      left -= (left > BLOCK_SIZE ? BLOCK_SIZE : left);

    }

    cbcmac_append(key, BUF);
  }

  /* B_n..m: message blocks */
  if(payload_len > 0) {
    u32_t left, idx;

    memset(BUF, 0, sizeof(BUF));

    left = payload_len;
    idx = 0;
    while(left > 0) {

      /* Auth data + padding with zeroes */
      memset(BUF, 0, sizeof(BUF));
      SMEMCPY(&BUF[0], &payload[idx], (left > BLOCK_SIZE ? BLOCK_SIZE : left));

      idx += (left > BLOCK_SIZE ? BLOCK_SIZE : left);
      left -= (left > BLOCK_SIZE ? BLOCK_SIZE : left);

      cbcmac_append(key, BUF);
    }
  }

  /* Copy mic_len bytes of CBC-MAC to outbuf */
  SMEMCPY(outbuf, BUF, mic_len);
  return mic_len;
}

/**
 * calc the cbamac to encrypt
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param adata The adata
 * @param adata_len The adata length
 * @param cipher The cipher
 * @param cipher_len The cipher length
 * @param mic_len The mic len 
 * @param outbuf The Encrypt result
 * @return Number of encrypted bytes at success, negative value at failure.
 */
static int
cbcmac_verify(const u8_t *key, const u8_t *nonce,
              const u8_t *adata, u32_t adata_len,
              const u8_t *cipher, u32_t cipher_len,
              int mic_len, u8_t *outbuf)
{
  u8_t BUF[BLOCK_SIZE];
  u16_t tmp, tmp2;
  u8_t flags;

  cbcmac_clear();

  /* Block B_0 */
  memset(BUF, 0, sizeof(BUF));

  /* B_0: Flags field */
  flags = 0;
  flags += 64 * (adata_len > 0 ? 1 : 0); /* contains associated data */
  flags += 8 * ((mic_len - 2) / 2); /* auth. length */
  flags += 1 * (L_SIZELEN - 1); /* size. length */
  SMEMCPY(&BUF[0], &flags, 1);

  /* B_0: Nonce */
  SMEMCPY(&BUF[1], nonce, NONCE_LEN);

  /* B_0: Size field */
  tmp2 = (u16_t) cipher_len; /* Note: limits cipher length to 65536 bytes */
  tmp = PP_HTONS(tmp2);
  SMEMCPY(&BUF[BLOCK_SIZE - L_SIZELEN], &tmp, L_SIZELEN); /* MSB */
  cbcmac_append(key, BUF);

  /* B_1..n: auth. blocks */
  if(adata_len > 0) {
    u32_t left, idx;
    int len;

    memset(BUF, 0, sizeof(BUF));
    if(adata_len < 65536) {
      /* 2 bytes data length in first auth. block */
      tmp2 = (u16_t) adata_len;
      tmp = PP_HTONS(tmp2);
      SMEMCPY(&BUF[0], &tmp, 2);
      len = 2;
    } else {
      u8_t sizefield[6];
      sizefield[0] = 0xff;
      sizefield[1] = 0xfe;
      sizefield[2] = (adata_len >> 24) & 0xff;
      sizefield[3] = (adata_len >> 16) & 0xff;
      sizefield[4] = (adata_len >> 8) & 0xff;
      sizefield[5] = (adata_len) & 0xff;
      SMEMCPY(&BUF[0], &sizefield, sizeof(sizefield));
      len = sizeof(sizefield);
    }

    /* 16 - len bytes data in first auth. block */
    left = adata_len;
    idx = 0;
    SMEMCPY(&BUF[len], &adata[idx],
           (left > (BLOCK_SIZE - len) ? (BLOCK_SIZE - len) : left));
    idx += (left > (BLOCK_SIZE - len) ? (BLOCK_SIZE - len) : left);
    left -= (left > (BLOCK_SIZE - len) ? (BLOCK_SIZE - len) : left);

    /* 16 bytes data in subsequent auth. blocks */
    while(left > 0) {
      cbcmac_append(key, BUF);
      /* Auth data + padding with zeroes */
      memset(BUF, 0, sizeof(BUF));
      SMEMCPY(&BUF[0], &adata[idx], (left > BLOCK_SIZE ? BLOCK_SIZE : left));
      idx += (left > BLOCK_SIZE ? BLOCK_SIZE : left);
      left -= (left > BLOCK_SIZE ? BLOCK_SIZE : left);
    }

    cbcmac_append(key, BUF);
  }

  /* B_n..m: message blocks */
  if(cipher_len > 0) {
    u32_t left, idx, counter, i;

    memset(BUF, 0, sizeof(BUF));

    left = cipher_len;
    idx = 0;
    counter = 1; /* S_1 .. S_n */
    while(left > 0) {

      /* Decrypt block right now */
      ctr_next_ctr_block(key, nonce, counter, BUF);
      /* XOR with payload block */
      for(i = 0; i < BLOCK_SIZE; i++) {
        if(idx + i >= cipher_len) {
          BUF[i] = 0;
        } else {
          BUF[i] ^= cipher[idx + i];
        }
      }
      counter++;

      idx += (left > BLOCK_SIZE ? BLOCK_SIZE : left);
      left -= (left > BLOCK_SIZE ? BLOCK_SIZE : left);

      cbcmac_append(key, BUF);
    }
  }

  /* Copy mic_len bytes of CBC-MAC to outbuf */
  SMEMCPY(outbuf, BUF, mic_len);
  return mic_len;
}

/**
 * encrypt the payload
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param payload The payload
 * @param payload_len The payload length
 * @param outbuf The Encrypt result
 * @return Number of encrypted bytes at success, negative value at failure.
 */
static u32_t
ctr_payload(const u8_t *key, const u8_t *nonce,
            const u8_t *payload, u32_t payload_len,
            u8_t *outbuf)
{
  u8_t BUF[BLOCK_SIZE];
  u16_t counter;
  int i;
  u32_t left, idx;

  /* Encrypt payload */
  left = payload_len;
  idx = 0;
  counter = 1; /* S_1 .. S_n */
  while(left > 0) {
    ctr_next_ctr_block(key, nonce, counter, BUF);

    /* XOR with payload block */
    for(i = 0; i < BLOCK_SIZE; i++) {
      if(idx + i >= payload_len) {
        break;
      }
      BUF[i] ^= payload[idx + i];
    }

    SMEMCPY(&outbuf[idx], BUF, (left > BLOCK_SIZE ? BLOCK_SIZE : left));

    idx += (left > BLOCK_SIZE ? BLOCK_SIZE : left);
    left -= (left > BLOCK_SIZE ? BLOCK_SIZE : left);

    counter++;
  }

  return idx;
}

/**
 * encrypt the mic
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param cbcmac The cbcmac
 * @param mic_len The mic length
 * @param outbuf The Encrypt result
 * @return Number of encrypted bytes at success, negative value at failure.
 */
static int
ctr_mic(const u8_t *key, const u8_t *nonce,
        const u8_t *cbcmac, int mic_len, u8_t *outbuf)
{
  u8_t BUF[BLOCK_SIZE];
  u8_t flags;
  u16_t tmp;
  int i;
  
  /* Prepare CTR block */
  memset(BUF, 0, sizeof(BUF));

  /* CTR block: Flags field */
  flags = 1 * (L_SIZELEN - 1); /* size. length */
  SMEMCPY(&BUF[0], &flags, 1);

  /* CTR block: Nonce */
  SMEMCPY(&BUF[1], nonce, NONCE_LEN);

  /* CTR block: Counter */
  tmp = PP_HTONS(0); /* S_0: counter is 0 */
  SMEMCPY(&BUF[BLOCK_SIZE - L_SIZELEN], &tmp, L_SIZELEN); /* MSB. */

  /* Encrypt CTR block */
  ieee802154_aes_encrypt(BUF, key);

  /* XOR with CBC-MAC */
  for(i = 0; i < mic_len; i++) {
    BUF[i] ^= cbcmac[i];
  }

  SMEMCPY(outbuf, BUF, mic_len);

  return mic_len;
}

/**
 * encrypt the buffer by aes ccm
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param adata The adata
 * @param adata_len The adata length
 * @param payload The payload
 * @param payload_len The payload length
 * @param mic_len The mic len 
 * @param outbuf The Encrypt result
 * @return Number of encrypted bytes at success, negative value at failure.
 */
int
ieee802154_aes_ccm_encrypt(const u8_t *key, const u8_t *nonce,
                const u8_t *adata, u32_t adata_len,
                const u8_t *payload, u32_t payload_len,
                int mic_len, u8_t *outbuf)
{
  int cbcmac_len;
  int ctr_len;

  /* MIC length: 4, 6, 8, 10, 12, 14, or 16 bytes */
  u8_t cbcmac[16];

  /* Copy adata (header) */
  SMEMCPY(&outbuf[0], adata, adata_len);


  /* Authentication: calculate CBC-MAC (MIC) over header and payload */
  cbcmac_len = cbcmac_calc(key, nonce, adata, adata_len, payload, payload_len,
                           mic_len, cbcmac);
  if(cbcmac_len < 0 || cbcmac_len != mic_len) {
    return -1;
  }

  /* Encryption: encrypt payload using CTR */
  ctr_len = ctr_payload(key, nonce, payload, payload_len, &outbuf[adata_len]);
  if(ctr_len < 0) {
    return -2;
  }

  /* Encryption: encrypt MIC */
  mic_len = ctr_mic(key, nonce, cbcmac, mic_len, &outbuf[adata_len + payload_len]);
  if(mic_len < 0) {
    return -3;
  }

  return (u32_t)(adata_len + payload_len + mic_len);
}

/**
 * decrypt the buffer by aes ccm
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param adata The adata
 * @param adata_len The adata length
 * @param ciphermic The ciphermic
 * @param ciphermic_len The ciphermic length
 * @param mic_len The mic len 
 * @param outbuf The Encrypt result
 * @return Number of plaintext bytes at success, negative value at failure.
 */
int
ieee802154_aes_ccm_decrypt(const u8_t *key, const u8_t *nonce,
                    const u8_t *adata, u32_t adata_len,
                    const u8_t *ciphermic, u32_t ciphermic_len,
                    int mic_len, u8_t *outbuf)
{
  int cbcmac_len, cbcmac_len2;
  u32_t i, ctr_len;

  /* MIC length: 4, 6, 8, 10, 12, 14, or 16 bytes */
  u8_t cbcmac[16];
  u8_t cbcmac2[16]; /* recomputed MIC */

  /*
   * Return value:
   * Number of plaintext bytes at success, negative value at failure.
   */

  /* Santity-check: Cipher + MIC must be equal to or longer than the MIC itself */
  if(ciphermic_len < mic_len) {
    return -6;
  }

  /* Decryption: decrypt MIC */
  cbcmac_len = ctr_mic(key, nonce, &ciphermic[ciphermic_len - mic_len], mic_len,
                       cbcmac);
  if(cbcmac_len < 0 || cbcmac_len != mic_len) {
    return -1;
  }

  /* Decryption: decrypt payload using CTR */
  ctr_len = ctr_payload(key, nonce, ciphermic, ciphermic_len - mic_len,
                        &outbuf[0]);
  if(ctr_len == 0) {
    return -2;
  }

  /* Authentication: re-calculate CBC-MAC (MIC) over header and payload */
  cbcmac_len2 = cbcmac_calc(key, nonce, adata, adata_len, outbuf, ctr_len,
                            mic_len, cbcmac2);
  if(cbcmac_len2 < 0 || cbcmac_len2 != mic_len) {
    return -3;
  }

  /* Verify that MICs match */
  if(cbcmac_len != cbcmac_len2) {
    return -4;
  }
  for(i = 0; i < cbcmac_len; i++) {
    if(cbcmac[i] != cbcmac2[i]) {
      return -5;
    }
  }

  return (int)ctr_len;
}

/**
 * verify the buffer by aes ccm mic
 *
 * @param key The key to encrypt
 * @param nonce The nonce to init
 * @param adata The adata
 * @param adata_len The adata length
 * @param ciphermic The ciphermic
 * @param ciphermic_len The ciphermic length
 * @param mic_len The mic len 
 * @return 1 if buffer integrity was verified, 0 otherwise.
 */
int
ieee802154_aes_ccm_verify(const u8_t *key, const u8_t *nonce,
                   const u8_t *adata, u32_t adata_len,
                   const u8_t *ciphermic, u32_t ciphermic_len,
                   int mic_len)
{
  int i;
  int cbcmac_len, cbcmac_len2;

  /* MIC length: 4, 6, 8, 10, 12, 14, or 16 bytes */
  u8_t cbcmac[16];
  u8_t cbcmac2[16]; /* recomputed MIC */

  /* Decryption: decrypt MIC */
  cbcmac_len = ctr_mic(key, nonce, &ciphermic[ciphermic_len - mic_len], mic_len,
                       cbcmac);
  if(cbcmac_len < 0 || cbcmac_len != mic_len) {
    return 0;
  }

  /* Integrity check: both decrypt and calculate CBC-MAC at the same time.
   * We now decrypt the cipher on-the-fly, without storing the cleartext. */
  cbcmac_len2 = cbcmac_verify(key, nonce, adata, adata_len, ciphermic,
                              ciphermic_len - mic_len, mic_len, cbcmac2);
  if(cbcmac_len2 < 0 || cbcmac_len2 != mic_len) {
    return 0;
  }

  /* Verify that MICs match */
  if(cbcmac_len != cbcmac_len2) {
    return 0;
  }
  for(i = 0; i < cbcmac_len; i++) {
    if(cbcmac[i] != cbcmac2[i]) {
      return 0;
    }
  }

  return 1;
}

#endif /* LOWPAN_AES_CRYPT */
/* end */
