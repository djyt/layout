/***************************************************************************
    Video Emulation: OutRun Sprite Rendering Hardware.
    Based on MAME source code.

    Copyright Aaron Giles.
    All rights reserved.
***************************************************************************/

#pragma once

#include "../globals.hpp"

class RenderS16;
class osprite;

class HWSprites
{
public:
    HWSprites(RenderS16*);
    ~HWSprites();
    void init(const uint8_t*);
    void render(const uint8_t, osprite *sprite_entries, uint16_t sprite_count);

private:
    RenderS16* parent;

    // Clip values.
    uint16_t x1, x2;

    static const uint32_t SPRITES_LENGTH = 0x100000 >> 2;
    static const uint16_t COLOR_BASE = 0x800;

    uint32_t sprites[SPRITES_LENGTH]; // Converted sprites

    inline void draw_pixel(
        const int32_t x, 
        const uint16_t pix, 
        const uint16_t colour, 
        const uint8_t shadow, 
        uint32_t* pPixel);
};

