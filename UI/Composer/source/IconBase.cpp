
#include "RscManager.h"
#include "IconBase.h"


CIconBase::CIconBase(float initPosX, float initPosY, int textureID)
{
	m_ID	= textureID;
	m_TexID = g_cRscManager.GetTexture( m_ID );
	m_Size = g_cRscManager.GetSize( m_ID );
	m_Pos.x = initPosX;	
	m_Pos.y = initPosY;

	PA_3DCreateSpriteFromTex(m_ID, m_TexID, m_Size.x, m_Size.y, 
							m_ID, m_Pos.x, m_Pos.y); 
}

CIconBase::~CIconBase(void)
{
}

bool CIconBase::IsTouch(float stylusPosX, float stylusPosY)
{
	float activeRegionLeft = m_Pos.x - m_Size.x/2;
	float activeRegionRight = m_Pos.x + m_Size.x/2;
	float activeRegionTop = m_Pos.y + m_Size.x/2;
	float activeRegionBottom = m_Pos.y - m_Size.x/2;

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
