#include "Types.h"

//#include "gfx/all_gfx.c"
#include "gfx/all_gfx.h"

#ifndef MUSIC_H
#define MUSIC_H

typedef struct _NOTE
{
	u8			noteType;
	DSVECTOR	pos;			// 화면 상의 그려지는 위치
	//DSRECTANGLE	boundingBox;

	// 음의 높이는 추후에 여기에 추가한다.

}NOTE;

typedef struct _INSTRUMENT
{
	u8		m_InstrumentTexType;
	u8		m_SpriteIndexStart;		// 추가될 이미지의 스프라이트 인식 번호 시작위치
	u8		m_SpriteID[32];			// 0 : 악기 아이콘 식별 번호, 1 : 오선지의 식별 번호
									// 2 ~ : 음표의 식별번호로 최대 화면에 30개의 음표가 나타날 수 있음  

	u16			m_NumNotes;
	NOTE		m_Melody[MAX_NOTES];

	
	s16		m_PosY;					// 실제 화면에 그려질 한 악기 악보( 아이콘 + 악보 시트) 좌상단 위치

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
	u8			m_NumInstrument;	// 현재 추가된 악기의 수
	u8			m_SelectedInstrument;	// 선택된 악기 번호, 0이면 아무 악기도 선택되지 않음
	u8			m_SpriteIndexStart;		// 추가될 이미지의 스프라이트 인식 번호 시작위치
	//INSTRUMENT	m_Instrument[MAX_ADD_INSTRUMENT];
	INSTRUMENT	m_Instrument[5];

	s16		m_LastBottomLine;
};


#endif