#include "IconBase.h"

#ifndef ICON_PLAY_H
#define ICON_PLAY_H

class CIconPlay :
	public CIconBase
{
public:
	CIconPlay(void);
	CIconPlay(s16 initPosX = 0.0f, s16 initPosY = 0.0f, u8 textureID = TEX_PLAY);
	~CIconPlay(void);
};

#endif