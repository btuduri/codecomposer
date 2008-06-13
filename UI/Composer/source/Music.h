#include "Types.h"

//#include "gfx/all_gfx.c"
#include "gfx/all_gfx.h"

#ifndef MUSIC_H
#define MUSIC_H

typedef struct _NOTE
{
	u8			noteType;
	DSVECTOR	pos;			// ȭ�� ���� �׷����� ��ġ
	//DSRECTANGLE	boundingBox;

	// ���� ���̴� ���Ŀ� ���⿡ �߰��Ѵ�.

}NOTE;

typedef struct _INSTRUMENT
{
	u8		m_InstrumentTexType;
	u8		m_SpriteIndexStart;		// �߰��� �̹����� ��������Ʈ �ν� ��ȣ ������ġ
	u8		m_SpriteID[32];			// 0 : �Ǳ� ������ �ĺ� ��ȣ, 1 : �������� �ĺ� ��ȣ
									// 2 ~ : ��ǥ�� �ĺ���ȣ�� �ִ� ȭ�鿡 30���� ��ǥ�� ��Ÿ�� �� ����  

	u16			m_NumNotes;
	NOTE		m_Melody[MAX_NOTES];

	
	s16		m_PosY;					// ���� ȭ�鿡 �׷��� �� �Ǳ� �Ǻ�( ������ + �Ǻ� ��Ʈ) �»�� ��ġ

	void Create(u8 instrumentTexType, u8 sheetTexType, u16 spriteIndexStart, s16 posY)
	{
		m_InstrumentTexType = instrumentTexType;
		m_SpriteIndexStart = spriteIndexStart;
		m_NumNotes = 0;
		
		m_PosY = posY;
		

		// Draw Instrument Icon
		PA_3DCreateSpriteFromTex(m_SpriteIndexStart + 0, g_cRscManager.GetTexture(instrumentTexType), 32, 32, 
								0, 16.0, m_PosY + 16.0 + 10.0); 
	

		// Draw sheet image
		for(u8 i = 0; i < 4; ++i)
		{
			PA_3DCreateSpriteFromTex(m_SpriteIndexStart + 1 + i, g_cRscManager.GetTexture(sheetTexType), 64, 64, 
									0, 68.0 + 64.0*i, m_PosY + 32.0 + 10.0); 
		}

		
		//PA_OutputText(1,0,1," index ; %d %d", m_SpriteIndexStart, sheetTexType);
	}

}INSTRUMENT;


class CMusic
{
public:
	CMusic(void);
	~CMusic(void);
	void Initialize(void);

	//inline bool IsFull(void){	return ((m_NumInstrument < MAX_ADD_INSTRUMENT)? false: true);	}
	bool IsFull(void)
	{
		if( m_NumInstrument < MAX_ADD_INSTRUMENT )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	bool AddInstrument( u8 instrumentType );
	void DelInstrument( u8 instrumentIndexNum );

private:
	u8			m_NumInstrument;	// ���� �߰��� �Ǳ��� ��
	u8			m_SelectedInstrument;	// ���õ� �Ǳ� ��ȣ, 0�̸� �ƹ� �Ǳ⵵ ���õ��� ����
	u8			m_SpriteIndexStart;		// �߰��� �̹����� ��������Ʈ �ν� ��ȣ ������ġ
	//INSTRUMENT	m_Instrument[MAX_ADD_INSTRUMENT];
	INSTRUMENT	m_Instrument[5];

	s16		m_LastBottomLine;
};


#endif