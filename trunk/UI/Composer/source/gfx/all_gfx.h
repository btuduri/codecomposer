//Gfx converted using Mollusk's PAGfx Converter

//This file contains all the .h, for easier inclusion in a project

#ifndef ALL_GFX_H
#define ALL_GFX_H

#ifndef PAGfx_struct
    typedef struct{
    void *Map;
    int MapSize;
    void *Tiles;
    int TileSize;
    void *Palette;
    int *Info;
} PAGfx_struct;
#endif


// Background files : 
extern const int Background_Info[3]; // BgMode, Width, Height
extern const unsigned short Background_Map[768] __attribute__ ((aligned (4))) ;  // Pal : Background_Pal
extern const unsigned char Background_Tiles[832] __attribute__ ((aligned (4))) ;  // Pal : Background_Pal
extern PAGfx_struct Background; // background pointer


// Sprite files : 
extern const unsigned short Signal_Line_Texture[64] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Bguitar_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Drum_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Eguitar_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Flute_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Guitar_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Piano_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Play_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Plus_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Trumpet_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Icon_Violin_Texture[1024] __attribute__ ((aligned (4))) ;  // 16bit texture
extern const unsigned short Sheet_Piano_Texture[4096] __attribute__ ((aligned (4))) ;  // 16bit texture

// Palette files : 
extern const unsigned short Background_Pal[8] __attribute__ ((aligned (4))) ;


#endif

