#ifndef SPRITEFORMAT_HPP
#define SPRITEFORMAT_HPP

#include "../stdint.hpp"

#include <QString>

struct SpriteFormat
{
    QString name;           // Identifier
    bool canBeHidden;       // Can Sprite be hidden in selection list?
    int defaultPal;         // Default Palette Entry
    int defaultRoutine;     // Default Routine Number
    int defaultShadow;      // Default Shadow

    uint8_t  width;         // Width
    uint8_t  height;        // Height
    uint8_t  bank;          // Sprite Bank
    uint16_t offset;        // Offset Within Bank
};

struct SpriteEntry
{
    uint8_t props;
    int8_t x;
    int16_t y;

    uint8_t type;
    uint8_t pal;

    // Sprite is selected in editor
    bool selected;
};

struct SpriteSectionEntry
{
    QString name;

    // Sprite Frequency Value Bitmask
    uint16_t frequency;

    QList<SpriteEntry> sprites;

    // Section is selected in editor
    bool selected;

    // Density of entry
    int density;

    // Guess at what maximum density of sprite should be
    const static int DENSITY_MAX = 0x4F;
};

#endif // SPRITEFORMAT_HPP
