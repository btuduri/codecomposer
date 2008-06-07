#include "IconPlus.h"

CIconPlus::CIconPlus(float initPosX, float initPosY, int textureID)
			:CIconBase(initPosX, initPosY, textureID)
{
	
}

CIconPlus::~CIconPlus(void)
{
}


void CIconPlus::OnTouch(void)
{
	if( !m_IsActivated )
	{
		float posX = m_Pos.x;
		float posY = m_Pos.y - m_Size.y;
		for(int i = 0; i < MAX_INSTRUMENT; ++i )
		{
			if( i%6 == 0 ){	posX = m_Pos.x;	}
			posX = m_Pos.x + 32*(i%8);
			posY = (m_Pos.y - m_Size.y) - 32*(i/8);
			PA_3DCreateSpriteFromTex(i+10, m_TexID, 32, 32, m_ID, posX, posY);
		}

		m_IsActivated = true;
	}
	else
	{


	}

}