#include "Types.h"

#ifndef ICON_BASE_H
#define ICON_BASE_H

class CIconBase
{
public:
	CIconBase(void){};
	CIconBase(float initPosX, float initPosY, int textureID);
	~CIconBase(void);

	virtual void Render(void){};
	virtual void OnTouch(void){};
	virtual void OnDoubleTouch(void){};
	virtual bool IsTouch(float stylusPosX, float stylusPosY);

protected:
	int			m_ID;		// 아이콘의 식별자
	u16			m_TexID;	// 아이콘 텍스쳐의 식별자
	DSVECTOR	m_Pos;		// 아이콘의 위치
	DSSIZE		m_Size;
};


#endif