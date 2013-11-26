/***************************************************************************
    Export Level Data Base Class

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef EXPORTBASE_HPP
#define EXPORTBASE_HPP

#include <QList>

class QString;
class Levels;
struct HeightSegment;
struct SpriteSectionEntry;

class ExportBase
{
public:
    virtual void write(QString& filename,
                       Levels *levels,
                       QList<HeightSegment> heightMaps,
                       QList<SpriteSectionEntry> spriteMaps) = 0;
};

#endif // EXPORTBASE_HPP
