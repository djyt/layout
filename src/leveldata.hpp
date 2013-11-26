/***************************************************************************
    Layout: A Track Editor for OutRun

    This class represents an individual level or road section.
    A road section is an end section or the split between levels.

    A level consists of:
    - A series of path points to set the direction
    - A series of width points to change the width
    - A series of scenery points to reference scenery patterns
    - A series of height points to reference height patterns
    - A palette for each road generator
    - A palette lookup for the sky
    - A palette lookup for the ground

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef ROADDATA_HPP
#define ROADDATA_HPP

#include <QString>
#include <QPoint>
#include <QVector>
#include <QStandardItem>

#include "globals.hpp"
#include "controlpoint.hpp"
#include "levels/levelpalette.hpp"
#include "height/heightformat.hpp"

struct PathPoint
{
    // Track Co-ordinates of this point.
    QPoint p;

    // Position of Change (Index into road)
    int pos;

    // Angle Increment from this road point to the next.
    int angle_inc;

    // Length of this segment.
    int length;
};

struct WidthRender
{
    // Have roads split at this point
    bool split;

    // Road Co-ordinates
    QPoint road1_lhs;
    QPoint road1_rhs;
    QPoint road2_lhs;
    QPoint road2_rhs;
};

class LevelData
{

public:
    // Fixed point '1' representation.
    const static int FIXED_ONE = 0x1000;

    // Fixed Length of normal level
    const static uint16_t LEVEL_LENGTH = 0x79C;

    // CPU 1 Requires the level to extend further for the horizon. 90 * 4 bytes.
    const static uint16_t CPU1_EXTRA_LENGTH = 90 * 4; // (0x168)

    // Level Length for CPU 1 with Look ahead (extend the level into the horizon)
    const static uint16_t LEVEL_LENGTH_CPU1 = LEVEL_LENGTH + CPU1_EXTRA_LENGTH;

    // Fixed Length of splits
    const static uint16_t SPLIT_LENGTH = 384;

    // Split Length for CPU1 with Look ahead
    // needs to be 0x962 / 4 in terms of length. 600 positions total. Or 0x258 in hex
    const static uint16_t SPLIT_LENGTH_CPU1 = SPLIT_LENGTH + CPU1_EXTRA_LENGTH;

    // needs to be 0x9c2 / 4 in terms of length. 624 positions total.
    const static uint16_t END_LENGTH_CPU1 = 624;

    // Amount of points to look ahead when plotting parallel segments
    const static int EDGES_OFFSET = 8;

    // Maximum length of each section
    const static int SECTION_LENGTH_MAX = 350;

    // Maximum angle for each section
    const static int SECTION_ANGLE_MAX = 252;

    // Level Type
    int type;

    // Length of this level
    int length;

    // End Position of level (always less than LEVEL_LENGTH)
    int end_pos;

    // Starting Width of Level (OutRun hard-codes the start width of the first track)
    int startWidth;

    // Road Points
    QList<PathPoint>* points;

    // Width Points
    QList<ControlPoint> widthP;

    // Sprite Points
    QList<ControlPoint> spriteP;

    // Height Points
    QList<ControlPoint> heightP;

    // Level Path
    QPoint *path;

    // Level Path (Rendered Version)
    QPoint *path_render;

    WidthRender* width_render;

    // Shared Palette Entries
    LevelPalette* pal;

    uint16_t skyPal;  // Address in palette RAM for SKY
    uint16_t gndPal;  // Address in palette RAM for GROUND
    uint16_t roadPal; // Road Palette

    LevelData(LevelPalette *pal, int type, QList<PathPoint>* points = NULL);
    ~LevelData();
    void clear();
    void updatePathData();
    QRectF getPathRect();
    void updateWidthData();
    void insertPathPoint(int index);
    void deletePathPoint(int index);
    void insertControlPoints(const int startPos, const int change);
    void removeControlPoints(const int startPos, const int endPos, const int change);
    void changePathLength(const int index, const int newLength);
    int  insertWidthPoint(int);
    int  insertHeightPoint(int);
    int  insertSceneryPoint(int);
    QPoint posToPoint(int);
    int splitPathPoints(int insertPos);

private:
    QList<PathPoint> pointsInternal;
    void updateRenderData();
};

extern LevelData* levelData;

#endif // ROADDATA_HPP
