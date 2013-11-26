/***************************************************************************
    S16 Rendering Widget.

    Renders an OutRun Scene.

    Uses elements of the CannonBall engine, but also modified to instantly
    draw the scene from any position.

    The original engine would have to iterate from position 0;

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>

#include "../import/romloader.hpp"
#include "../leveldata.hpp"
#include "../levels/levels.hpp"
#include "hwroad.hpp"
#include "hwsprites.hpp"
#include "oroad.hpp"
#include "osprites.hpp"
#include "renders16.hpp"

const static double POS_LENGTH = 10 * 12;  // Length of each segment

RenderS16::RenderS16(QWidget *parent) :
    QWidget(parent)
{
    rom0      = NULL;
    screen    = new QImage(S16_WIDTH, S16_HEIGHT, QImage::Format_RGB32);
    hwroad    = new HWRoad();
    hwsprites = new HWSprites(this);
    oroad     = NULL;
    osprites  = NULL;
    pixels    = new uint32_t[S16_WIDTH * S16_HEIGHT];

    for (int i = 0; i < S16_WIDTH * S16_HEIGHT; i++)
        pixels[i] = 0;

    guideLines    = GUIDES_OFF;
    sceneryGuides = false;
}

RenderS16::~RenderS16()
{
    delete screen;
    delete hwroad;
    delete hwsprites;
    if (oroad != NULL)
        delete oroad;
    if (osprites != NULL)
        delete osprites;

    delete pixels;
}

void RenderS16::setData(Levels *levels, QList<HeightSegment>* heightSections, QList<SpriteSectionEntry>* spriteSections,
                        RomLoader* rom0, RomLoader *sprites, RomLoader *rom1, RomLoader *roadRom)
{
    this->levels         = levels;
    this->heightSections = heightSections;
    this->rom0           = rom0;

    hwroad->init(roadRom->rom);
    oroad  = new ORoad(heightSections, hwroad, rom1);
    hwsprites->init(sprites->rom);
    osprites = new OSprites(hwsprites, spriteSections, oroad, rom0);

    init();
}

void RenderS16::init()
{
    oroad->init();
    oroad->road_width = levelData->startWidth << 16;
    oroad->road_width_bak = oroad->road_width >> 16;

    osprites->init();

    mousePress   = 0;
    horizonYOff  = 0;
    lastPos      = 0;

    setCameraX();
    setCameraY();
}

void RenderS16::setCameraX(int x)
{
    oroad->car_x_bak = x;
    redrawPos();
}

void RenderS16::setCameraY(int y)
{
    horizonYOff = y;
    redrawPos();
}

void RenderS16::setGuidelines(int guideLines)
{
    this->guideLines = guideLines;
    update();
}

void RenderS16::setSceneryGuides(bool enabled)
{
    sceneryGuides = enabled;
    update();
}

void RenderS16::redrawPos()
{
    setRoadPos(lastPos);
}

void RenderS16::setRoadPos(int pos)
{
    // Check valid position
    if (pos != -1 && pos < levelData->end_pos)
    {
        if (levelData->end_pos > 0)
        {
            if (DEBUG) std::cout << "tick road pos " << std::endl;
            // Scroll road surface
            if (pos > lastPos)
                oroad->pos_fine += 10;
            else if (pos < lastPos)
                oroad->pos_fine -= 10;

            lastPos = pos;

            oroad->road_pos = pos << 16;
            updateRoadWidth(pos);
            updateRoadHorizon(pos);
            updateRoadHeight(pos);
            oroad->tick();
            oroad->tick();
            oroad->tick();
            oroad->tick();
            updateSprites(pos);
        }
    }

    update();
}

// Swap Sprite RAM And Update Palette Data
void RenderS16::updateSprites(int posEnd)
{
    osprites->init(); // Clear all sprite entries
    osprites->update_shadow_offset(oroad->tilemap_h_target);

    // Find start Control Point to Iterate Back From
    int startIndex = 0;
    int posStart   = 0;
    for (int i = levelData->spriteP.size() - 1; i >= 0; i--)
    {
        ControlPoint cp = levelData->spriteP.at(i);
        if (cp.pos < posEnd - 64)
        {
            startIndex = i;
            break;
        }
    }

    // Then iterate from the previous point to that
    startIndex -= 2;
    if (startIndex < 0) startIndex = 0;

    // If level is mapped to Stage 0, we need to render the startline
    if (levels->levelContainsStartLine() && posEnd < 42)
    {
        osprites->init_startline_sprites();
        posStart = 0;
    }
    else if (startIndex < levelData->spriteP.size())
    {
        posStart = levelData->spriteP.at(startIndex).pos;
    }

    osprites->sprite_scroll_speed = 0;
    for (int i = posStart; i <= posEnd; i++)
    {
        const ControlPoint* cp = startIndex < levelData->spriteP.size() ? &levelData->spriteP.at(startIndex) : NULL;
        osprites->tick(i, cp);
        osprites->sprite_copy();

        if (cp != NULL && cp->pos <= i)
        {
            startIndex++;
        }
    }
    copySpritePalData();
}

// To update the road width, we must iterate through the entire level until we get to the
// relevant point.
void RenderS16::updateRoadWidth(int pos)
{
    int cpIndex   = 0;
    int width     = 0;                              // Segment road width
    int change    = 0;                              // Segment adjustment speed
    oroad->road_width = levelData->startWidth << 16; // Road Width at start of level

    // Iterate until current position
    for (int i = 0; i <= pos; i++)
    {
        if (cpIndex < levelData->widthP.size())
        {
            ControlPoint widthPoint = levelData->widthP.at(cpIndex);
            if (i == widthPoint.pos)
            {
                width  = widthPoint.value1 << 16;
                change = widthPoint.value2;

                if (width <= oroad->road_width)
                    change = -change;

                cpIndex++;
            }
        }

        if (change)
        {
            oroad->road_width += (0xD0 * change) << 4;
            if (change > 0)
            {
                if (oroad->road_width > width)
                {
                    oroad->road_width = width;
                    change = 0;
                }
            }
            else if (change < 0)
            {
                if (oroad->road_width < width)
                {
                    oroad->road_width = width;
                    change = 0;
                }
            }
        }
    }
    oroad->road_width_bak = oroad->road_width >> 16;
}

// To set the correct horizon height we must parse all horizon heights until the current position.
void RenderS16::updateRoadHorizon(int pos)
{
    // Set Default Horizon Value at start of level
    oroad->setHorizon(0x240 + horizonYOff);

    foreach (ControlPoint cp, levelData->heightP)
    {
        HeightSegment seg = heightSections->at(cp.value1);

        // Horizon Section
        if (seg.type == 4)
        {
            // No need to process segments after the current position
            // And if we're on a road horizon position at present, this needs
            // to be updated by updateRoadHeight which is more granular
            const double segLength = HEIGHT_LENGTH / (POS_LENGTH / seg.step);

            if (cp.pos + segLength > pos)
                return;

            oroad->setHorizon(seg.value1 + horizonYOff);
        }
    }
}

void RenderS16::updateRoadHeight(int pos)
{
    // Find closest height segment
    int index = -1;
    foreach (ControlPoint cp, levelData->heightP)
    {
        if (cp.pos <= pos)
            index++;
        else
            break;
    }

    // Closest Height Segment Found
    if (index != -1)
    {
        ControlPoint cp   = levelData->heightP.at(index);
        HeightSegment seg = heightSections->at(cp.value1);

        const int posInSeg = pos - cp.pos;         // Distance into height section

        switch (seg.type)
        {
        // ----------------------------------------------------------------------------------------
        // HEIGHTMAP TYPE 0: STANDARD ELEVATION
        // ----------------------------------------------------------------------------------------
        case 0:
        {
            double heightPos = 0;
            int segmentIndex = 0;

            // Store the position at which each height section actually starts
            QList<double> heightLengths;
            heightLengths.push_back(0);

            // Iterate height map to calculate which segment we've reached
            // segmentIndex will contain the correct segment within the heightmap
            for (int i = segmentIndex; i < seg.data.size(); i++)
            {
                int16_t v = seg.data.at(i);

                double segLength = 0;

                if (v == 0)     segLength = POS_LENGTH / seg.step;
                else if (v < 0) segLength = POS_LENGTH / (seg.step * seg.value1); // could be wrong way around
                else if (v > 0) segLength = POS_LENGTH / (seg.step * seg.value2); // could be wrong way around

                double increment = HEIGHT_LENGTH / segLength;

                if (heightPos + increment <= posInSeg)
                {
                    segmentIndex++;
                }
                heightPos += increment;
                heightLengths.push_back(heightPos);
            }


            if (segmentIndex < seg.data.size() - 5)
            {
                double currentLength = heightLengths.at(segmentIndex);

                // Calculate length of segment we're in.
                double segmentLength = heightLengths.at(segmentIndex + 1) - currentLength;

                // Calculate distance into segment
                double percentageIntoSegment = (posInSeg - heightLengths.at(segmentIndex)) / segmentLength;
                int heightStart = (HEIGHT_LENGTH * percentageIntoSegment);

                oroad->setHeightMap(cp.value1, segmentIndex, 0x100 + heightStart);
            }
            else
            {
                oroad->setHeightMap(0, 0, 0x100);
            }
        }
        break;
        // ----------------------------------------------------------------------------------------
        // HEIGHTMAP TYPE 1: HOLD SECTION
        // ----------------------------------------------------------------------------------------
        case 1:
        case 2:
        {
            const int delay = seg.value1;

            // Position length of parts 1 and 2 of height section
            const double segLength = HEIGHT_LENGTH / (POS_LENGTH / seg.step);

            // Position length of part 2 (the delayed section)
            // Approximation only sadly, due to a bug in the OutRun code.
            const double delayLength = delay / (POS_LENGTH / seg.step);
            const double segLength2 = segLength + delayLength;

            const double segLength3 = (segLength * 2) + delayLength;

            // Part 1: This scales height start from 256 to 512
            if (posInSeg < segLength)
            {
                double percentageIntoSegment = (segLength - posInSeg) / segLength;
                int heightStart = HEIGHT_LENGTH - (HEIGHT_LENGTH * percentageIntoSegment);
                oroad->setHeightMap(cp.value1, 0, 0x100 + heightStart, 0x100);
            }
            // Part 2: This scales delay down to zero
            else if (posInSeg < segLength2)
            {
                double percentageIntoSegment = (segLength2 - posInSeg) / delayLength;
                int newDelay = (delay * percentageIntoSegment);
                oroad->setHeightMap(cp.value1, 1, 0x1FF, 0x100);
                oroad->setDelay(newDelay);
            }
            // Part 3: This scales height start from 512 to 256
            else if (posInSeg < segLength3)
            {
                double percentageIntoSegment = (segLength3 - posInSeg) / segLength;
                int heightStart = (HEIGHT_LENGTH * percentageIntoSegment);
                oroad->setHeightMap(cp.value1, 1, 0x100 + heightStart, 0x100);
            }
            else
            {
                oroad->setHeightMap(0, 0, 0x100);
            }
        }
        break;
        // ----------------------------------------------------------------------------------------
        // HEIGHTMAP TYPE 3: MIXED HOLD SECTION
        // ----------------------------------------------------------------------------------------
        case 3:
        {
            const int delay = seg.value1;
            const double delayLength = delay / (POS_LENGTH / seg.step);
            const double endLength   = HEIGHT_LENGTH / (POS_LENGTH / seg.step);

            const double segLength  = HEIGHT_LENGTH / (POS_LENGTH / 4); // step is hard-coded to 4
            const double segLength2 = (segLength * 6) + delayLength;
            const double segLength3 = segLength2 + endLength;

            // Part 1: Positions 1 to 5. This scales height start from 256 to 511
            if (posInSeg < segLength * 6)
            {
                const int segmentIndex = posInSeg / segLength;
                const int finePos = (segLength * (segmentIndex + 1)) - posInSeg;
                const double percentageIntoSegment = (segLength - finePos) / segLength;
                const int heightStart = (HEIGHT_LENGTH * percentageIntoSegment);
                oroad->setHeightMap(cp.value1, segmentIndex, 0x100 + heightStart);
            }
            // Part 2: Hold on Entry 6
            else if (posInSeg < segLength2)
            {
                double percentageIntoSegment = (segLength2 - posInSeg) / delayLength;
                int newDelay = (delay * percentageIntoSegment);
                oroad->setHeightMap(cp.value1, 6, 0x1FF, 0x100);
                oroad->setDelay(newDelay);
            }
            // Part 3: This scales height start from 512 to 256
            else if (posInSeg < segLength3)
            {
                double percentageIntoSegment = (segLength3 - posInSeg) / endLength;
                int heightStart = (HEIGHT_LENGTH * percentageIntoSegment);
                oroad->setHeightMap(cp.value1, 6, 0x100 + heightStart, 0x100);
            }
            else
            {
                oroad->setHeightMap(0, 0, 0x100);
            }
        }
        break;
        // ----------------------------------------------------------------------------------------
        // CHANGE HORIZON
        // ----------------------------------------------------------------------------------------
        case 4:
        {
            const double segLength = HEIGHT_LENGTH / (POS_LENGTH / seg.step);
            int heightStart;
            if (posInSeg < segLength)
            {
                double percentageIntoSegment = (segLength - posInSeg) / segLength;
                heightStart = HEIGHT_LENGTH - (HEIGHT_LENGTH * percentageIntoSegment);
            }
            else
            {
                heightStart = 0xFF;
            }
            oroad->setHeightMap(cp.value1, 0, 0x100 + heightStart);
            oroad->removeUserOffset(horizonYOff);
            oroad->setHorizonMod(seg.value1 + horizonYOff);
        }
        break;

        } // end switch

    }
    // No Height Segment Found
    else
    {
        oroad->setHeightMap(0, 0, 0x100);
    }
}

// ------------------------------------------------------------------------------------------------
// Rendering
// ------------------------------------------------------------------------------------------------

void RenderS16::paintEvent(QPaintEvent*)
{
    // Roms not loaded yet
    if (rom0 == NULL)
        return;

    screen->fill(0);

    if (levelData->end_pos > 0)
        drawS16Frame();

    if (sceneryGuides)
        drawSelectedSprites();

    if (guideLines != GUIDES_OFF)
        drawGuidelines();

    // Stretches image to target
    QRect target(0, 0, width(), height());
    QPainter painter(this);
    painter.drawImage(target, *screen);
}

void RenderS16::drawS16Frame()
{
    if (DEBUG) std::cout << "drawS16Frame() " << std::endl;
    hwroad->render_background(pixels);
    hwroad->render_foreground(pixels);
    hwsprites->render(8, osprites->sprite_entries, osprites->sprite_count);

    uint32_t* pix  = pixels;

    // Lookup real RGB value from rgb array for backbuffer
    int x = 0;
    int y = 0;

    for (int i = 0; i < (S16_WIDTH * S16_HEIGHT); i++)
    {
        screen->setPixel(x, y, rgb[*(pix++) & ((S16_PALETTE_ENTRIES * 3) - 1)]);

        if (++x == S16_WIDTH)
        {
            x = 0;
            y++;
        }
    }
}

void RenderS16::drawGuidelines()
{
    int x = 0, y = 0, w = 0, h = 0;

    if (guideLines == GUIDES_43)
    {
        w = 320;
        h = 224;
    }
    else if (guideLines == GUIDES_169)
    {
        w = 398;
        h = 224;
    }

    x = (S16_WIDTH  - w) / 2;
    y = (S16_HEIGHT - h) / 2;

    QPainter painter(screen);
    painter.fillRect(0,   y, x, h, QColor(0,0,0,128)); // Left  Border
    painter.fillRect(x+w, y, x, h, QColor(0,0,0,128)); // Right Border
}

void RenderS16::drawSelectedSprites()
{
    QPainter painter(screen);

    for (int i = 0; i < osprites->sprite_count; i++)
    {
        osprite* spr = &osprites->sprite_entries[i];

        if (!spr->is_hidden() && spr->selected)
        {
            int x = std::min(spr->get_screen_x1(), spr->get_screen_x2());
            int y = std::min(spr->get_screen_y1(), spr->get_screen_y2());
            painter.fillRect(x + S16_X_OFF, y, spr->get_screen_width(), spr->get_screen_height(), QColor(255,255,255,96));
        }
    }
}

// ---------------------------------------------------------------------------
// Palette Handling Code
// ---------------------------------------------------------------------------

void RenderS16::writePal32(uint32_t* palAddr, const uint32_t data)
{
    uint32_t adr = *palAddr & 0x1fff;

    palette[adr]   = (data >> 24) & 0xFF;
    palette[adr+1] = (data >> 16) & 0xFF;
    palette[adr+2] = (data >> 8) & 0xFF;
    palette[adr+3] = data & 0xFF;

    refresh_palette(adr);
    refresh_palette(adr+2);

    *palAddr += 4;
}

uint16_t RenderS16::readPal16(uint32_t palAddr)
{
    uint32_t adr = palAddr & 0x1fff;
    return (palette[adr] << 8) | palette[adr+1];
}

void RenderS16::writePal32(uint32_t adr, const uint32_t data)
{
    adr &= 0x1fff;

    palette[adr]   = (data >> 24) & 0xFF;
    palette[adr+1] = (data >> 16) & 0xFF;
    palette[adr+2] = (data >> 8) & 0xFF;
    palette[adr+3] = data & 0xFF;
    refresh_palette(adr);
    refresh_palette(adr+2);
}

void RenderS16::refresh_palette(uint32_t palAddr)
{
    palAddr &= ~1;
    uint32_t rgbAddr = palAddr >> 1;
    uint32_t a = (palette[palAddr] << 8) | palette[palAddr + 1];
    uint32_t r = (a & 0x000f) << 1; // r rrr0
    uint32_t g = (a & 0x00f0) >> 3; // g ggg0
    uint32_t b = (a & 0x0f00) >> 7; // b bbb0
    if ((a & 0x1000) != 0)
        r |= 1; // r rrrr
    if ((a & 0x2000) != 0)
        g |= 1; // g gggg
    if ((a & 0x4000) != 0)
        b |= 1; // b bbbb

    r = r * 255 / 31;
    g = g * 255 / 31;
    b = b * 255 / 31;

    rgb[rgbAddr] = qRgb(r, g, b);

    // Create shadow / highlight colours at end of RGB array
    // The resultant values are the same as MAME
    r = r * 202 / 256;
    g = g * 202 / 256;
    b = b * 202 / 256;

    rgb[rgbAddr + S16_PALETTE_ENTRIES] =
    rgb[rgbAddr + (S16_PALETTE_ENTRIES * 2)] = qRgb(r, g, b);
}

// ------------------------------------------------------------------------------------------------
// PALETTE CODE FOR SPRITES AND ROAD
// ------------------------------------------------------------------------------------------------

// Palette Data. Stored in blocks of 32 bytes.
const uint32_t PAL_DATA = 0x14ED8;

// Palette Ram: Sprite Entries Start Here
static const uint32_t PAL_SPRITES = 0x121000;

// Copy Sprite Palette Data To Palette RAM On Vertical Interrupt
//
// Source Address: 0x858E
// Input:          Source address in rom of data format
// Output:         None

void RenderS16::copySpritePalData()
{
    // Return if no palette entries to copy
    if (osprites->pal_copy_count <= 0) return;

    for (int16_t i = 0; i < osprites->pal_copy_count; i++)
    {
        // Palette Data Source Offset (aligned to start of 32 byte boundry, * 5)
        uint16_t src_offset = osprites->pal_addresses[(i * 2) + 0] << 5;
        uint32_t src_addr = 2 + PAL_DATA + src_offset; // Source address in ROM

        uint16_t dst_offset = osprites->pal_addresses[(i * 2) + 1] << 5;
        uint32_t dst_addr = 2 + PAL_SPRITES + dst_offset;

        // Move 28 Bytes from ROM to palette RAM
        for (uint16_t j = 0; j < 7; j++)
        {
            writePal32(&dst_addr, rom0->read32(&src_addr));
        }
    }
    osprites->pal_copy_count = 0; // All entries copied
}

void RenderS16::setupRoadPalettes()
{
    setupSkyPalette();
    setupGroundColor();
    setupRoadCentre();
    setupRoadStripes();
    setupRoadSides();
    setupRoadColour();
}

// Setup sky palette. Can be a shaded effect of 1F entries.
// Source: 8CA4
void RenderS16::setupSkyPalette()
{
    // Address of sky palette information
    uint32_t dst = 0x120F00; // palette ram
    for (int16_t i = 0; i < LevelPalette::SKY_LENGTH; i++)
        writePal32(&dst, levelData->pal->sky[levelData->skyPal][i]);
}

// Initalise Colour Of Road Sides
// Source: 8ED2
void RenderS16::setupGroundColor()
{
    // Address of ground palette information
    uint32_t dst_pal_ground1 = 0x120840; // palette ram: ground 1
    uint32_t dst_pal_ground2 = 0x120860; // palette ram: ground 2

    for (int16_t i = 0; i < LevelPalette::GND_LENGTH; i++)
    {
        uint32_t data = levelData->pal->gnd[levelData->gndPal][i];
        writePal32(&dst_pal_ground1, data);
        writePal32(&dst_pal_ground2, data);
    }
}

void RenderS16::setupRoadCentre()
{
    writePal32(0x12080C, levelData->pal->road[levelData->roadPal][LevelPalette::CENTRE1]); // Road 1 Colours
    writePal32(0x12081C, levelData->pal->road[levelData->roadPal][LevelPalette::CENTRE2]); // Road 2 Colours
}

void RenderS16::setupRoadStripes()
{
    writePal32(0x120804, levelData->pal->road[levelData->roadPal][LevelPalette::STRIPE1]); // Road 1 Colours
    writePal32(0x120814, levelData->pal->road[levelData->roadPal][LevelPalette::STRIPE2]); // Road 2 Colours
}

void RenderS16::setupRoadSides()
{
    writePal32(0x120808, levelData->pal->road[levelData->roadPal][LevelPalette::SIDE1]);  // Road 1 Colours
    writePal32(0x120818, levelData->pal->road[levelData->roadPal][LevelPalette::SIDE2]);  // Road 2 Colours
}

void RenderS16::setupRoadColour()
{
    writePal32(0x120800, levelData->pal->road[levelData->roadPal][LevelPalette::ROAD1]);  // Road 1 Colours
    writePal32(0x120810, levelData->pal->road[levelData->roadPal][LevelPalette::ROAD2]);  // Road 2 Colours
}


// ------------------------------------------------------------------------------------------------
// MOUSE PRESSES
// ------------------------------------------------------------------------------------------------


void RenderS16::mousePressEvent(QMouseEvent *event)
{
    Qt::MouseButton button = event->button();

    if (button == Qt::LeftButton)
    {
        oldX = event->x();
        oldY = event->y();
        mousePress |= 1;
    }
}

void RenderS16::mouseReleaseEvent(QMouseEvent *event)
{
    Qt::MouseButton button = event->button();

    if (button == Qt::LeftButton)
    {
        mousePress &= ~1;
    }
}

void RenderS16::mouseMoveEvent(QMouseEvent *event)
{
    if (mousePress & 1)
    {
        // Adjust horizon
        int yDiff = (event->y() - oldY) * 6;

        if (yDiff != 0 && lastPos != -1)
        {
            oldY = event->y();
            horizonYOff += yDiff;
            if (horizonYOff < CAMERA_Y_MIN)
                horizonYOff = CAMERA_Y_MIN;
            else if (horizonYOff > CAMERA_Y_MAX)
                horizonYOff = CAMERA_Y_MAX;

            emit sendCameraY(horizonYOff);
        }

        // Adjust x
        int xDiff = (event->x() - oldX);

        if (xDiff != 0)
        {
            oldX = event->x();
            oroad->car_x_bak += xDiff;
            if (oroad->car_x_bak > CAMERA_X_MAX)
                oroad->car_x_bak = CAMERA_X_MAX;
            else if (oroad->car_x_bak < CAMERA_X_MIN)
                oroad->car_x_bak = CAMERA_X_MIN;

            emit sendCameraX(oroad->car_x_bak);
        }

        if (xDiff || yDiff)
            redrawPos();
    }
}

// Move the road
void RenderS16::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta();

    if (!numDegrees.isNull())
    {
        int xChange = numDegrees.x();
        int yChange = numDegrees.y();

        // Move Camera X (either using Shift modifier, or horizontal mouse wheel if it exists)
        if (event->modifiers().testFlag(Qt::ShiftModifier) || xChange)
        {
            int change = event->modifiers().testFlag(Qt::ShiftModifier) ? yChange : xChange;

            if (std::abs(change) >= 5)
                change /= 5;
            else
                change = change > 0 ? 1 : -1;
            oroad->car_x_bak += change;
            if (oroad->car_x_bak > CAMERA_X_MAX)
                oroad->car_x_bak = CAMERA_X_MAX;
            else if (oroad->car_x_bak < CAMERA_X_MIN)
                oroad->car_x_bak = CAMERA_X_MIN;

            emit sendCameraX(oroad->car_x_bak);
        }
        // Scroll Road Position
        else
        {
            if (std::abs(numDegrees.y()) >= 120)
                yChange /= 120;
            else
                yChange = numDegrees.y() > 0 ? 1 : -1;

            const int newPos = lastPos + yChange;
            if (newPos >= 0 && newPos < levelData->end_pos)
                emit sendNewPosition(newPos);
        }
    }

    event->accept();
}
