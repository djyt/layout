/***************************************************************************
    Simple OutRun Sprite Rendering Class.

    - Note the data format does not contain an end marker, so this info
      needs to be passed in.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../utils.hpp"
#include "sprite.hpp"

// Static Members
QRgb*     Sprite::pal;
uint32_t* Sprite::data;

Sprite::Sprite()
{
    spritePalette = -1;
    image = NULL;
}

Sprite::~Sprite()
{
    if (image != NULL)
        delete image;
}

// Setup Sprite Data
void Sprite::convertSpriteRom(uint8_t* rom, int length, QRgb* p)
{
    pal = p;

    const uint8_t* spr = rom;
    int romLength = length >> 2;
    data = new uint32_t[romLength];

    for (int i = 0; i < romLength; i++)
    {
        uint8_t d3 = *spr++;
        uint8_t d2 = *spr++;
        uint8_t d1 = *spr++;
        uint8_t d0 = *spr++;

        data[i] = (d0 << 24) | (d1 << 16) | (d2 << 8) | d3;
    }
}


// Sprite Height is 0x7F + Height Helper
void Sprite::setSprite(  const int bank,
                         const int offset,
                         const int width,
                         const int height,
                         const bool flip,
                         const int pal)
{
    spriteOffset  = (bank * 0x10000) + offset;
    spriteWidth   = width;
    spriteHeight  = height + 1;
    spritePalette = pal;
    spriteHFlip   = flip;

    if (image != NULL)
        delete image;

    image = new QImage(spriteWidth, spriteHeight, QImage::Format_ARGB32);
    image->fill(Qt::transparent);
    draw(image);
}

void Sprite::setSprite(  const int bank,
                         const int offset,
                         const int width,
                         const int height,
                         const int pal)
{
    spriteOffset  = (bank * 0x10000) + offset;
    spriteWidth   = width;
    spriteHeight  = height + 1;
    spritePalette = pal;

    if (image != NULL)
        delete image;

    image = new QImage(spriteWidth, spriteHeight, QImage::Format_ARGB32);
    image->fill(Qt::transparent);
    draw(image);
}

void Sprite::draw(QImage* image)
{
    int32_t pix;

    // Screen X-Coordinates
    int x = 0;
    int counter = 0;

    for (int y = 0; y < spriteHeight;)
    {
        const uint32_t pixels = data[spriteOffset + counter++];

        if (x == 0 && pixels == 0x000000F0)
            continue;

        // draw eight pixels
        if (!spriteHFlip)
        {
            pix = (pixels >> 28) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >> 24) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >> 20) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >> 16) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >> 12) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >>  8) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >>  4) & 0xf; setPixel(image, x, y, pix); x++;
            pix = (pixels >>  0) & 0xf; setPixel(image, x, y, pix); x++;
        }

        // Flipped
        else
        {
            pix = (pixels >> 28) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >> 24) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >> 20) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >> 16) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >> 12) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >>  8) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >>  4) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
            pix = (pixels >>  0) & 0xf; setPixel(image, spriteWidth - x, y, pix); x++;
        }

        // stop if the second-to-last pixel in the group was 0xf
        if ((pixels & 0x000000f0) == 0x000000f0)
        {
            x = 0;
            y++;
        }
    }
}

void Sprite::setPixel(QImage* image, int x, int y, int index)
{
    if (x >= 0 && x <= image->width() && x < spriteWidth && index != 0 && index != 15)
    {
        // Greyscale
        if (spritePalette == -1)
        {
            index = 0xFF - (index << 4);
            image->setPixel(x, y, qRgb(index, index, index));
        }
        // Palette
        else
        {
            image->setPixel(x, y, pal[index + (spritePalette * 16)]);
        }
    }
}
