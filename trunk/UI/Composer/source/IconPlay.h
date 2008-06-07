#include "IconBase.h"

#ifndef ICON_PLAY_H
#define ICON_PLAY_H

class CIconPlay :
	public CIconBase
{
public:
	CIconPlay(void);
	CIconPlay(float initPosX = 0.0f, float initPosY = 0.0f, int textureID = TEX_PLAY);
	~CIconPlay(void);
};

#endif