/***************************************************************************
    Export Level Data to CannonBall.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef EXPORTCANNONBALL_HPP
#define EXPORTCANNONBALL_HPP

#include <QList>
#include "exportbase.hpp"

class LevelData;
struct ControlPoint;

class ExportCannonball : public ExportBase
{
public:
    ExportCannonball();

    void write(QString& filename,
               Levels *levels,
               QList<HeightSegment> heightMaps,
               QList<SpriteSectionEntry> spriteMaps);

private:
    // Exporter Version Number. Bump this when the header changes to avoid incompatibilities
    const static int EXPORT_VERSION = 1;

    // Position in stream
    int pos;

    void writeCPU1Path(QDataStream& out, LevelData* level, const int length);
    int getLevelLength(LevelData* level);
    int getEndSectionLength(LevelData* level);
    int getSplitLength(LevelData* level);
    void writeCurveData(QDataStream& out, LevelData* level, bool invertCurve = false);
    void writeWidthHeightData(QDataStream& out, LevelData* level);
    void insertAtPos(QList<ControlPoint>* list, ControlPoint cp);
    void writeSpriteData(QDataStream& out, LevelData* level);
    void writeHeightMaps(QDataStream& out, QList<HeightSegment> heightMaps);
    void writeSpriteMaps(QDataStream& out, QList<SpriteSectionEntry> spriteMaps);
    int getSpriteMapLength(QList<SpriteSectionEntry> spriteMaps);
};

#endif // EXPORTCANNONBALL_HPP
