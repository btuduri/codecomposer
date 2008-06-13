#include "RscManager.h"

// Include Resource image
// Converted using PAGfx
#include "gfx/all_gfx.c"
#include "gfx/all_gfx.h"


CRscManager::CRscManager(void)
{

}


CRscManager::~CRscManager(void)
{

}


void CRscManager::LoadTexture(void)
{
	
	// First, create the gfx with the corresponding images and sizes. Images converted as 16bit sprites in PAGfx
	// Piano Icon Texture
	m_Size[TEX_PIANO].x = 32;	m_Size[TEX_PIANO].y = 32;
	m_uTextureID[TEX_PIANO] = PA_3DCreateTex((void*)Icon_Piano_Texture, m_Size[TEX_PIANO].x, m_Size[TEX_PIANO].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Guitar Icon Texture
	m_Size[TEX_GUITAR].x = 32;	m_Size[TEX_GUITAR].y = 32;
	m_uTextureID[TEX_GUITAR] = PA_3DCreateTex((void*)Icon_Guitar_Texture, m_Size[TEX_GUITAR].x, m_Size[TEX_GUITAR].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Base Guitar Icon Texture
	m_Size[TEX_BGUITAR].x = 32;	m_Size[TEX_BGUITAR].y = 32;
	m_uTextureID[TEX_BGUITAR] = PA_3DCreateTex((void*)Icon_Bguitar_Texture, m_Size[TEX_BGUITAR].x, m_Size[TEX_BGUITAR].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Electric Guitar Icon Texture
	m_Size[TEX_EGUITAR].x = 32;	m_Size[TEX_EGUITAR].y = 32;
	m_uTextureID[TEX_EGUITAR] = PA_3DCreateTex((void*)Icon_Eguitar_Texture, m_Size[TEX_EGUITAR].x, m_Size[TEX_EGUITAR].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Violin Icon Texture
	m_Size[TEX_VIOLIN].x = 32;	m_Size[TEX_VIOLIN].y = 32;
	m_uTextureID[TEX_VIOLIN] = PA_3DCreateTex((void*)Icon_Violin_Texture, m_Size[TEX_VIOLIN].x, m_Size[TEX_VIOLIN].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Drum Icon Texture
	m_Size[TEX_DRUM].x = 32;	m_Size[TEX_DRUM].y = 32;
	m_uTextureID[TEX_DRUM] = PA_3DCreateTex((void*)Icon_Drum_Texture, m_Size[TEX_DRUM].x, m_Size[TEX_DRUM].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Flute Icon Texture
	m_Size[TEX_FLUTE].x = 32;	m_Size[TEX_FLUTE].y = 32;
	m_uTextureID[TEX_FLUTE] = PA_3DCreateTex((void*)Icon_Flute_Texture, m_Size[TEX_FLUTE].x, m_Size[TEX_FLUTE].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Trumpet Icon Texture
	m_Size[TEX_TRUMPET].x = 32;	m_Size[TEX_TRUMPET].y = 32;
	m_uTextureID[TEX_TRUMPET] = PA_3DCreateTex((void*)Icon_Trumpet_Texture, m_Size[TEX_TRUMPET].x, m_Size[TEX_TRUMPET].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	// Sheet5 Texture
	m_Size[TEX_SHEET5].x = 64;	m_Size[TEX_SHEET5].y = 64;
	m_uTextureID[TEX_SHEET5] = PA_3DCreateTex((void*)Sheet_Piano_Texture, m_Size[TEX_SHEET5].x, m_Size[TEX_SHEET5].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLUS, (void*)IconPlus_Pal);

	// Line Texture
	m_Size[TEX_LINE].x = 8;	m_Size[TEX_LINE].y = 8;
	m_uTextureID[TEX_LINE] = PA_3DCreateTex((void*)Signal_Line_Texture, m_Size[TEX_LINE].x, m_Size[TEX_LINE].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLUS, (void*)IconPlus_Pal);

	// Plus Icon Texture
	m_Size[TEX_PLUS].x = 32;	m_Size[TEX_PLUS].y = 32;
	m_uTextureID[TEX_PLUS] = PA_3DCreateTex((void*)Icon_Plus_Texture, m_Size[TEX_PLUS].x, m_Size[TEX_PLUS].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLUS, (void*)IconPlus_Pal);

	// Play Icon Texture
	m_Size[TEX_PLAY].x = 32;	m_Size[TEX_PLAY].y = 32;
	m_uTextureID[TEX_PLAY] = PA_3DCreateTex((void*)Icon_Play_Texture, m_Size[TEX_PLAY].x, m_Size[TEX_PLAY].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	return ;
}