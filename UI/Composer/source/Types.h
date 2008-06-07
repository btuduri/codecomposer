#include <PA9.h>

#ifndef TYPES_H
#define TYPES_H

static const u8 MAX_TEXTURE = 0xff;
static const u8 MAX_INSTRUMENT = 8;

enum { TEX_PLUS = 0, TEX_PLAY, TEX_PIANO, TEX_GUITAR };

//==============================================
// DSVECTOR
typedef struct _DSVECTOR {
    float x;
    float y;
    float z;
} DSVECTOR;


//==============================================
// SIZE
typedef struct _DSSIZE {
    int x;
	int y;

	_DSSIZE& operator = (const _DSSIZE sourceSize )
	{
		this ->x = sourceSize.x;
		this ->y = sourceSize.y;
	};
} DSSIZE;


//==============================================
// DSMATRIX
typedef struct _DSMATRIX
{
	union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };

        float m[4][4];
    };
}DSMATRIX;


#endif