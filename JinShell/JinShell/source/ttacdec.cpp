/*
 * ttadec.c
 *
 * Description:	 TTAv1 decoder library for HW players
 * Developed by: Alexander Djourik <ald@true-audio.com>
 *               Pavel Zhilin <pzh@true-audio.com>
 *
 * Copyright (c) 2004 True Audio Software. All rights reserved.
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the True Audio Software nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if 0

#include <nds.h>

#include "plugin.h"
#include "plugin_def.h"
#include "std.h"

#include "ttacdec.h"
#include "ttac_filter.h"

/************************* bit operations ******************************/

#define GET_BINARY(value, bits) \
  while (bit_count < bits) { \
    bit_cache |= (*bit_pos++) << bit_count; \
    bit_count += 16; \
  } \
  value = bit_cache & bit_mask[bits]; \
  bit_cache >>= bits; \
  bit_count -= bits; \
  bit_cache &= bit_mask[bit_count];

#define GET_UNARY(value) \
  value = 0; \
  while(bit_cache==0){ \
    value += bit_count; \
    bit_cache = *bit_pos++; \
    bit_count = 16; \
  } \
  while ((bit_cache & 1)==0) { \
    value++; \
    bit_cache >>= 1; \
    bit_count--; \
  } \
  bit_cache >>= 1; \
  bit_count--;

#define setup_buffer_read(pCodeBuf) \
unsigned int bit_count=0; \
unsigned int bit_cache=0; \
unsigned short *bit_pos=(unsigned short *)pCodeBuf;

#define done_buffer_read() /* bit_pos += 4; */ // skip CRC32

/************************* decoder functions ****************************/

void TTAC_Decode_8bit (u8 *pCodeBuf,s8 *buf,u32 DecompressedSamplesCount)
{
  setup_buffer_read(pCodeBuf);
  
  int tta_last=0;
  unsigned int tta_rice_k0=10;
  unsigned int tta_rice_k1=10;
  unsigned int tta_rice_sum0=shift_16[10];
  unsigned int tta_rice_sum1=shift_16[10];
  
  fltst fst;
  
  filter_init(&fst);
  
  for (;DecompressedSamplesCount!=0;DecompressedSamplesCount--) {
    int value;
    
    // decode Rice unsigned
    unsigned int unary;
    GET_UNARY(unary);
    
    unsigned int k;
    bool depth;
    
    if(unary==0) {
      depth = false; k = tta_rice_k0;
      }else{
      depth = true; k = tta_rice_k1;
      unary--;
    }
    
    if (k!=0) {
      unsigned int binary;
      GET_BINARY(binary, k);
      value = (unary << k) + binary;
      }else{
      value = unary;
    }
    
    if(depth==true){
      tta_rice_sum1 += value - (tta_rice_sum1 >> 4);
      if (tta_rice_k1 > 0 && tta_rice_sum1 < shift_16[tta_rice_k1]){
        tta_rice_k1--;
        }else{
        if (tta_rice_sum1 > shift_16[tta_rice_k1 + 1]) tta_rice_k1++;
      }
      value += bit_shift[tta_rice_k0];
    }
    
    {
      tta_rice_sum0 += value - (tta_rice_sum0 >> 4);
      if (tta_rice_k0 > 0 && tta_rice_sum0 < shift_16[tta_rice_k0]){
        tta_rice_k0--;
        }else{
        if (tta_rice_sum0 > shift_16[tta_rice_k0 + 1]) tta_rice_k0++;
      }
    }

    value = DEC(value);

    // decompress stage 1: adaptive hybrid filter
    hybrid_filter_8bit(&fst, &value);

    // decompress stage 2: fixed order 1 prediction
    value += PREDICTOR1(tta_last, 4);	// bps 8
    tta_last = value;

    *buf++ = value;
  }
  
  done_buffer_read();
}

void TTAC_Decode_16bit (u8 *pCodeBuf,s16 *buf,u32 DecompressedSamplesCount)
{
  setup_buffer_read(pCodeBuf);
  
  int tta_last=0;
  unsigned int tta_rice_k0=10;
  unsigned int tta_rice_k1=10;
  unsigned int tta_rice_sum0=shift_16[10];
  unsigned int tta_rice_sum1=shift_16[10];
  
  fltst fst;
  
  filter_init(&fst);
  
  for (;DecompressedSamplesCount!=0;DecompressedSamplesCount--) {
    int value;
    
    // decode Rice unsigned
    unsigned int unary;
    GET_UNARY(unary);
    
    unsigned int k;
    bool depth;
    
    if(unary==0) {
      depth = false; k = tta_rice_k0;
      }else{
      depth = true; k = tta_rice_k1;
      unary--;
    }
    
    if (k!=0) {
      unsigned int binary;
      GET_BINARY(binary, k);
      value = (unary << k) + binary;
      }else{
      value = unary;
    }
    
    if(depth==true){
      tta_rice_sum1 += value - (tta_rice_sum1 >> 4);
      if (tta_rice_k1 > 0 && tta_rice_sum1 < shift_16[tta_rice_k1]){
        tta_rice_k1--;
        }else{
        if (tta_rice_sum1 > shift_16[tta_rice_k1 + 1]) tta_rice_k1++;
      }
      value += bit_shift[tta_rice_k0];
    }
    
    {
      tta_rice_sum0 += value - (tta_rice_sum0 >> 4);
      if (tta_rice_k0 > 0 && tta_rice_sum0 < shift_16[tta_rice_k0]){
        tta_rice_k0--;
        }else{
        if (tta_rice_sum0 > shift_16[tta_rice_k0 + 1]) tta_rice_k0++;
      }
    }

    value = DEC(value);

    // decompress stage 1: adaptive hybrid filter
    hybrid_filter_16bit(&fst, &value);

    // decompress stage 2: fixed order 1 prediction
    value += PREDICTOR1(tta_last, 5);	// bps 16
    tta_last = value;

    *buf++ = value;
  }
  
  done_buffer_read();
}

#endif
