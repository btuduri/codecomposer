#include "RscManager.h"
#include "Music.h"



CMusic::CMusic(void)
{
	m_SpriteIndexStart = 64;
	m_NumInstrument = 0;
	m_SelectedInstrument = 0; // 0이면 아무것도 선택되지 않았음
	m_LastBottomLine = 0.0;
}

CMusic::~CMusic(void)
{
}

void CMusic::Initialize(void)
{
	m_SpriteIndexStart = 64;
	m_NumInstrument = 0;
	m_SelectedInstrument = 0; // 0이면 아무것도 선택되지 않았음
	m_LastBottomLine = 0.0;
}

bool CMusic::AddInstrument(u8 instrumentType)
{
	// Is it possible that Adding instrument?
	if( IsFull() )
	{
		PA_OutputText(1,0,1,"m_Inst : %d", m_NumInstrument);
		return false;
	}
	
	PA_OutputText(1,0,1," Add inst");

	switch( instrumentType )
	{
		case TEX_PIANO:
			m_Instrument[m_NumInstrument].Create( TEX_PIANO, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_GUITAR:
			m_Instrument[m_NumInstrument].Create( TEX_GUITAR, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_BGUITAR:
			m_Instrument[m_NumInstrument].Create( TEX_BGUITAR, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_EGUITAR:
			m_Instrument[m_NumInstrument].Create( TEX_EGUITAR, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_VIOLIN:
			m_Instrument[m_NumInstrument].Create( TEX_VIOLIN, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_DRUM:
			m_Instrument[m_NumInstrument].Create( TEX_DRUM, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_FLUTE:
			m_Instrument[m_NumInstrument].Create( TEX_FLUTE, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		case TEX_TRUMPET:
			m_Instrument[m_NumInstrument].Create( TEX_TRUMPET, TEX_SHEET5, 64 * (m_NumInstrument+1), m_LastBottomLine);
			break;
		default:
			break;
	}

	m_LastBottomLine += 10 + 35 + 10;
	++m_NumInstrument;
	return true;
}