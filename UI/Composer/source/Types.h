#include <PA9.h>

#ifndef TYPES_H
#define TYPES_H

static const u8 MAX_TEXTURE = 16;
static const u8 MAX_INSTRUMENT = 8;		// 악기의 종류
static const u8 MAX_ADD_INSTRUMENT = 8;	// 최대 추가할 수 있는 악기의 수
static const u16 MAX_NOTES = 0xffff;	// 최대 음표 수



enum {  TEX_PIANO = 0, TEX_GUITAR ,TEX_BGUITAR , TEX_EGUITAR, TEX_VIOLIN,
		TEX_DRUM, TEX_FLUTE, TEX_TRUMPET, 
		TEX_PLUS, TEX_PLAY,
		TEX_SHEET5, TEX_SHEET2, TEX_LINE};

//==============================================
// DSVECTOR
typedef struct _DSVECTOR 
{
    s16 x;
    s16 y;
} DSVECTOR;


//==============================================
// SIZE
typedef struct _DSSIZE 
{
    u8 x;
	u8 y;

	_DSSIZE& operator = (const _DSSIZE sourceSize )
	{
		this ->x = sourceSize.x;
		this ->y = sourceSize.y;
	};
} DSSIZE;


//==============================================
// DSMATRIX
//typedef struct _DSMATRIX
//{
//	union {
//        struct {
//            s16        _11, _12, _13, _14;
//            s16        _21, _22, _23, _24;
//            s16        _31, _32, _33, _34;
//            s16        _41, _42, _43, _44;
//
//        };
//
//        s16 m[4][4];
//    };
//}DSMATRIX;

//==============================================
// DSBOUNDING BOX
//typedef struct _DSRECTANGLE
//{
//    s16 left;
//    s16 right;
//    s16 top;
//	s16 bottom;
//
//	_DSRECTANGLE& operator = (const _DSRECTANGLE sourceRect )
//	{
//		this ->left = sourceRect.left;
//		this ->right = sourceRect.right;
//		this ->top = sourceRect.top;
//		this ->bottom = sourceRect.bottom;
//	};
//} DSRECTANGLE;


#endif