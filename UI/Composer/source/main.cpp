// PALib Template Application

// Includes
#include <PA9.h>       // Include for PA_Lib
#include "Camera.h"
#include "RscManager.h"
#include "IconPlus.h"
#include "IconPlay.h"
#include "Music.h"

// Converted using PAGfx
//#include "gfx/all_gfx.c"
#include "gfx/all_gfx.h"


//==================================================
// Declaration Function
void Initialize(void);
void FrameUpdate(void);
void FrameRender(void);
void Destroy(void);


//==================================================
// Global variables
CCamera			g_cCamera;
CRscManager		g_cRscManager;


CIconPlus	*g_pIconPlus;
CIconPlay	*g_pIconPlay;

CMusic		*g_pMusic;





//==================================================
// Main Fuction
int main()
{
	PA_Init();    // Initializes PA_Lib
	PA_InitVBL(); // Initializes a standard VBL
	PA_Init3D(); // Uses Bg0
	PA_Reset3DSprites();

	//PA_InitText(0, 1);
	//PA_OutputSimpleText(0, 7, 22, "Song Title!");

	
	PA_Init16cBg(0, 1); // 16 color background init with default colors for text
	PA_InitText(1, 1);

	Initialize();

	PA_16cText(0, 45, 170, 255, 190, "Song Title", 1, 4, 100); 
		// #screen, x1, y1, x2, y2, Text string, color(1-10), text size(0-4), max Characters

	// Load Backgrounds with their palettes !
	PA_EasyBgLoad(0, 3,	Background); // #Screen, #Background, Background name
	PA_EasyBgLoad(1, 3, Background); // #Screen, #Background, Background name


	// Infinite loop to keep the program running
	while (1)
	{
		FrameUpdate();
		FrameRender();

		PA_3DProcess();  // Update sprites
		PA_WaitForVBL();
	}
	
	Destroy();
	return 0;
}


//==================================================
// Initialize
// Desc : Initialize whatever
void Initialize(void)
{
	// Load texture
	g_cRscManager.LoadTexture();

	// Create Icons
	g_pIconPlus = new CIconPlus(16, 175);
	g_pIconPlay = new CIconPlay(239, 175);

	// Create Music
	g_pMusic	= new CMusic();

}


//==================================================
// Destroy
// Desc : 매 프레임 화면을 그려준다.
void Destroy(void)
{
	delete g_pIconPlus;
	delete g_pIconPlay;
	delete g_pMusic;

}


//==================================================
// FrameUpdate
// Desc : 매 프레임 마다 값을 갱신해주어야할
//		  경우 여기서 처리한다.
void FrameUpdate(void)
{
	// 스타일러스 입력 처리
	if( Stylus.Newpress )
	{
		if( g_pIconPlus ->IsTouch( Stylus.X, Stylus.Y ) )
		{
			g_pIconPlus ->OnTouch();
		}
		else
		{
			g_pIconPlus ->OffTouch();
		}
	}
	

	if( g_pIconPlus ->DoesAddInst() )
	{
		g_pMusic ->AddInstrument( g_pIconPlus ->GetAddInstNum() );
		
		g_pIconPlus ->SetAddInstFalse();
	}
}


//==================================================
// FrameRender
// Desc : 매 프레임 화면을 그려준다.
void FrameRender(void)
{


}


