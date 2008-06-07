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


int CRscManager::LoadTexture(void)
{
	// Plus Icon Texture
	// First, create the gfx with the corresponding images and sizes. Images converted as 16bit sprites in PAGfx
	m_Size[TEX_PLUS].x = 32;	m_Size[TEX_PLUS].y = 32;
	m_uTextureID[TEX_PLUS] = PA_3DCreateTex((void*)IconPlus_Texture, m_Size[TEX_PLUS].x, m_Size[TEX_PLUS].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLUS, (void*)IconPlus_Pal);

	// Play Icon Texture
	m_Size[TEX_PLAY].x = 32;	m_Size[TEX_PLAY].y = 32;
	m_uTextureID[TEX_PLAY] = PA_3DCreateTex((void*)IconPlay_Texture, m_Size[TEX_PLAY].x, m_Size[TEX_PLAY].y, TEX_16BITS);	
	//PA_Load3DSpritePal(TEX_PLAY, (void*)IconPlay_Pal);

	return TRUE;
}