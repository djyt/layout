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

#include <QtCore/qmath.h>
#include "leveldata.hpp"

// Global reference to the current level
LevelData* levelData;

LevelData::LevelData(LevelPalette* pal, int type, QList<PathPoint> *points)
{
    this->pal    = pal;
    this->type   = type;

    // Ability to share path data for end sections
    if (points == NULL)
    {
        this->points = &pointsInternal;
    }
    else
    {
        this->points = points;
    }

    switch (type)
    {
        case 0:
            length = LEVEL_LENGTH;
            break;
        case 1:
            length = 400;
            break;
        case 2:
            length = 440;
            break;
    }

    path         = new QPoint[length + CPU1_EXTRA_LENGTH];
    path_render  = new QPoint[length + CPU1_EXTRA_LENGTH];
    width_render = new WidthRender[length + CPU1_EXTRA_LENGTH];
    end_pos = -1;
}

LevelData::~LevelData()
{
    if (path != NULL)
    {
        delete[] path;
        path = NULL;
    }

    if (path_render != NULL)
    {
        delete[] path_render;
        path_render = NULL;
    }

    if (width_render != NULL)
    {
        delete[] width_render;
        width_render = NULL;
    }
}

void LevelData::clear()
{
    if (type != 1)
        points->clear();
    widthP.clear();
    spriteP.clear();
    heightP.clear();
    end_pos = -1;

    // Road Palette Indexes
    skyPal  = 0;
    gndPal  = 0;
    roadPal = 0;

    startWidth = 0;
}

// Generate Road Values
void LevelData::updatePathData()
{
    qreal angle = 0;
    int pos = 0;

    for (int i = 0; i < points->size(); i++)
    {
        PathPoint rp = points->at(i);
        rp.pos = pos;
        points->replace(i, rp);

        qreal angle_inc = ((qreal) -rp.angle_inc / 10000);

        for (int j = 0; j < rp.length; j++)
        {
            // Convert floating point to fixed point
            int x = qSin(angle) * FIXED_ONE;
            int y = qCos(angle) * FIXED_ONE;

            path[pos].setX(x);
            path[pos].setY(y);

            angle += angle_inc;

            // Do not exceed allowed length of level
            if (++pos > length)
            {
                end_pos = length;
                updateRenderData();
                return;
            }
        }
    }

    end_pos = pos;
    updateRenderData();
}

// Get the rectangle co-ordinates containing the path
// This can probably be merged with the below
QRectF LevelData::getPathRect()
{
    const static int PADDING = 40;

    if (end_pos == 0)
    {
        return QRectF(0, 0, 0, 0);
    }

    // Update Rendered Path
    int xinc = 0;
    int yinc = 0;

    int xMin = INT_MAX;
    int xMax = INT_MIN;
    int yMin = INT_MAX;
    int yMax = INT_MIN;

    for (int i = 0; i < end_pos; i++)
    {
        xinc += path[i].x();
        yinc += -path[i].y();

        int xPos = xinc / (FIXED_ONE / 2);
        int yPos = yinc / (FIXED_ONE / 2);

        if (xPos < xMin) xMin = xPos;
        if (xPos > xMax) xMax = xPos;
        if (yPos < yMin) yMin = yPos;
        if (yPos > yMax) yMax = yPos;
    }

    //std::cout << xMin << "," << yMin << "," << (xMax - xMin) << "," << (yMax - yMin) << std::endl;

    return QRectF(xMin - PADDING, yMin - PADDING, (xMax - xMin) + (PADDING * 2), (yMax - yMin) + (PADDING * 2));
}

// Update Render Data From Path Data
void LevelData::updateRenderData()
{
    // Update Rendered Path
    int xinc = 0;
    int yinc = 0;

    for (int i = 0; i < end_pos; i++)
    {
        xinc += path[i].x();
        yinc += -path[i].y();

        path_render[i].setX(xinc / (FIXED_ONE / 2));
        path_render[i].setY(yinc / (FIXED_ONE / 2));
    }

    int pos = 0;

    // Update Control Points from render data
    for (int i = 0; i < points->size(); i++)
    {
        PathPoint rp = points->at(i);
        rp.p.setX(path_render[pos].x());
        rp.p.setY(path_render[pos].y());
        points->replace(i, rp);

        pos += rp.length;
    }

    updateWidthData();
}

void LevelData::updateWidthData()
{
    // Scale values, to increase accuracy of parallel line rendering
    const int SCALE  = 8;

    // Each road width is a constant. It's really the separation between the roads that
    // creates the illusion of a road width.
    const int ROAD_WIDTH = 32 * SCALE;

    long next_width  = (startWidth * SCALE) << 16;
    long width       = next_width;
    int change       = 0;
    int point        = 0;

    for (int i = 0; i < end_pos; i++)
    {
        if (point < widthP.size())
        {
            ControlPoint rw = widthP.at(point);

            if (i == rw.pos)
            {
                next_width = (rw.value1 * SCALE) << 16;
                change     = rw.value2 * SCALE;
                if (next_width <= width)
                    change = -change;

                point++;
            }
        }

        if (change)
        {
            // Adjust width per road position
            width += ((0xD5 * change) << 4);

            if (change > 0 && width > next_width)
            {
                width = next_width;
                change = 0;
            }
            else if (change < 0 && width < next_width)
            {
                width = next_width;
                change = 0;
            }
        }

        // To plot a parallel point, we need to understand the direction, so we look ahead by EDGES_OFFSET
        int renderStart = i;

        int renderEnd = i + EDGES_OFFSET;
        if (renderEnd >= end_pos)
            renderEnd = end_pos - 1;

        if (renderEnd == i)
        {
            if (--renderStart < 0)
                renderStart = 0;
        }

        const int x1 = path_render[renderStart].x() * SCALE;
        const int y1 = path_render[renderStart].y() * SCALE;
        const int x2 = path_render[renderEnd].x() * SCALE;
        const int y2 = path_render[renderEnd].y() * SCALE;

        const int l  = qSqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));

        if (l != 0)
        {
            // Gap, or width between roads
            const int gap = (width / 15) >> 16;

            // Road 1
            /*const int l_x1 = x1 + w * (y2-y1) / l;
            const int l_y1 = y1 + w * (x1-x2) / l;
            const int r_x1 = x1 - w * (y2-y1) / l;
            const int r_y1 = y1 - w * (x1-x2) / l;*/
            const int road1_x1 = x1 - (gap - (ROAD_WIDTH >> 1)) * (y2-y1) / l;
            const int road1_y1 = y1 - (gap - (ROAD_WIDTH >> 1)) * (x1-x2) / l;
            const int road1_x2 = x1 - (gap + (ROAD_WIDTH >> 1)) * (y2-y1) / l;
            const int road1_y2 = y1 - (gap + (ROAD_WIDTH >> 1)) * (x1-x2) / l;

            // Road 2
            const int road2_x1 = x1 + (gap - (ROAD_WIDTH >> 1)) * (y2-y1) / l;
            const int road2_y1 = y1 + (gap - (ROAD_WIDTH >> 1)) * (x1-x2) / l;
            const int road2_x2 = x1 + (gap + (ROAD_WIDTH >> 1)) * (y2-y1) / l;
            const int road2_y2 = y1 + (gap + (ROAD_WIDTH >> 1)) * (x1-x2) / l;

            WidthRender *wr = &width_render[i];

            // Set Road 1 Edges
            wr->road1_lhs.setX(road1_x1 / SCALE);
            wr->road1_lhs.setY(road1_y1 / SCALE);
            wr->road1_rhs.setX(road1_x2 / SCALE);
            wr->road1_rhs.setY(road1_y2 / SCALE);

            // Set Road 2 Edges
            wr->road2_lhs.setX(road2_x1 / SCALE);
            wr->road2_lhs.setY(road2_y1 / SCALE);
            wr->road2_rhs.setX(road2_x2 / SCALE);
            wr->road2_rhs.setY(road2_y2 / SCALE);

            // Have roads split in two? Used to skip inner line rendering
            wr->split = (width >> 16) >= (250 * SCALE);
        }
    }
}

void LevelData::insertPathPoint(int index)
{
    // Use previous point as a basis
    if (index > 0)
    {
        PathPoint rp = points->at(index - 1);
        points->insert(index, rp);
    }
    // Position at start
    else
    {
        PathPoint rp;
        rp.length = 200;
        rp.angle_inc = 0;
        points->insert(0, rp);
    }

    const int newLength = (*points)[index].length;
    const int startPos  = (index - 1) >= 0 ? (*points)[index-1].pos + (*points)[index-1].length : 0;

    // We've Changed Length Of Track
    // So Iterate Control Points And Move Them Up
    insertControlPoints(startPos, newLength);
}

void LevelData::deletePathPoint(int index)
{
    const int oldLength = (*points)[index].length;
    const int startPos  = (*points)[index].pos;
    const int endPos    = startPos + oldLength;
    points->removeAt(index);
    removeControlPoints(startPos, endPos, oldLength);
}

void LevelData::insertControlPoints(const int startPos, const int change)
{
    // We've Changed Length Of Track
    // So Iterate Control Points And Move Them Up
    for (int i = 0; i < widthP.size(); i++)
    {
        if (widthP[i].pos > startPos)
            widthP[i].pos += change;
    }
    for (int i = 0; i < heightP.size(); i++)
    {
        if (heightP[i].pos > startPos)
            heightP[i].pos += change;
    }
    for (int i = 0; i < spriteP.size(); i++)
    {
        if (spriteP[i].pos > startPos)
            spriteP[i].pos += change;
    }
}

// When removing a path section, remove the control points on this section
// Move the control points ahead of it down
void LevelData::removeControlPoints(const int startPos, const int endPos, const int change)
{
    // Completely Remove Control Points On Section Of Track Removed
    for (int i = 0; i < widthP.size();)
    {
        if (widthP[i].pos >= startPos && widthP[i].pos < endPos)
            widthP.removeAt(i);
        else
            i++;
    }
    for (int i = 0; i < heightP.size();)
    {
        if (heightP[i].pos >= startPos && heightP[i].pos < endPos)
            heightP.removeAt(i);
        else
            i++;
    }
    for (int i = 0; i < spriteP.size();)
    {
        if (spriteP[i].pos >= startPos && spriteP[i].pos < endPos)
            spriteP.removeAt(i);
        else
            i++;
    }

    // We've Changed Length Of Track
    // So Iterate Control Points And Move Them Down
    for (int i = 0; i < widthP.size(); i++)
    {
        if (widthP[i].pos > startPos)
            widthP[i].pos -= change;
    }
    for (int i = 0; i < heightP.size(); i++)
    {
        if (heightP[i].pos > startPos)
            heightP[i].pos -= change;
    }
    for (int i = 0; i < spriteP.size(); i++)
    {
        if (spriteP[i].pos > startPos)
            spriteP[i].pos -= change;
    }
}

void LevelData::changePathLength(const int index, const int newLength)
{
    const int oldLength     = (*points)[index].length;
    (*points)[index].length = newLength;

    // Section has been shortened, delete control points on it
    // Move control points ahead down
    if (newLength < oldLength)
    {
        // Delete control points that were on this section
        const int startPos = (*points)[index].pos + newLength;
        const int endPos   = (*points)[index].pos + oldLength;
        removeControlPoints(startPos, endPos, oldLength - newLength);
    }
    // Section has been lengthed - just move control points ahead of it up
    else
    {
        const int startPos  = (*points)[index].pos;
        const int endPos    = startPos + oldLength;
        const int change    = newLength - oldLength;
        insertControlPoints(endPos, change);
    }
}

int LevelData::insertWidthPoint(int pos)
{
    ControlPoint wp;
    int insert_pos = 0;

    // Find right position to insert width
    foreach (ControlPoint cp, widthP)
    {
        if (cp.pos < pos) insert_pos++;
        else break;
    }

    // Use Previous Point As Basis
    if (insert_pos > 0)
    {
        wp.value2 = widthP.at(insert_pos - 1).value2;
        wp.value1 = widthP.at(insert_pos - 1).value1;
    }
    else
    {
        wp.value2 = 64;
        wp.value1 = startWidth;
    }

    wp.pos = pos;

    widthP.insert(insert_pos, wp);
    updateWidthData();
    return insert_pos;
}

int LevelData::insertHeightPoint(int pos)
{
    ControlPoint wp;
    int insert_pos = 0;

    foreach (ControlPoint cp, heightP)
    {
        if (cp.pos < pos) insert_pos++;
        else break;
    }

    // Use Previous Point As Basis
    if (insert_pos > 0)
    {
        wp.value1 = heightP.at(insert_pos - 1).value1;
        wp.value2 = heightP.at(insert_pos - 1).value2;
    }
    else
    {
        wp.value1 = 0;
        wp.value2 = 0;
    }

    wp.pos = pos;
    heightP.insert(insert_pos, wp);
    return insert_pos;
}

int LevelData::insertSceneryPoint(int pos)
{
    ControlPoint wp;
    int insert_pos = 0;
    foreach (ControlPoint cp, spriteP)
    {
        if (cp.pos < pos) insert_pos++;
        else break;
    }

    // Use Previous Point As Basis
    if (insert_pos > 0)
    {
        wp.value1 = spriteP.at(insert_pos - 1).value1;
        wp.value2 = spriteP.at(insert_pos - 1).value2;
    }
    else
    {
        wp.value1 = 0xFF;
        wp.value2 = 0;
    }
    wp.pos = pos;
    spriteP.insert(insert_pos, wp);
    return insert_pos;
}

// Convert Position to QPoint
QPoint LevelData::posToPoint(int pos)
{
    return path_render[pos];
}

// Split current path segment into two
int LevelData::splitPathPoints(int insertPos)
{
    int pointFound = -1;
    for (int i = 0; i < points->size(); i++)
    {
        if ((*points)[i].pos < insertPos)
            pointFound++;
        else
            break;
    }

    if (pointFound != -1)
    {
        int splitLength = (*points)[pointFound].length;

        // Reduce length of old point
        (*points)[pointFound].length = insertPos - (*points)[pointFound].pos;

        // Create new point at this position
        PathPoint pp;
        pp.pos       = insertPos;
        pp.length    = splitLength - (*points)[pointFound].length;
        pp.angle_inc = (*points)[pointFound].angle_inc;
        points->insert(pointFound+1, pp);
        updatePathData();
    }

    return pointFound;
}