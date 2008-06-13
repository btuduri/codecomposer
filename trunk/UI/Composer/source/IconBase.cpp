
#include "RscManager.h"
#include "IconBase.h"


CIconBase::CIconBase(s16 initPosX, s16 initPosY, u8 textureID)
{
	m_ID	= textureID;
	//m_TexID = g_cRscManager.GetTexture( m_ID );
	m_Size = g_cRscManager.GetSize( m_ID );
	m_Pos.x = initPosX;	
	m_Pos.y = initPosY;
	PA_3DCreateSpriteFromTex(m_ID, g_cRscManager.GetTexture(textureID), m_Size.x, m_Size.y, 
							m_ID, m_Pos.x, m_Pos.y); 


}

CIconBase::~CIconBase(void)
{
}

bool CIconBase::IsTouch(s16 stylusPosX, s16 stylusPosY)
{
	s16 activeRegionLeft = m_Pos.x - m_Size.x/2;
	s16 activeRegionRight = m_Pos.x + m_Size.x/2;
	s16 activeRegionTop = m_Pos.y + m_Size.x/2;
	s16 activeRegionBottom = m_Pos.y - m_Size.x/2;

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
