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
	u16		m_uTextureID[MAX_INSTRUMENT];	// �߰��� �� �ִ� �Ǳ��� ����ŭ�� �Ǳ� �ؽ���
	bool	m_IsActivated;	// �������� �������� Ȱ��ȭ �Ǿ���.
};

#endif
