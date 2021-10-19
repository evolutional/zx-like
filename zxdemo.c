/*
------------------------------------------------------------------------------
		  Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

Note that different files in this project has different licenses. Each source
file contains licensing information for the code in that file.

*/

#include "dos.h"
#include "zx.h"

#include <stdio.h>

void loading( const char* filename );

// Simple demo
// 1) Fakes a loading screen
// 2) Use the keys 1-6 to cycle through various screen demos

int zxmain( int argc, char *argv[] ) {
    
    const char* files[] = {
        "files/cyberpunk.scr",
        "files/morphine-pulpfiction_256x192.scr",
        "files/morphine-pulpfiction_256x192_ulaplus.scr",
        "files/morphine-pulpfiction_256x192_hicolor.scr",
        "files/morphine-pulpfiction_256x192_hicolor_ulaplus.scr",
        "files/django.scr",
    };

    setdoublebuffer(1);
    loading("files/django.scr");

    zxsetborder(BLACK);

    while(!shuttingdown())
    {
        waitvbl();

        if (keystate(KEY_1)) {
            zxloadscr(files[0]);
        } else if (keystate(KEY_2)) {
            zxloadscr(files[1]);
        } else if (keystate(KEY_3)) {
             zxloadscr(files[2]);
        } else if (keystate(KEY_4)) {
            zxloadscr(files[3]);
        } else if (keystate(KEY_5)) {
            zxloadscr(files[4]);
        } else if (keystate(KEY_6)) {
            zxloadscr(files[5]);
        }

        zxvsync();
        swapbuffers();
    }

    return 0;
}

void loading( const char* filename ) {

    unsigned char buf[6912];
    FILE* f = fopen(filename, "rb");
    if (f == 0) {
        return;
    }
    fread(buf, 1, sizeof(buf), f);
    fclose(f);

    zxsetborder(WHITE);
    zxcls( ZX_ATTR(BLACK, WHITE) );

    int progress = 0;
    int t = 0;
    int w, h;

    int sync = 0;
    zxgetscreendim( &w, &h );

    struct sync_entry {
        int len0;
        enum zxula_color_t color0;
        int len1;
        enum zxula_color_t color1;
        int duration;
        int loading;
    };

    const struct sync_entry sync_pattern[] = {
        { 2467, RED, 2868, CYAN, 2000, 0 },
        { 1667, YELLOW, 1768, BLUE, 350, 0 },
        { 2467, WHITE, 2868, WHITE, 1000, 0 },
        { 2467, RED, 2868, CYAN, 1700, 0 },
        { 2667, YELLOW, 2968, BLUE, 500, 1 },
        { 2467, YELLOW, 2768, BLUE, 1500, 1 },
        { 2567, YELLOW, 2368, BLUE, 800, 1 },
        { 2467, YELLOW, 2768, BLUE, 2300, 1 },
        { 0, 0, 0, 0, 0 }
    };

    const struct sync_entry* entry = sync_pattern;
    int counter = 0;

    // Wait a bit
    for(int n = 0; n < 175; ++n) {
        waitvbl();
        zxvsync();
        swapbuffers();
        // Space skips the wait
        if (keystate(KEY_SPACE)) {
            break;
        }
    }

    unsigned char* dst = zxscr();
    unsigned char* src = buf;

    while(!shuttingdown())
    {
        waitvbl();

        for(int y = 0; y < h; ++y) {
            for(int x = 0; x < w; ++x) {

                if (sync) {
                    if (++t > entry->len0) {
                        sync = 1 - sync;                    
                        t = 0;
                    }
                    zxsetborder(entry->color0);
                } else {
                    if (++t > entry->len1) {
                        sync = 1 - sync;                    
                        t = 0;

                        if (++counter > entry->duration) {                       
                            ++entry;
                            counter = 0;
                            if (entry->duration == 0) {
                                break;
                            }                            
                        }   
                    }
                    zxsetborder(entry->color1);
                }

                if (entry->loading) {
                    if (++progress >= 2500) {
                        memcpy(dst, src, 1);
                        dst++;
                        src++;
                        progress = 0;
                        if (src >= buf + sizeof(buf)) {
                            return;
                        }
                    }
                }

                zxrenderpixel(screenbuffer(), y, x);
            }
        }

        swapbuffers();
    }
}


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
