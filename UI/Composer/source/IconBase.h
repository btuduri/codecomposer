#include "Types.h"

#ifndef ICON_BASE_H
#define ICON_BASE_H

class CIconBase
{
public:
	CIconBase(void){};
	CIconBase(s16 initPosX, s16 initPosY, u8 textureID);
	~CIconBase(void);

	virtual void Render(void){};
	virtual void OnTouch(void){};
	virtual void OffTouch(void){};
	virtual void OnDoubleTouch(void){};
	virtual bool IsTouch(s16 stylusPosX, s16 stylusPosY);

protected:
	u8			m_ID;		// �������� �ĺ���
	//u16			m_TexID;	// ������ �ؽ����� �ĺ���
	DSVECTOR	m_Pos;		// �������� ��ġ
	DSSIZE		m_Size;
	u16			m_SpriteIndexStart;
};


#endif