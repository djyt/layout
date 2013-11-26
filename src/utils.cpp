/***************************************************************************
    Layout Utility Functions.
    - Functions to convert System 16 Palette to QT Format.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "utils.hpp"

// S16 Palette Format Reference
//
// D15 : Shade hi/lo
// D14 : Blue bit 0
// D13 : Green bit 0
// D12 : Red bit 0
// D11 : Blue bit 4
// D10 : Blue bit 3
//  D9 : Blue bit 2
//  D8 : Blue bit 1
//  D7 : Green bit 4
//  D6 : Green bit 3
//  D5 : Green bit 2
//  D4 : Green bit 1
//  D3 : Red bit 4
//  D2 : Red bit 3
//  D1 : Red bit 2
//  D0 : Red bit 1

Utils::Utils()
{
}

// Convert QT RGB value to S16
uint16_t Utils::convertToS16(QColor value)
{
    int r = (value.red()   * 255) / 2048;
    int g = (value.green() * 255) / 2048;
    int b = (value.blue()  * 255) / 2048;

    uint16_t s16Color =
    ((r & 1) << 12) + ((r & 0xFE) >> 1) +
    ((g & 1) << 13) + ((g & 0xFE) << 3) +
    ((b & 1) << 14) + ((b & 0xFE) << 7);

    return s16Color;
}

// Convert Single S16 Palette Entry to QT Format
QRgb Utils::convertToQT(uint16_t value)
{
    uint32_t r = (value & 0x000f) << 1; // r rrr0
    uint32_t g = (value & 0x00f0) >> 3; // g ggg0
    uint32_t b = (value & 0x0f00) >> 7; // b bbb0
    if ((value & 0x1000) != 0)
        r |= 1; // r rrrr
    if ((value & 0x2000) != 0)
        g |= 1; // g gggg
    if ((value & 0x4000) != 0)
        b |= 1; // b bbbb

    r = r * 255 / 31;
    g = g * 255 / 31;
    b = b * 255 / 31;

    return qRgb(r, g, b);
}

// Convery S16 Sprite Palette to QT Format
QRgb* Utils::convertSpritePalette(uint8_t* srcPal)
{
    QRgb* pal = new QRgb[SPR_PAL_ENTRIES];
    // 32 Bytes Per Palette. 255 Paletes Total.
    for (int i = 0; i < SPR_PAL_ENTRIES; i++)
    {
        uint16_t a = (srcPal[(i * 2)] << 8) | srcPal[(i * 2) + 1];
        pal[i] = convertToQT(a);
    }
    return pal;
}
