//Gfx converted using Mollusk's PAGfx Converter

//This file contains all the .c, for easier inclusion in a project

#ifdef __cplusplus
extern "C" {
#endif

#include "all_gfx.h"


// Background files : 
#include "Background.c"

// Sprite files : 
#include "IconPlus.c"
#include "IconPlay.c"

// Palette files : 
#include "Background.pal.c"

// Background Pointers :
PAGfx_struct Background = {(void*)Background_Map, 768, (void*)Background_Tiles, 832, (void*)Background_Pal, (int*)Background_Info };


#ifdef __cplusplus
}
#endif

