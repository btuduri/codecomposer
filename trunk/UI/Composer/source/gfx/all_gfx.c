//Gfx converted using Mollusk's PAGfx Converter

//This file contains all the .c, for easier inclusion in a project

#ifdef __cplusplus
extern "C" {
#endif

#include "all_gfx.h"


// Background files : 
#include "Background.c"

// Sprite files : 
#include "Signal_Line.c"
#include "Icon_Bguitar.c"
#include "Icon_Drum.c"
#include "Icon_Eguitar.c"
#include "Icon_Flute.c"
#include "Icon_Guitar.c"
#include "Icon_Piano.c"
#include "Icon_Play.c"
#include "Icon_Plus.c"
#include "Icon_Trumpet.c"
#include "Icon_Violin.c"
#include "Sheet_Piano.c"

// Palette files : 
#include "Background.pal.c"

// Background Pointers :
PAGfx_struct Background = {(void*)Background_Map, 768, (void*)Background_Tiles, 832, (void*)Background_Pal, (int*)Background_Info };


#ifdef __cplusplus
}
#endif

