/***************************************************************************
    Simple OutRun Sprite Rendering Class.

    - Note the data format does not contain an end marker, so this info
      needs to be passed in.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <QRgb>
#include <QImage>

#include "../stdint.hpp"

class Sprite
{
public:
    QImage* image;

    Sprite();
    ~Sprite();
    static void convertSpriteRom(uint8_t*, int length, QRgb *p);

    void setSprite(const int bank, const int offset, const int width, const int height, const int pal);
    void setSprite(const int bank, const int offset, const int width, const int height, const bool flip, const int pal = -1);
    void draw(QImage* image);

private:
    static QRgb* pal;
    static uint32_t* data;
    int spriteOffset;
    int spriteWidth;
    int spriteHeight;
    int spritePalette;
    bool spriteHFlip;

    void setPixel(QImage* image, int x, int y, int index);

};

#endif // SPRITE_HPP
