/***************************************************************************
    S16 Rendering Widget.

    Renders an OutRun Scene.

    Uses elements of the CannonBall engine, but also modified to instantly
    draw the scene from any position.

    The original engine would have to iterate from position 0;

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef RENDERS16_HPP
#define RENDERS16_HPP

#include <QWidget>
#include "../globals.hpp"

class HWRoad;
class HWSprites;
class ORoad;
class OSprites;
class RomLoader;
class Levels;
struct HeightSegment;
struct SpriteSectionEntry;

class RenderS16 : public QWidget
{
    Q_OBJECT

public:
    const static int    CAMERA_X_MAX = 1500;
    const static int    CAMERA_X_MIN = -1500;
    const static int    CAMERA_Y_MAX = 3000;
    const static int    CAMERA_Y_MIN = -600;

    uint32_t* pixels; // Internal pixel array

    explicit RenderS16(QWidget *parent = 0);
    ~RenderS16();
    void setData(Levels* levels, QList<HeightSegment> *heightSections, QList<SpriteSectionEntry> *spriteSections,
                 RomLoader* rom0, RomLoader* sprites, RomLoader* rom1, RomLoader* roadRom);
    void init();
    
signals:
    void sendNewPosition(int);
    void sendCameraX(int);
    void sendCameraY(int);
    
public slots:
    void setupRoadPalettes();
    void redrawPos();
    void setRoadPos(int);
    void setGuidelines(int);
    void setSceneryGuides(bool);
    void setCameraX(int x = 0);
    void setCameraY(int y = 0);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    const static bool   DEBUG = false;

    Levels* levels;

    QList<HeightSegment>* heightSections;

    QImage* screen;   // Screen image

    HWRoad* hwroad;   // Road Code
    HWSprites* hwsprites;
    ORoad*  oroad;
    OSprites* osprites;
    RomLoader* rom0;

    int lastPos;
    int mousePress;
    int horizonYOff;
    int oldX, oldY;
    int guideLines;
    bool sceneryGuides;

    enum
    {
        GUIDES_OFF,
        GUIDES_43,
        GUIDES_169
    };

    uint8_t palette[S16_PALETTE_ENTRIES * 2]; // 2 Bytes Per Palette Entry
    QRgb rgb[S16_PALETTE_ENTRIES * 3];        // Extended to hold shadow/hilight colours

    void drawS16Frame();
    void drawGuidelines();
    void drawSelectedSprites();
    void updateSprites(int posEnd);
    void copySpritePalData();
    uint16_t readPal16(uint32_t);
    void writePal32(uint32_t*, const uint32_t);
    void writePal32(uint32_t, const uint32_t);
    void refresh_palette(uint32_t);
    void updateRoadWidth(int);
    void updateRoadHorizon(int);
    void updateRoadHeight(int);
    void setupSkyPalette();
    void setupGroundColor();
    void setupRoadCentre();
    void setupRoadStripes();
    void setupRoadSides();
    void setupRoadColour();  
};

#endif // RENDERS16_HPP
