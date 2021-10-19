/*
------------------------------------------------------------------------------
		  Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

Note that different files in this project has different licenses. Each source
file contains licensing information for the code in that file.

*/

#ifndef zxlike_h
#define zxlike_h

enum zxscreenmode_t {
     // ZX Spectrum ULA screen format 256x192 with 32x24 attributes
    zxscreenmode_spectrum,
    // ZX Spectrum style screen, but with a linear buffer
    zxscreenmode_spectrum_linear,
    // ZX-Uno 128x96 "Radastan" mode. 4bpp color.
    zxscreenmode_lores16,
    // Timex hi-color mode. Color applies per pixel row.
    zxscreenmode_timex_hicolor,
    // Timex hi-res mode. Two color 512x192 screen
    zxscreenmode_timex_hires,
};

enum zxula_color_t {
    BLACK,
    BLUE,
    RED,
    MAGENTA,
    GREEN,
    CYAN,
    YELLOW,
    WHITE,

    BRIGHT_BLACK,
    BRIGHT_BLUE,
    BRIGHT_RED,
    BRIGHT_MAGENTA,
    BRIGHT_GREEN,
    BRIGHT_CYAN,
    BRIGHT_YELLOW,
    BRIGHT_WHITE,
};

#define FLASH   (1 << 7)
#define BRIGHT  (1 << 6)

#define ZX_ATTR(pen, paper) (paper << 3 | pen)

void zxinit( void );
void zxdestroy( void );

void zxcls( int color );
void zxsetattr( int row, int col, unsigned char attr );
unsigned char zxgetattr( int row, int col );

unsigned char* zxscrattrs( void );
unsigned char* zxscr( void );

void zxsetborder( enum zxula_color_t color );
enum zxula_color_t zxgetborder( void );

void zxresetpal( void );
void zxsetulaplus( int enabled );
void zxsetulapluspal( int index, int color );
void zxsetulapluspal_ex( unsigned char* colors );
int zxgetulapluspal( int index );

int zxloadscr( const char* filename );

void zxsetscreenmode( enum zxscreenmode_t mode );
void zxrenderscreen( unsigned char* screen );
void zxrenderpixel( unsigned char* screen, int line, int x );
void zxrenderscanline( unsigned char* screen, int line );
void zxvsync( void );

void zxgetscreendim( int* w, int* h );

#endif // zxlike_h


#ifdef ZXLIKE_IMPLEMENTATION

#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dos.h"

int zxmain( int argc, char *argv[] );

struct zx_t {
    enum zxscreenmode_t mode;
    int flash_counter;
    
    struct {
        enum zxula_color_t color;
        int width;
        int height;
    } border;   

    struct {
        bool enabled;
        uint8_t colors[64];
    } ulaplus;

    unsigned char vram[0x4000]; // 16k RAM for everything
}* ula;

// ZX Spectrum ULA Palette in XRGB format
static const uint32_t zx_spectrum_palette[16] = {
    0x00000000, // BLACK
    0x000022C7, // BLUE
    0x00D62816, // RED
    0x00D433C7, // MAGENTA
    0x0000C525, // GREEN
    0x0000C7C9, // CYAN
    0x00CCC82A, // YELLOW
    0x00CACACA, // WHITE
    0x00000000, // BRIGHT_BLACK
    0X00002BFB, // BRIGHT_BLUE
    0X00FF331C, // BRIGHT_RED
    0X00FF40FC, // BRIGHT_MAGENA
    0X0000F92F, // BRIGHT_GREEN
    0X0000FBFE, // BRIGHT_CYAN
    0X00FFFC36, // BRIGHT_YELLOW
    0X00FFFFFF, // BRIGHT_WHITE
};

unsigned char* zxgetattrptr( int row, int col ) {
    unsigned char* attrs = zxscrattrs();
    if (ula->mode == zxscreenmode_timex_hicolor) {
        uint16_t addr = ((row & 0xC0) << 5) + ((row & 0x07) << 8) + ((row & 0x38) << 2) + col;
        return attrs + addr;
    }
    return &attrs[(row*32)+col];
}

void zxcls( int color ) {
    memset(zxscr(), 0, 32 * 192);
    switch(ula->mode) {
        case zxscreenmode_timex_hicolor:
            memset(&ula->vram[0x2000], color & 0xF, 0x2000);
            break;
        case zxscreenmode_timex_hires:
            memset(ula->vram, color & 0x1, 0x4000);
            break;
        case zxscreenmode_lores16:
            memset(&ula->vram, (color&0xF) | ((color&0xF)<<4), 0x2000);
            break;
        default:
            memset(zxscrattrs(), color, 768);
            break;
    }
}

void zxsetattr( int row, int col, unsigned char v ) {
    unsigned char* attr = zxgetattrptr(row, col);
    *attr = v;
}

unsigned char zxgetattr( int row, int col ) {
    unsigned char* attr = zxgetattrptr(row, col);
    return *attr;
}

unsigned char* zxscrattrs( void ) {
    if (ula->mode == zxscreenmode_timex_hicolor) {
        return &ula->vram[0x2000];
    }
    return &ula->vram[32 * 192];
}

unsigned char* zxscr( void ) {
    return ula->vram;
}

void zxsetborder( enum zxula_color_t color ) {
    ula->border.color = color;
}
enum zxula_color_t zxgetborder( void ) {
    return ula->border.color;
}

void zxsetulaplus( int enabled ) {
    ula->ulaplus.enabled = enabled;
}

void zxsetulapluspal( int index, int color ) {
    // G3R3B2
    uint8_t r = (color & 0b00011100) >> 2;
    uint8_t g = (color & 0b11100000) >> 5;
    uint8_t b = (color & 0b00000011) << 1;

    b |= (b >> 1) | (b >> 2);

    r = (r << 5) | (r << 2) | (r >> 1);
    g = (g << 5) | (g << 2) | (g >> 1);
    b = (b << 5) | (b << 2) | (b >> 1);

    ula->ulaplus.colors[index & 0x3F] = color;
    setpal(index & 0x3F, r >> 2, g >> 2, b >> 2);
}

void zxsetulapluspal_ex( unsigned char* colors ) {
    for(int i = 0; i < 64; ++i) {
        zxsetulapluspal(i, *colors++);
    }
}

int zxgetulapluspal( int index ) {
    return ula->ulaplus.colors[index & 0x3F];
}

void zxinit( void ) {
    ula = (struct zx_t*)malloc(sizeof(struct zx_t));
    zxsetscreenmode( zxscreenmode_spectrum );
    ula->border.color = WHITE;
}

void zxdestroy( void ) {
    free(ula);
}

void zxresetpal( void ) {
    for(int i = 0; i < 16; ++i) {
        uint8_t r = zx_spectrum_palette[i] >> 18;
        uint8_t g = zx_spectrum_palette[i] >> 10;
        uint8_t b = zx_spectrum_palette[i] >> 2;

        setpal(i, r, g, b);
        setpal(i+16, r, g, b);
        setpal(i+32, r, g, b);
        setpal(i+48, r, g, b);
    }
}

int zxloadscr( const char* filename ) {
    
    const int ula_pixel_size = 6144;
    const int ula_attr_size = 768;
    const int ula_scr_size = ula_pixel_size + ula_attr_size;
    const int ulaplus_pal_size = 64;

    uint8_t buf[12352]; // max size of timex hicolor+ulaplus
    FILE* f = fopen(filename, "rb");
    if (f == 0) {
        return -1;
    }
    int n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n <= 0) {
        return -2;
    }

    switch(n) {
        case 6912:  // scr
            zxsetscreenmode(zxscreenmode_spectrum);
            memcpy(zxscr(), buf, ula_scr_size);
            return 0;
        case 6912 + 64:  // scr + ulaplus
            zxsetscreenmode(zxscreenmode_spectrum);
            memcpy(zxscr(), buf, ula_scr_size);
            zxsetulapluspal_ex(buf + ula_scr_size);
            zxsetulaplus(1);
            return 0;
        case 12288:  // timex hicolor
            zxsetscreenmode(zxscreenmode_timex_hicolor);
            memcpy(zxscr(), buf, ula_pixel_size);
            memcpy(zxscr() + 0x2000, buf + ula_pixel_size, ula_pixel_size);
            return 0;
        case 12288 + 64: // timex hicolor + ula plus
            zxsetscreenmode(zxscreenmode_timex_hicolor);
            memcpy(zxscr(), buf, ula_pixel_size);
            memcpy(zxscr() + 0x2000, buf + ula_pixel_size, ula_pixel_size);
            zxsetulapluspal_ex(buf + (ula_pixel_size * 2));
            zxsetulaplus(1);
            return 0;
    }
    return -3;
}

void zxsetscreenmode( enum zxscreenmode_t mode ) {
    switch(mode) {
        case zxscreenmode_spectrum:
        case zxscreenmode_spectrum_linear:
        case zxscreenmode_lores16:
        case zxscreenmode_timex_hicolor:
            setvideomode( videomode_320x240 );
            ula->flash_counter = 0;
            ula->border.width = 32;
            ula->border.height = 24;
            ula->ulaplus.enabled = false;
            zxresetpal();
            break;
        case zxscreenmode_timex_hires:
            setvideomode( videomode_640x200 );
            ula->flash_counter = 0;
            ula->border.width = 64;
            ula->border.height = 8;
            ula->ulaplus.enabled = false;
            zxresetpal();
            break;
    }

    ula->mode = mode;
}

void zxrenderscreen( unsigned char* screen ) {
    const int screen_h = 192 + (2*ula->border.height);
    for(int line = 0; line < screen_h; ++line) {
        zxrenderscanline( screen, line );
    }
}

void zxrenderpixel( unsigned char* screen, int line, int x ) {
    const int screen_w = 256 + (2*ula->border.width);
    const int screen_h = 192 + (2*ula->border.height);

    if (line >= screen_h || x >= screen_w) {
        return;
    }

    int y = line;

    int idx = (y * screen_w) + x;
    if (y < ula->border.height || y >= 192 + ula->border.height) {
        screen[idx] = ula->border.color;
    }
    else {
        if (x < ula->border.width || x >= 256 + ula->border.width) {
            screen[idx] = ula->border.color;
            } else {
            int col = (x - ula->border.width)/8;
            int row = (y - ula->border.height)/8;
            int disp_y = y - ula->border.height;
            int disp_x = x - ula->border.width;

            // main screen
            switch(ula->mode) {
                case zxscreenmode_spectrum:
                case zxscreenmode_spectrum_linear:
                case zxscreenmode_timex_hicolor:
                    {
                        uint8_t data_byte;
                        uint8_t attr_byte;
                        
                        if (ula->mode == zxscreenmode_timex_hicolor) {
                            // Attrs start at $6000 in a real machine and is interleaved in the same pixel format
                            const uint8_t* attrs = ula->vram + 0x2000;
                            uint16_t addr = ((disp_y & 0xC0) << 5) + ((disp_y & 0x07) << 8) + ((disp_y & 0x38) << 2) + col;
                            attr_byte = attrs[addr];
                        } else {
                            const uint8_t* attrs = zxscrattrs();
                            attr_byte = attrs[(row * 32)+col];
                        }
                        
                        if (ula->mode != zxscreenmode_spectrum_linear) {
                            uint16_t addr = ((disp_y & 0xC0) << 5) + ((disp_y & 0x07) << 8) + ((disp_y & 0x38) << 2) + col;
                            data_byte = ula->vram[addr];
                        } else {
                            uint16_t addr = (disp_y * 256) + col;
                            data_byte = ula->vram[addr];
                        }

                        uint8_t data_bit = x % 8;
                        uint8_t p = (data_byte >> (7 - data_bit)) & 0x01;
                        
                        uint8_t paper_color = (attr_byte & 0b00111000) >> 3;
                        uint8_t ink_color = (attr_byte & 0b00000111);
                        uint8_t flash = attr_byte & 0b10000000;
                        uint8_t bright = attr_byte & 0b01000000;

                        if (ula->ulaplus.enabled) {
                            uint8_t palette_id = (flash | bright) >> 2;
                            screen[idx] = p 
                                ? palette_id | ink_color 
                                : palette_id | paper_color + 8;
                        } else {
                            if (bright)
                            {
                                // bright?
                                ink_color += 8;
                                paper_color += 8;
                            }
                            
                            if (flash && ula->flash_counter >= 0xF) {
                                uint8_t tmp = ink_color;
                                ink_color = paper_color;
                                paper_color = tmp;
                            }

                            screen[idx] = p ? ink_color : paper_color;
                        }
                    }
                    break;
                case zxscreenmode_lores16:
                    {
                        if (disp_y % 2 == 0 && disp_x % 2 == 0) {
                            // 128*96 16 bit colour mode, each pixel is AAAA'BBBB * 8
                            // we need to plot double height / double width pixels
                            int lo_x = disp_x / 2;
                            int lo_y = disp_y / 2;
                            uint16_t addr = (lo_y * 64) + (lo_x/2);
                            uint8_t data_byte = ula->vram[addr];
                            
                            uint8_t p = lo_x % 2 == 0 ? ((data_byte & 0xF0) >> 4) : (data_byte & 0x0F);

                            screen[idx] = p & 0xF;
                            screen[idx+1] = p & 0xF;
                            screen[idx + screen_w] = p & 0xF;
                            screen[idx + screen_w + 1] = p & 0xF;
                        }
                    }
                    break;
                case zxscreenmode_timex_hires:
                    {
                        uint16_t addr = ((disp_y & 0xC0) << 5) + ((disp_y & 0x07) << 8) + ((disp_y & 0x38) << 2) + col;
                        if (disp_x % 2 == 1) {
                            addr += 0x2000;
                        }
                        uint8_t data_byte = ula->vram[addr];
                        uint8_t data_bit = x % 8;
                        uint8_t p = (data_byte >> (7 - data_bit)) & 0x01;
                        screen[idx] = p;
                    }
                    break;
            }
        }
    }
}

void zxrenderscanline( unsigned char* screen, int line ) {
    const int screen_w = 256 + (2*ula->border.width);
    for (int x = 0; x < screen_w; x++) {
        zxrenderpixel(screen, line, x);
    }
}

void zxvsync( void ) {
    zxrenderscreen(screenbuffer());
    ula->flash_counter = ++ula->flash_counter & 0x1F;
}

void zxgetscreendim( int* w, int* h ) {
    if (w) {
        *w = 256 + (2*ula->border.width);
    }

    if (h) {
        *h = 192 + (2*ula->border.height);;
    }
}

int main( int argc, char* argv[] ) {
    zxinit();
    int result = zxmain(argc, argv);
    zxdestroy();
    return result;
}

#endif // ZXLIKE_IMPLEMENTATION

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2021 Oli Wilkinson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
