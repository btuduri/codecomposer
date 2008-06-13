#include "IconPlus.h"

CIconPlus::CIconPlus(s16 initPosX, s16 initPosY, u8 textureID)
			:CIconBase(initPosX, initPosY, textureID)
{
	m_SpriteIndexStart = 16;
	m_AddInstrumentNum = 0x00;

	m_DoesAddInst = false;

	
}

CIconPlus::~CIconPlus(void)
{
}


void CIconPlus::OnTouch(void)
{
	if( !m_IsActivated )
	{
		s16 posX = m_Pos.x;
		s16 posY = m_Pos.y - m_Size.y;
		
		for(u8 i = 0x00; i < MAX_INSTRUMENT; ++i )
		{
			if( i%8 == 0 ){	posX = m_Pos.x;	}
			posX = m_Pos.x + 32*(i%8);
			posY = (m_Pos.y - m_Size.y - 5) - 32*(i/8);
			PA_3DCreateSpriteFromTex(m_SpriteIndexStart + i, i, 32, 32, m_ID, posX, posY);
			
		}

		m_IsActivated = true;
	}
	else
	{
		// 악기를 추가하는 코드를 넣기

		OffTouch();
	}
}


void CIconPlus::OffTouch(void)
{
	for(u8 i = 0; i < MAX_INSTRUMENT; ++i )
	{
		PA_3DDeleteSprite(m_SpriteIndexStart + i);
	}

	m_IsActivated = false;
}


bool CIconPlus::IsTouch(s16 stylusPosX, s16 stylusPosY)
{
	s16 activeRegionLeft = m_Pos.x - m_Size.x/2;
	s16 activeRegionRight = m_Pos.x + m_Size.x/2;
	s16 activeRegionTop = m_Pos.y + m_Size.x/2;
	s16 activeRegionBottom = m_Pos.y - m_Size.x/2;

	if( !m_IsActivated )
	{
		if( (stylusPosX > activeRegionLeft && stylusPosX < activeRegionRight) &&
			(stylusPosY > activeRegionBottom && stylusPosY < activeRegionTop) )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		
		if( (stylusPosX > activeRegionLeft && stylusPosX < activeRegionRight) &&
			(stylusPosY > activeRegionBottom && stylusPosY < activeRegionTop) )
		{
			 
			return false;
		}
		else
		{
			activeRegionTop -= m_Size.y;
			activeRegionBottom -= m_Size.y;

			for(u8 i=0; i< MAX_INSTRUMENT; ++i)
			{
				if( (stylusPosX > activeRegionLeft && stylusPosX < activeRegionRight) &&
					(stylusPosY > activeRegionBottom && stylusPosY < activeRegionTop) )
				{
					m_AddInstrumentNum = i;
					//PA_OutputText(1, 10, 10, "%d", i);
					SetAddInstTrue();	// 악기를 추가 해야한다.
					return true;
				}

				activeRegionLeft += m_Size.x;
				activeRegionRight += m_Size.x;
			}

			return false;
		}

	}
}