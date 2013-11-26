/***************************************************************************
    Video Emulation: OutRun Sprite Rendering Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

#include <stdlib.h>       // abs
#include "../globals.hpp"
#include "renders16.hpp"
#include "osprite.hpp"
#include "hwsprites.hpp"

/*******************************************************************************************
*  Out Run/X-Board-style sprites
*
*      Offs  Bits               Usage
*       +0   e------- --------  Signify end of sprite list
*       +0   -h-h---- --------  Hide this sprite if either bit is set
*       +0   ----bbb- --------  Sprite bank
*       +0   -------t tttttttt  Top scanline of sprite + 256
*       +2   oooooooo oooooooo  Offset within selected sprite bank
*       +4   ppppppp- --------  Signed 7-bit pitch value between scanlines
*       +4   -------x xxxxxxxx  X position of sprite (position $BE is screen position 0)
*       +6   -s------ --------  Enable shadows
*       +6   --pp---- --------  Sprite priority, relative to tilemaps
*       +6   ------vv vvvvvvvv  Vertical zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
*       +8   y------- --------  Render from top-to-bottom (1) or bottom-to-top (0) on screen
*       +8   -f------ --------  Horizontal flip: read the data backwards if set
*       +8   --x----- --------  Render from left-to-right (1) or right-to-left (0) on screen
*       +8   ------hh hhhhhhhh  Horizontal zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
*       +E   dddddddd dddddddd  Scratch space for current address
*
*  Out Run only:
*       +A   hhhhhhhh --------  Height in scanlines - 1
*       +A   -------- -ccccccc  Sprite color palette
*
*  X-Board only:
*       +A   ----hhhh hhhhhhhh  Height in scanlines - 1
*       +C   -------- cccccccc  Sprite color palette
*
*  Final bitmap format:
*
*            -s------ --------  Shadow control
*            --pp---- --------  Sprite priority
*            ----cccc cccc----  Sprite color palette
*            -------- ----llll  4-bit pixel data
*
 *******************************************************************************************/

HWSprites::HWSprites(RenderS16* parent)
{
    this->parent = parent;
    x1 = 0;
    x2 = S16_WIDTH;
}

HWSprites::~HWSprites()
{
}

void HWSprites::init(const uint8_t* src_sprites)
{
    if (src_sprites)
    {
        // Convert S16 tiles to a more useable format
        const uint8_t *spr = src_sprites;

        for (uint32_t i = 0; i < SPRITES_LENGTH; i++)
        {
            uint8_t d3 = *spr++;
            uint8_t d2 = *spr++;
            uint8_t d1 = *spr++;
            uint8_t d0 = *spr++;

            sprites[i] = (d0 << 24) | (d1 << 16) | (d2 << 8) | d3;
        }
    }
}

void HWSprites::render(const uint8_t priority, osprite* sprite_entries, uint16_t sprite_count)
{
    const uint32_t numbanks = SPRITES_LENGTH / 0x10000;

    for (uint16_t i = 0; i < sprite_count; i++)
    {
        osprite* spr = &sprite_entries[i];
        uint16_t* data = sprite_entries[i].data;

        uint32_t sprpri  = 1 << ((data[3] >> 12) & 3);
        if (sprpri != priority) continue;

        // if hidden, or top greater than/equal to bottom, or invalid bank, punt
        if (spr->is_hidden()) continue;
        
        int16_t bank    = (data[0] >> 9) & 7;
        int32_t top     = spr->get_screen_y1();
        uint32_t addr   = data[1];
        int32_t pitch   = ((data[2] >> 1) | ((data[4] & 0x1000) << 3)) >> 8;
        int32_t xpos    = spr->get_screen_x1();
        uint8_t shadow  = (data[3] >> 14) & 1;
        int32_t vzoom   = data[3] & 0x7ff;
        int32_t ydelta  = spr->get_y_delta();
        int32_t flip    = (~data[4] >> 14) & 1;
        int32_t xdelta  = spr->get_x_delta();
        int32_t hzoom   = data[4] & 0x7ff;
        int32_t color   = (data[5] & 0x7f) << 4;
        int32_t x, y, ytarget, yacc = 0, pix;

        // initialize the end address to the start address
        data[7] = addr;

        // clamp to within the memory region size
        if (numbanks != 0)
            bank %= numbanks;

        const uint32_t spritedata = 0x10000 * bank;

        // clamp to a maximum of 8x (not 100% confirmed)
        if (vzoom < 0x40) vzoom = 0x40;
        if (hzoom < 0x40) hzoom = 0x40;

        // loop from top to bottom
        ytarget = spr->get_screen_y2();

        // Adjust for widescreen mode
        xpos += S16_X_OFF;

        spr->width = 0;

        for (y = top; y != ytarget; y += ydelta)
        {
            // skip drawing if not within the cliprect
            if (y >= 0 && y < S16_HEIGHT)
            {
                uint32_t* pPixel = &parent->pixels[y * S16_WIDTH];
                int32_t xacc = 0;

                // non-flipped case
                if (flip == 0)
                {
                    // start at the word before because we preincrement below
                    data[7] = (addr - 1);

                    int line_width = 0;

                    for (x = xpos; (xdelta > 0 && x < S16_WIDTH) || (xdelta < 0 && x >= 0); )
                    {
                        uint32_t pixels = sprites[spritedata + ++data[7]]; // Add to base sprite data the vzoom value

                        // draw four pixels
                        pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;

                        line_width = std::abs(xpos - x);

                        if (line_width > spr->width)
                            spr->width = line_width;

                        // stop if the second-to-last pixel in the group was 0xf
                        if ((pixels & 0x000000f0) == 0x000000f0)
                            break;
                    }
                }
                // flipped case
                else
                {
                    // start at the word after because we predecrement below
                    data[7] = (addr + 1);

                    int line_width = 0;

                    for (x = xpos; (xdelta > 0 && x < S16_WIDTH) || (xdelta < 0 && x >= 0); )
                    {
                        uint32_t pixels = sprites[spritedata + --data[7]];

                        // draw four pixels
                        pix = (pixels >>  0) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  4) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >>  8) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 12) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 16) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 20) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 24) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;
                        pix = (pixels >> 28) & 0xf; while (xacc < 0x200) { draw_pixel(x, pix, color, shadow, pPixel+x); x += xdelta; xacc += hzoom; } xacc -= 0x200;

                        line_width = std::abs(xpos - x);

                        if (line_width > spr->width)
                            spr->width = line_width;

                        // stop if the second-to-last pixel in the group was 0xf
                        if ((pixels & 0x0f000000) == 0x0f000000)
                            break;
                    }
                }
            }
            // accumulate zoom factors; if we carry into the high bit, skip an extra row
            yacc += vzoom; 
            addr += pitch * (yacc >> 9);
            yacc &= 0x1ff;
        }
    }
}

void HWSprites::draw_pixel(const int32_t x, const uint16_t pix, const uint16_t colour, const uint8_t shadow, uint32_t* pPixel)
{
    if (x >= x1 && x < x2 && pix != 0 && pix != 15)
    {
        if (shadow && pix == 0xa) 
        {
            *pPixel &= 0xfff;
            *pPixel += 0x2000;
        } 
        else 
        {
            *pPixel = (pix | colour | COLOR_BASE) & 0xfff;
        }
    }  
}
