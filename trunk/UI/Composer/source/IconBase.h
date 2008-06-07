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
	int			m_ID;		// �������� �ĺ���
	u16			m_TexID;	// ������ �ؽ����� �ĺ���
	DSVECTOR	m_Pos;		// �������� ��ġ
	DSSIZE		m_Size;
};


#endif