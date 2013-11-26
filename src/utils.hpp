/***************************************************************************
    Layout Utility Functions.
    - Functions to convert System 16 Palette to QT Format.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef UTILS_HPP
#define UTILS_HPP

#include <QRgb>
#include <QColor>

#include "stdint.hpp"

class Utils
{
public:
    // 256 Entries. 32 Bytes each
    static const int SPR_PAL_ENTRIES = 256 * 16;

    Utils();

    static uint16_t convertToS16(QColor value);
    static QRgb convertToQT(uint16_t value);
    static QRgb* convertSpritePalette(uint8_t* srcPal);
};

#endif // UTILS_HPP
