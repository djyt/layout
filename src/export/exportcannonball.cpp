/***************************************************************************
    Export Level Data to CannonBall.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <QFile>
#include <QDataStream>
#include <QMessageBox>
#include <QtCore/qmath.h> // Sqrt

#include "exportcannonball.hpp"
#include "../leveldata.hpp"
#include "../stdint.hpp"
#include "../levels/levels.hpp"
#include "../sprites/spriteformat.hpp"

ExportCannonball::ExportCannonball()
{
}

// MACROS: Helper Macros to output specfic data size
#define out8(x)   out << (int8_t)   x; pos += sizeof(int8_t)
#define outu8(x)  out << (uint8_t)  x; pos += sizeof(uint8_t)
#define out16(x)  out << (int16_t)  x; pos += sizeof(int16_t)
#define outu16(x) out << (uint16_t) x; pos += sizeof(uint16_t)
#define outu32(x) out << (uint32_t) x; pos += sizeof(uint32_t)

void ExportCannonball::write(QString& filename,
                             Levels* levels,
                             QList<HeightSegment> heightMaps,
                             QList<SpriteSectionEntry> spriteMaps)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(0, "Read only", "The file is in read only mode");
        return;
    }

    pos = 0;
    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian);

    LevelData* splitLevel = levels->getSplit();

    // --------------------------------------------------------------------------------------------
    // Write Version Header & Settings
    // --------------------------------------------------------------------------------------------

    const static bool VERBOSE = false;

    outu32(EXPORT_VERSION); // Header Version
    outu8((levels->displayStartLine() ? 1 : 0));

    // --------------------------------------------------------------------------------------------
    // Write Master Header
    // --------------------------------------------------------------------------------------------

    const static uint32_t HEADER = pos + ((Levels::MAP_SLOTS + 8) * sizeof(uint32_t));

    // [1] CPU 1 Path Data Offset (Only need the value for the start)
    int offset = HEADER;
    if (VERBOSE) std::cout << std::hex << "Header 1: " << offset << std::endl;
    outu32(offset);
    offset += (LevelData::LEVEL_LENGTH_CPU1 * sizeof(uint32_t)) * LEVELS;

    // CPU 0 Start of Levels 1-15
    for (int i = 0; i < LEVELS; i++)
    {
        if (VERBOSE) std::cout << std::hex << "Level Header: " << i << "," << offset << std::endl;
        outu32(offset);
        offset += getLevelLength(levels->getMappedLevelP(i));
    }

    // End Sections CPU 1
    if (VERBOSE) std::cout << std::hex << "End Section CPU 1: " << offset << std::endl;
    outu32(offset);
    offset += (LevelData::END_LENGTH_CPU1 * sizeof(uint32_t)); // End sections share the same road path
    // End Sections CPU 0
    for (int i = LEVELS; i < Levels::MAP_SLOTS; i++)
    {
        if (VERBOSE) std::cout << std::hex << "End Section Header: " << i << "," << offset << std::endl;
        outu32(offset);
        offset += getEndSectionLength(levels->getMappedLevelP(i));
    }

    // Split CPU 1
    if (VERBOSE) std::cout << std::hex << "Header Split CPU 1: " << offset << std::endl;
    outu32(offset);
    offset += (LevelData::SPLIT_LENGTH_CPU1 * sizeof(uint32_t));
    // Split CPU 0
    if (VERBOSE) std::cout << std::hex << "Header Split CPU 0: " << offset << std::endl;
    outu32(offset);
    offset += getSplitLength(splitLevel);

    // [2] Start of Sky Palettes
    if (VERBOSE) std::cout << std::hex << "Header Sky: " << offset << std::endl;
    outu32(offset);
    offset += LevelPalette::SKY_PALS * sizeof(uint32_t); // pointers
    offset += (LevelPalette::SKY_PALS * LevelPalette::SKY_LENGTH) * sizeof(uint32_t);

    // [3] Start of Ground Palettes
    if (VERBOSE) std::cout << std::hex << "Header Gnd: " << offset << std::endl;
    outu32(offset);
    offset += LevelPalette::GND_PALS * sizeof(uint32_t); // pointers
    offset += (LevelPalette::GND_PALS * LevelPalette::GND_LENGTH) * sizeof(uint32_t);

    // [4] Start of Sprite Map Entries
    if (VERBOSE) std::cout << std::hex << "Header Smap: " << offset << std::endl;
    outu32(offset);
    offset += getSpriteMapLength(spriteMaps);

    // [5] Start of Height Map Entries
    if (VERBOSE) std::cout << std::hex << "Header Hmap: " << offset << std::endl;
    outu32(offset);

    // --------------------------------------------------------------------------------------------
    // CPU 1 Road Data: Write Path Info For All Stage
    // --------------------------------------------------------------------------------------------

    if (VERBOSE) std::cout << std::hex << "Path Data Start: " << pos << std::endl;
    for (int i = 0; i < LEVELS; i++)
    {
        writeCPU1Path(out, levels->getMappedLevelP(i), LevelData::LEVEL_LENGTH_CPU1);
    }

    // --------------------------------------------------------------------------------------------
    // CPU 0 Level Data: Write Path Info For All Stage
    // --------------------------------------------------------------------------------------------

    // Header length of level sections
    const static int LEVEL_HEADER = 9 * sizeof(uint32_t);
    const static int END_MARKER   = sizeof(uint16_t);

    for (int i = 0; i < LEVELS; i++)
    {
        if (VERBOSE) std::cout << std::hex << "Level Data Start: " << i << "," << pos << std::endl;
        LevelData* level = levels->getMappedLevelP(i);

        // First step is to write level header
        const int DATA_START   = LEVEL_HEADER + pos;
        const int CURVE_LENGTH = DATA_START + 38 + (3 * sizeof(uint16_t) * level->points->size()) + END_MARKER;
        const int WIDTH_LENGTH = CURVE_LENGTH    + (4 * sizeof(uint16_t) * (level->widthP.size() + level->heightP.size())) + END_MARKER;

        outu32(DATA_START);      // Sky Palette Entries
        outu32(DATA_START + 2);  // Road Stripe Centre Palette Entries
        outu32(DATA_START + 10); // Road Stripe Palette Entries
        outu32(DATA_START + 18); // Road Side Palette Entries
        outu32(DATA_START + 26); // Road Palette Entries
        outu32(DATA_START + 34); // Ground Palette Entries
        outu32(DATA_START + 38); // Curve Data
        outu32(CURVE_LENGTH);    // Width/Height Data
        outu32(WIDTH_LENGTH);    // Sprite Data

        // Write Level Palette Data
        uint32_t* pal = level->pal->road[level->roadPal];
        outu16(level->skyPal);
        outu32(pal[LevelPalette::CENTRE1]);
        outu32(pal[LevelPalette::CENTRE2]);
        outu32(pal[LevelPalette::STRIPE1]);
        outu32(pal[LevelPalette::STRIPE2]);
        outu32(pal[LevelPalette::SIDE1]);
        outu32(pal[LevelPalette::SIDE2]);
        outu32(pal[LevelPalette::ROAD1]);
        outu32(pal[LevelPalette::ROAD2]);
        outu16(level->gndPal);
        outu16(level->gndPal);

        writeCurveData(out, level);
        writeWidthHeightData(out, level);
        writeSpriteData(out, level);
    }

    // --------------------------------------------------------------------------------------------
    // CPU 1 End Section Data: Write Path Info For All Sections
    // End sections share the same road path
    // --------------------------------------------------------------------------------------------

    if (VERBOSE) std::cout << std::hex << "End Section CPU 1: " << pos << std::endl;
    writeCPU1Path(out, levels->getMappedLevelP(LEVELS), LevelData::END_LENGTH_CPU1);

    // --------------------------------------------------------------------------------------------
    // CPU 0 End Section Data: Write Path Info For All Sections
    // --------------------------------------------------------------------------------------------
    {
        for (int i = LEVELS; i < Levels::MAP_SLOTS; i++)
        {
            LevelData* level = levels->getMappedLevelP(i);
            if (VERBOSE) std::cout << std::hex << "End Section Start: " << i << "," << pos << std::endl;
            const static int LEVEL_HEADER = 3 * sizeof(uint32_t);
            const static int END_MARKER   = sizeof(uint16_t);

            const int DATA_START   = LEVEL_HEADER + pos;
            const int CURVE_LENGTH = DATA_START   + (3 * sizeof(uint16_t) * level->points->size()) + END_MARKER;
            const int WIDTH_LENGTH = CURVE_LENGTH + (4 * sizeof(uint16_t) * (level->widthP.size() + level->heightP.size())) + END_MARKER;

            outu32(DATA_START);   // Curve Data
            outu32(CURVE_LENGTH); // Width/Height Data
            outu32(WIDTH_LENGTH); // Sprite Data

            writeCurveData(out, level, true); // Note we invert the curve data for end sections to match the original game
            writeWidthHeightData(out, level);
            writeSpriteData(out, level);
        }
    }

    // --------------------------------------------------------------------------------------------
    // Split CPU 1
    // --------------------------------------------------------------------------------------------
    if (VERBOSE) std::cout << std::hex << "Split CPU 1: " << pos << std::endl;
    writeCPU1Path(out, splitLevel, LevelData::SPLIT_LENGTH_CPU1);

    // --------------------------------------------------------------------------------------------
    // Split CPU 0
    // --------------------------------------------------------------------------------------------
    {
        if (VERBOSE) std::cout << std::hex << "Split CPU 0: " << pos << std::endl;
        const static int LEVEL_HEADER = 3 * sizeof(uint32_t);
        const static int END_MARKER   = sizeof(uint16_t);

        const int DATA_START   = LEVEL_HEADER + pos;
        const int CURVE_LENGTH = DATA_START   + (3 * sizeof(uint16_t) * splitLevel->points->size()) + END_MARKER;
        const int WIDTH_LENGTH = CURVE_LENGTH + (2 * sizeof(uint32_t)) + END_MARKER;

        outu32(DATA_START);   // Curve Data
        outu32(CURVE_LENGTH); // Width/Height Data
        outu32(WIDTH_LENGTH); // Sprite Data

        writeCurveData(out, splitLevel);
        outu32(0);          // Width/Height Data (Just Output Blank!)
        outu32(0);
        outu16(0x7FFF);
        writeSpriteData(out, splitLevel);
    }


    // --------------------------------------------------------------------------------------------
    // Write Shared Mapping Data
    // --------------------------------------------------------------------------------------------

    // Sky Palette Entries
    if (VERBOSE) std::cout << std::hex << "Sky Data Start: " << pos << std::endl;

    // Write pointers to data
    uint32_t skyData = pos + (LevelPalette::SKY_PALS * sizeof(uint32_t)); // Start of actual data
    for (int pal = 0; pal < LevelPalette::SKY_PALS; pal++)
    {
        //std::cout << "write sky pointer at: " << (skyData) << std::endl;
        outu32(skyData);
        skyData += LevelPalette::SKY_LENGTH * sizeof(uint32_t);
    }

    for (int pal = 0; pal < LevelPalette::SKY_PALS; pal++)
    {
        //std::cout << "write sky palette at: " << (pos ) << std::endl;
        for (int i = 0; i < LevelPalette::SKY_LENGTH; i++)
        {
            outu32(levelData->pal->sky[pal][i]);
        }
    }

    // Ground Palette Entries
    if (VERBOSE)  std::cout << std::hex << "Ground Data Start: " << pos << std::endl;

    // Write pointers to data
    uint32_t gndData = pos + (LevelPalette::GND_PALS * sizeof(uint32_t)); // Start of actual data
    for (int pal = 0; pal < LevelPalette::GND_PALS; pal++)
    {
        outu32(gndData);
        gndData += LevelPalette::GND_LENGTH * sizeof(uint32_t);
    }

    for (int pal = 0; pal < LevelPalette::GND_PALS; pal++)
    {
        for (int i = 0; i < LevelPalette::GND_LENGTH; i++)
        {
            outu32(levelData->pal->gnd[pal][i]);
        }
    }

    if (VERBOSE) std::cout << std::hex << "Sprite Map Start: " << pos << std::endl;
    writeSpriteMaps(out, spriteMaps);

    if (VERBOSE) std::cout << std::hex << "Height Map Start: " << pos << std::endl;
    writeHeightMaps(out, heightMaps);

    file.close();
}

void ExportCannonball::writeCPU1Path(QDataStream& out, LevelData* level, const int length)
{
    const int finalX = level->path[level->end_pos-1].x();
    const int finalY = level->path[level->end_pos-1].y();

    // Fill horizon / remainder area of level with dummy data
    for (int j = level->end_pos; j < length; j++)
    {
        level->path[j].setX(finalX);
        level->path[j].setY(finalY);
    }

    for (int j = 0; j < length; j++)
    {
        out16(level->path[j].x());
        out16(level->path[j].y());
    }
}

int ExportCannonball::getLevelLength(LevelData *level)
{
    const static int LEVEL_HEADER = 9 * sizeof(uint32_t);
    const static int END_MARKER   = sizeof(uint16_t);

    int length = LEVEL_HEADER;
    length += (8 * sizeof(uint32_t)) + (3 * sizeof(uint16_t));                                      // Length of palettes
    length += (3 * sizeof(uint16_t) * level->points->size()) + END_MARKER;                           // Length of curves
    length += (4 * sizeof(uint16_t) * (level->widthP.size()  + level->heightP.size())) + END_MARKER; // Length of width/heights
    length += (2 * sizeof(uint16_t) * level->spriteP.size()) + END_MARKER;                           // Length of sprites

    return length;
}

int ExportCannonball::getEndSectionLength(LevelData *level)
{
    const static int LEVEL_HEADER = 3 * sizeof(uint32_t);
    const static int END_MARKER   = sizeof(uint16_t);

    int length = LEVEL_HEADER;
    length += (3 * sizeof(uint16_t) * level->points->size()) + END_MARKER;                           // Length of curves
    length += (4 * sizeof(uint16_t) * (level->widthP.size()  + level->heightP.size())) + END_MARKER; // Length of width/heights
    length += (2 * sizeof(uint16_t) * level->spriteP.size()) + END_MARKER;                           // Length of sprites

    return length;
}

int ExportCannonball::getSplitLength(LevelData *level)
{
    const static int LEVEL_HEADER = 3 * sizeof(uint32_t);
    const static int END_MARKER   = sizeof(uint16_t);

    int length = LEVEL_HEADER;
    length += (3 * sizeof(uint16_t) * level->points->size()) + END_MARKER; // Length of curves
    length += (2 * sizeof(uint32_t))                         + END_MARKER; // Length of width/heights
    length += (2 * sizeof(uint16_t) * level->spriteP.size()) + END_MARKER; // Length of sprites

    return length;
}

void ExportCannonball::writeCurveData(QDataStream& out, LevelData* level, bool invertCurve)
{
    foreach (PathPoint pp, (*level->points))
    {
        // Curve Info: Average Distance Between Points On Curve
        const QPoint p1 = level->path[pp.pos];
        const QPoint p2 = level->path[pp.pos+1];
        const int xdiff = p2.x() - p1.x();
        const int ydiff = p2.y() - p1.y();

        const double l = qSqrt((xdiff*xdiff)+(ydiff*ydiff));
        const int curveInfo = l == 0 ? 0 : (1.0 / l) * LevelData::FIXED_ONE;

        // Direction of Curve
        uint16_t curve = 1;

        // Check these curve values are correct.
        if (pp.angle_inc < 0)
            curve = invertCurve ? 2 : 3; // Left
        else if (pp.angle_inc > 0)
            curve = invertCurve ? 3 : 2; // Right

        outu16(pp.pos);
        out16 (curveInfo);
        outu16(curve);
    }
    // Write Curve End Marker
    outu16(0xFFFF);
}

void ExportCannonball::writeWidthHeightData(QDataStream& out, LevelData* level)
{
    QList<ControlPoint> combined;

    foreach (ControlPoint cp, level->widthP)
    {
        cp.type = 1; // width type
        insertAtPos(&combined, cp);
    }
    foreach (ControlPoint cp, level->heightP)
    {
        cp.type = 0; // height type
        insertAtPos(&combined, cp);
    }

    foreach (ControlPoint cp, combined)
    {
        //std::cout << "CP: " << cp.pos << ":" << cp.type << std::endl;
        outu16(cp.pos);    // Position
        outu16(cp.type);   // Section Type [Height]
        outu16(cp.value1); // Height Index
        outu16((cp.type == 0 ? 0 : cp.value2));
    }

    // Write Width/Height End Marker
    outu16(0x7FFF);
}

void ExportCannonball::writeSpriteData(QDataStream& out, LevelData* level)
{
    foreach (ControlPoint cp, level->spriteP)
    {
        out16(cp.pos);
        outu8(cp.value1); // No of sprite in segment
        outu8(cp.value2); // Sprite index
    }

    // Write Sprite End Marker
    outu16(0x7FFF);
}

void ExportCannonball::insertAtPos(QList<ControlPoint>* list, ControlPoint cp)
{
    int insert_pos = 0;

    foreach (ControlPoint item, *list)
    {
        if (item.pos < cp.pos)
            insert_pos++;
    }

    list->insert(insert_pos, cp);
}

void ExportCannonball::writeHeightMaps(QDataStream &out, QList<HeightSegment> heightMaps)
{
    // Start of actual data
    uint32_t heightMapData = pos + (heightMaps.size() * sizeof(uint32_t));

    // Write Pointers
    foreach (HeightSegment seg, heightMaps)
    {
        outu32(heightMapData);
        heightMapData += sizeof(int16_t) * 2;
        if (seg.type != 4)
            heightMapData += (seg.data.size() + 1) * sizeof(int16_t);
    }

    // Write Height Map Contents
    foreach (HeightSegment seg, heightMaps)
    {
        outu8(seg.type);
        outu8(seg.step);

        switch (seg.type)
        {
        case 0:
            outu8(seg.value1);
            outu8(seg.value2);
            break;

        case 1:
        case 2:
        case 3:
        case 4:
            out16(seg.value1);
            break;
        }

        if (seg.type != 4)
        {
            foreach (int16_t v, seg.data)
            {
                out16(v);
            }

            out16(-1);
        }
    }
}

void ExportCannonball::writeSpriteMaps(QDataStream &out, QList<SpriteSectionEntry> spriteMaps)
{
    // Start of actual data
    uint32_t spriteMapData = pos + (spriteMaps.size() * sizeof(uint32_t));

    // Write Pointers
    foreach (SpriteSectionEntry seg, spriteMaps)
    {
        outu32(spriteMapData);
        spriteMapData += ((seg.sprites.size() * 8) * sizeof(uint8_t)) + (sizeof(uint16_t) * 2);
    }

    // Write Spritemap Contents
    foreach (SpriteSectionEntry seg, spriteMaps)
    {
        outu16(seg.frequency);
        uint16_t offset = (seg.sprites.size() - 1) * 8;
        out16 (offset);

        foreach(SpriteEntry entry, seg.sprites)
        {
            outu8(entry.props);
            out8 (entry.x);
            out16(entry.y);
            outu8(0);
            outu8(entry.type);
            outu8(0);
            outu8(entry.pal);
        }
    }
}

int ExportCannonball::getSpriteMapLength(QList<SpriteSectionEntry> spriteMaps)
{
    int length = spriteMaps.size() * sizeof(uint32_t);
    foreach (SpriteSectionEntry seg, spriteMaps)
        length += ((seg.sprites.size() * 8) * sizeof(uint8_t)) + (sizeof(uint16_t) * 2);
    return length;
}
