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

	bool DoesAddInst(void){	return m_DoesAddInst;	}				// �Ǳ⸦ �߰��ؾ� �ϴ°�?
	void SetAddInstTrue(void){	m_DoesAddInst = true;	}	// �Ǳ� �߰� �÷��׸� true�� ����
	void SetAddInstFalse(void){	m_DoesAddInst = false;	}	// �Ǳ� �߰� �÷��׸� false�� ����

	u8	GetAddInstNum(void){	return m_AddInstrumentNum;	}


	bool IsActivate(void){	return m_IsActivated;	}

private:
	u16		m_uTextureID[MAX_INSTRUMENT];	// �߰��� �� �ִ� �Ǳ��� ����ŭ�� �Ǳ� �ؽ���
	bool	m_IsActivated;	// �������� �������� Ȱ��ȭ �Ǿ���.
	bool	m_DoesAddInst;
	u8		m_AddInstrumentNum;
	
};

#endif
