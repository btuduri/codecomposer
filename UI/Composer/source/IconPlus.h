#include "IconBase.h"

#ifndef ICON_PLUS_H
#define ICON_PLUS_H

class CIconPlus :
	public CIconBase
{
public:
	CIconPlus(void);
	CIconPlus(s16 initPosX = 0.0f, s16 initPosY = 0.0f, u8 textureID = TEX_PLUS);
	~CIconPlus(void);

	void OnTouch(void);
	void OffTouch(void);
	bool IsTouch(s16 stylusPosX, s16 stylusPosY);

	bool DoesAddInst(void){	return m_DoesAddInst;	}				// 악기를 추가해야 하는가?
	void SetAddInstTrue(void){	m_DoesAddInst = true;	}	// 악기 추가 플래그를 true로 변경
	void SetAddInstFalse(void){	m_DoesAddInst = false;	}	// 악기 추가 플래그를 false로 변경

	u8	GetAddInstNum(void){	return m_AddInstrumentNum;	}


	bool IsActivate(void){	return m_IsActivated;	}

private:
	u16		m_uTextureID[MAX_INSTRUMENT];	// 추가할 수 있는 악기의 수만큼의 악기 텍스쳐
	bool	m_IsActivated;	// 아이콘이 눌러져서 활성화 되었다.
	bool	m_DoesAddInst;
	u8		m_AddInstrumentNum;
	
};

#endif
