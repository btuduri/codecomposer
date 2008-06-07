#include "IconBase.h"

#ifndef ICON_PLUS_H
#define ICON_PLUS_H

class CIconPlus :
	public CIconBase
{
public:
	CIconPlus(void);
	CIconPlus(float initPosX = 0.0f, float initPosY = 0.0f, int textureID = TEX_PLUS);
	~CIconPlus(void);

	void OnTouch(void);

	inline bool IsActivate(void){	return m_IsActivated;	}

private:
	u16		m_uTextureID[MAX_INSTRUMENT];	// 추가할 수 있는 악기의 수만큼의 악기 텍스쳐
	bool	m_IsActivated;	// 아이콘이 눌러져서 활성화 되었다.
};

#endif
