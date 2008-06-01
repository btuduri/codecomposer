/*
 * ttaenc.c
 *
 * Description: TTAv1 lossless audio encoder/decoder.
 * Copyright (c) 2007, Aleksander Djuric (ald@true-audio.com)
 * Distributed under the GNU General Public License (GPL).
 * The complete text of the license can be found in the
 * COPYING file included in the distribution.
 *
 */

#include "ttaenc.h"

/******************* static variables and structures *******************/

static unsigned char BIT_BUFFER[BIT_BUFFER_SIZE + 8];
static unsigned char *BIT_BUFFER_END = BIT_BUFFER + BIT_BUFFER_SIZE;

static uint32 bit_cache;
static uint32 bit_count;

static unsigned char *bitpos;

static int outputsize;
static unsigned char *poutputbuf;

static const uint32 bit_mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

static const uint32 bit_shift[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000
};

static const uint32 *shift_16 = bit_shift + 4;

/************************* bit operations ******************************/

void init_buffer_write(void) {
	bit_count = bit_cache = 0;
	bitpos = BIT_BUFFER;
}

__inline void put_binary(unsigned int value, unsigned int bits) {
	while (bit_count >= 8) {
		if (bitpos == BIT_BUFFER_END) {
			int idx;
			unsigned char *psrc=BIT_BUFFER;
			for(idx=0;idx<BIT_BUFFER_SIZE;idx++){
			  *poutputbuf++ = *psrc++;
			  outputsize++;
			}
			bitpos = BIT_BUFFER;
		}

		*bitpos = (unsigned char) (bit_cache & 0xFF);
		bit_cache >>= 8;
		bit_count -= 8;
		bitpos++;
	}

	bit_cache |= (value & bit_mask[bits]) << bit_count;
	bit_count += bits;
}

__inline void put_unary(unsigned int value) {
	while (value) {
		while (bit_count >= 8) {
			if (bitpos == BIT_BUFFER_END) {
				int idx;
				unsigned char *psrc=BIT_BUFFER;
				for(idx=0;idx<BIT_BUFFER_SIZE;idx++){
				  *poutputbuf++ = *psrc++;
				  outputsize++;
				}
				bitpos = BIT_BUFFER;
			}

			*bitpos = (unsigned char) (bit_cache & 0xFF);
			bit_cache >>= 8;
			bit_count -= 8;
			bitpos++;
		}

//		bit_cache |= 1 << bit_count;
		bit_count += 1;
		value -= 1;
		
	}
	
	while (bit_count >= 8) {
		if (bitpos == BIT_BUFFER_END) {
			int idx;
			unsigned char *psrc=BIT_BUFFER;
			for(idx=0;idx<BIT_BUFFER_SIZE;idx++){
			  *poutputbuf++ = *psrc++;
			  outputsize++;
			}
			bitpos = BIT_BUFFER;
		}

		*bitpos = (unsigned char) (bit_cache & 0xFF);
		bit_cache >>= 8;
		bit_count -= 8;
		bitpos++;
	}
	
	bit_cache |= 1 << bit_count;
	bit_count += 1;
}

void done_buffer_write(void) {
	unsigned int res, bytes_to_write;

	while (bit_count) {
		*bitpos = (unsigned char) (bit_cache & 0xFF);
		bit_cache >>= 8;
		bit_count = (bit_count > 8) ? (bit_count - 8) : 0;
		bitpos++;
	}

	bytes_to_write = bitpos - BIT_BUFFER;
	{
		unsigned int idx;
		unsigned char *psrc=BIT_BUFFER;
		for(idx=0;idx<bytes_to_write;idx++){
		  *poutputbuf++ = *psrc++;
		  outputsize++;
		}
	}
	bitpos = BIT_BUFFER;

}

int done_buffer_read() {
	bit_cache = bit_count = 0;

	return 0;
}

/************************* filter functions ****************************/

__inline void memshl (register int *pA, register int *pB) {
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA   = *pB;
}

__inline void hybrid_filter (fltst *fs, int *in, int mode) {
	register int *pA = fs->dl;
	register int *pB = fs->qm;
	register int *pM = fs->dx;
	register int sum = fs->round;

	if (!fs->error) {
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++;
		sum += *pA++ * *pB, pB++; pM += 8;
	} else if (fs->error < 0) {
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
		sum += *pA++ * (*pB -= *pM++), pB++;
	} else {
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
		sum += *pA++ * (*pB += *pM++), pB++;
	}

	*(pM-0) = ((*(pA-1) >> 30) | 1) << 2;
	*(pM-1) = ((*(pA-2) >> 30) | 1) << 1;
	*(pM-2) = ((*(pA-3) >> 30) | 1) << 1;
	*(pM-3) = ((*(pA-4) >> 30) | 1);

	if (mode) {
		*pA = *in;
		*in -= (sum >> fs->shift);
		fs->error = *in;
	} else {
		fs->error = *in;
		*in += (sum >> fs->shift);
		*pA = *in;
	}

	*(pA-1) = *(pA-0) - *(pA-1);
	*(pA-2) = *(pA-1) - *(pA-2);
	*(pA-3) = *(pA-2) - *(pA-3);

	memshl (fs->dl, fs->dl + 1);
	memshl (fs->dx, fs->dx + 1);
}

void filter_init (fltst *fs, int shift) {
	{
	  unsigned char *p=(unsigned char *)fs;
	  int idx;
	  for(idx=0;idx<sizeof(fltst);idx++){
	    *p++=0;
	  }
	}
	fs->shift = shift;
	fs->round = 1 << (shift - 1);
}

/************************* basic functions *****************************/

void rice_init(adapt *rice, unsigned int k0, unsigned int k1) {
	rice->k0 = k0;
	rice->k1 = k1;
	rice->sum0 = shift_16[k0];
	rice->sum1 = shift_16[k1];
}

void encoder_init(encoder *tta, int nch, int byte_size) {
	int flt_set [3] = { 10, 9, 10 };
	int i;

	for (i = 0; i < nch; i++) {
		filter_init(&tta[i].fst, flt_set[byte_size - 1]);
		rice_init(&tta[i].rice, 10, 10);
		tta[i].last = 0;
	}
}

int compress(int *pSourceSamples,int SourceSamplesCount,int BitsPerSample,unsigned char *pCodeBuf)
{
	int *p, prev;
	unsigned int byte_size;
	unsigned int buffer_len;
	encoder ttaenc;

	outputsize=0;
	poutputbuf=pCodeBuf;
	
	byte_size = (BitsPerSample + 7) / 8;
	buffer_len = SourceSamplesCount;

	// init bit writer
	init_buffer_write();
	
	encoder_init(&ttaenc, 1, byte_size);
	
	prev=0;
	
	for (p = pSourceSamples; p < pSourceSamples + buffer_len; p++) {
		fltst *fst = &ttaenc.fst;
		adapt *rice = &ttaenc.rice;
		int *last = &ttaenc.last;

		int tmp;
		unsigned int value, k, unary, binary;
		
		// transform data
		*p -= prev / 2;

		// compress stage 1: fixed order 1 prediction
		tmp = *p;
		switch (byte_size) {
		case 1:	*p -= PREDICTOR1(*last, 4); break;	// bps 8
		case 2:	*p -= PREDICTOR1(*last, 5); break;	// bps 16
		case 3:	*p -= PREDICTOR1(*last, 5); break;	// bps 24
		} *last = tmp;

		// compress stage 2: adaptive hybrid filter
		hybrid_filter(fst, p, 1);

		value = ENC(*p);

		// encode Rice unsigned
		k = rice->k0;

		rice->sum0 += value - (rice->sum0 >> 4);
		if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
			rice->k0--;
		else if (rice->sum0 > shift_16[rice->k0 + 1])
			rice->k0++;

		if (value >= bit_shift[k]) {
			value -= bit_shift[k];
			k = rice->k1;

			rice->sum1 += value - (rice->sum1 >> 4);
			if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
				rice->k1--;
			else if (rice->sum1 > shift_16[rice->k1 + 1])
				rice->k1++;

			unary = 1 + (value >> k);
		} else unary = 0;

		put_unary(unary);
		if (k) {
			binary = value & bit_mask[k];
			put_binary(binary, k);
		}

	}
	
	done_buffer_write();

	return outputsize;
}

