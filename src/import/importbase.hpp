/***************************************************************************
    Import Level Data Base Class

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef IMPORTBASE_HPP
#define IMPORTBASE_HPP

#include <QList>
#include "../globals.hpp"

struct SpriteFormat;
struct SpriteSectionEntry;
struct HeightSegment;

class ImportBase
{
public:
    virtual bool loadLevel(int id, const bool loadPatterns = true) = 0;
    virtual QList<HeightSegment> loadHeightSections() = 0;
    virtual QList<SpriteFormat> loadSpriteList() = 0;
    virtual uint8_t* getPaletteData() = 0;
    virtual QList<SpriteSectionEntry> loadSpriteSections(int id = 0) = 0;
};

#endif // IMPORTBASE_HPP
