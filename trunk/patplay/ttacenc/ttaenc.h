/*
 * ttaenc.h
 *
 * Description: TTAv1 encoder definitions and prototypes
 * Copyright (c) 2007, Aleksander Djuric (ald@true-audio.com)
 * Distributed under the GNU General Public License (GPL).
 * The complete text of the license can be found in the
 * COPYING file included in the distribution.
 *
 */

#ifndef TTAENC_H
#define TTAENC_H

#define MAX_ORDER		16
#define BIT_BUFFER_SIZE (1024*1024)

#ifdef _WIN32
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
#else
typedef __uint32_t uint32;
typedef __uint64_t uint64;
#endif

#define PREDICTOR1(x, k)	((int)((((uint64)x << k) - x) >> k))
#define ENC(x)  (((x)>0)?((x)<<1)-1:(-(x)<<1))
#define DEC(x)  (((x)&1)?(++(x)>>1):(-(x)>>1))

typedef struct {
	unsigned int k0;
	unsigned int k1;
	unsigned int sum0;
	unsigned int sum1;
} adapt;

typedef struct {
	int shift;
	int round;
	int error;
	int qm[MAX_ORDER];
	int dx[MAX_ORDER];
	int dl[MAX_ORDER];
} fltst;

typedef struct {
	fltst fst;
	adapt rice;
	int last;
} encoder;

#endif	/* TTAENC_H */
