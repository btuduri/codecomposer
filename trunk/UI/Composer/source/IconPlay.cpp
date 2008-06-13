#include "IconPlay.h"

CIconPlay::CIconPlay(s16 initPosX, s16 initPosY, u8 textureID)
		:CIconBase(initPosX, initPosY, textureID)
{
	m_SpriteIndexStart = 32;
	//PA_3DCreateSpriteFromTex(m_ID, m_TexID, m_Size.x, m_Size.y, 
	//						0, m_Pos.x, m_Pos.y); 
}

CIconPlay::~CIconPlay(void)
{
}
