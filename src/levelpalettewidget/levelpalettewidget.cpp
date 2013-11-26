/***************************************************************************
    Level Palette Widget

    Features:
    - UI to Set the palette of the sky, ground and road.
    - Interpolate between colours.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "leveldata.hpp"
#include "utils.hpp"
#include "levelpalettewidget.hpp"
#include "ui_levelpalettewidget.h"

LevelPaletteWidget::LevelPaletteWidget(QWidget *parent, LevelPalette *levelPal) :
    QWidget(parent),
    ui(new Ui::RoadPaletteWidget)
{
    pal = levelPal;

    ui->setupUi(this);

    ui->spinRoadPal->setRange(0, LevelPalette::ROAD_PALS - 1);
    ui->spinSkyPal->setRange (0, LevelPalette::SKY_PALS - 1);
    ui->spinGndPal->setRange (0, LevelPalette::GND_PALS - 1);

    connect(ui->road1Back,     SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad1Back(int, uint16_t)));
    connect(ui->road1Cent,     SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad1Cent(int, uint16_t)));
    connect(ui->road1Edge,     SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad1Edge(int, uint16_t)));
    connect(ui->road1Stripe,   SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad1Stripe(int, uint16_t)));
    connect(ui->road2Back,     SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad2Back(int, uint16_t)));
    connect(ui->road2Cent,     SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad2Cent(int, uint16_t)));
    connect(ui->road2Edge,     SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad2Edge(int, uint16_t)));
    connect(ui->road2Stripe,   SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setRoad2Stripe(int, uint16_t)));
    connect(ui->sky,           SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setSky(int,uint16_t)));
    connect(ui->spinSkyPal,    SIGNAL(valueChanged(int)),        this,           SLOT(setSkyIndex(int)));
    connect(ui->ground,        SIGNAL(sendColor(int, uint16_t)), this,           SLOT(setGnd(int,uint16_t)));
    connect(ui->spinGndPal,    SIGNAL(valueChanged(int)),        this,           SLOT(setGndIndex(int)));
    connect(ui->spinRoadPal,   SIGNAL(valueChanged(int)),        this,           SLOT(setRoadIndex(int)));

    connect(ui->buttonInt,     SIGNAL(clicked()),                this,           SLOT(interpolateSky()));
    connect(ui->buttonFill,    SIGNAL(clicked()),                this,           SLOT(fillSky()));
    connect(ui->buttonIntG,    SIGNAL(clicked()),                this,           SLOT(interpolateGround()));
    connect(ui->buttonCopyTo1, SIGNAL(clicked()),                this,           SLOT(copyRoad2Palette()));
    connect(ui->buttonCopyTo2, SIGNAL(clicked()),                this,           SLOT(copyRoad1Palette()));
    connect(ui->sky,           SIGNAL(isSelected(bool)),         ui->buttonInt,  SLOT(setEnabled(bool)));
    connect(ui->sky,           SIGNAL(isSelected(bool)),         ui->buttonFill, SLOT(setEnabled(bool)));

    // Allow palette entries to be selected with right click
    ui->sky->setSelection(true);
}

LevelPaletteWidget::~LevelPaletteWidget()
{
    delete ui;
}

void LevelPaletteWidget::refresh()
{
    refreshRoadPalette();
    ui->spinRoadPal->setValue(levelData->roadPal);
    refreshSkyPalette();
    ui->spinSkyPal->setValue(levelData->skyPal);
    refreshGndPalette();
    ui->spinGndPal->setValue(levelData->gndPal);
}

void LevelPaletteWidget::refreshRoadPalette()
{
    uint32_t* roadPal = pal->road[levelData->roadPal];

    setupWidget(ui->road1Back,   roadPal[LevelPalette::ROAD1]);
    setupWidget(ui->road1Cent,   roadPal[LevelPalette::CENTRE1]);
    setupWidget(ui->road1Edge,   roadPal[LevelPalette::SIDE1]);
    setupWidget(ui->road1Stripe, roadPal[LevelPalette::STRIPE1]);
    setupWidget(ui->road2Back,   roadPal[LevelPalette::ROAD2]);
    setupWidget(ui->road2Cent,   roadPal[LevelPalette::CENTRE2]);
    setupWidget(ui->road2Edge,   roadPal[LevelPalette::SIDE2]);
    setupWidget(ui->road2Stripe, roadPal[LevelPalette::STRIPE2]);
}

void LevelPaletteWidget::refreshSkyPalette()
{
    // Sky palette (Note we invert this palette in terms of order for usability)
    int src = pal->SKY_LENGTH;
    QRgb* rgb = new QRgb[pal->SKY_LENGTH * 2];
    for (int i = 0; i < pal->SKY_LENGTH * 2;)
    {
        uint32_t color = pal->sky[levelData->skyPal][--src];
        rgb[i++] = Utils::convertToQT(color & 0xFFFF);
        rgb[i++] = Utils::convertToQT(color >> 16);
    }
    ui->sky->setPaletteData(rgb, 8, 8);
    ui->sky->setPalette(0);
}

void LevelPaletteWidget::refreshGndPalette()
{
    // Gnd palette (Note we invert this palette in terms of order for usability)
    int src = pal->GND_LENGTH;
    QRgb* rgb = new QRgb[pal->GND_LENGTH * 2];
    for (int i = 0; i < pal->GND_LENGTH * 2;)
    {
        uint32_t color = pal->gnd[levelData->gndPal][--src];
        rgb[i++] = Utils::convertToQT(color & 0xFFFF);
        rgb[i++] = Utils::convertToQT(color >> 16);
    }
    ui->ground->setPaletteData(rgb, 8, 2);
    ui->ground->setPalette(0);
}

void LevelPaletteWidget::setupWidget(PreviewPalette* widget, uint32_t color)
{
    QRgb* rgb = new QRgb[2];
    rgb[0] = Utils::convertToQT(color >> 16);
    rgb[1] = Utils::convertToQT(color & 0xFFFF);
    widget->setPaletteData(rgb, 2, 1);
    widget->setPalette(0);
}

void LevelPaletteWidget::setRoad1Back(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::ROAD1], entry, s16Color);
}

void LevelPaletteWidget::setRoad1Cent(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::CENTRE1], entry, s16Color);
}

void LevelPaletteWidget::setRoad1Edge(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::SIDE1], entry, s16Color);
}

void LevelPaletteWidget::setRoad1Stripe(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::STRIPE1], entry, s16Color);
}

void LevelPaletteWidget::setRoad2Back(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::ROAD2], entry, s16Color);
}

void LevelPaletteWidget::setRoad2Cent(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::CENTRE2], entry, s16Color);
}

void LevelPaletteWidget::setRoad2Edge(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::SIDE2], entry, s16Color);
}

void LevelPaletteWidget::setRoad2Stripe(int entry, uint16_t s16Color)
{
    setRoadColor(&pal->road[levelData->roadPal][LevelPalette::STRIPE2], entry, s16Color);
}

// Change a colour in a Sky Palette
void LevelPaletteWidget::setSky(int entry, uint16_t s16Color)
{
    // (Note we invert this palette in terms of order for usability)
    setRoadColor(&pal->sky[levelData->skyPal][pal->SKY_LENGTH - (entry >> 1) - 1], entry ^ 1, s16Color);
}

// Select Specific Sky Palette
void LevelPaletteWidget::setSkyIndex(int value)
{
    levelData->skyPal = value;
    refreshSkyPalette();
    emit refreshPreview();
    emit refreshPalette();
}

// Change a colour in a Ground Palette
void LevelPaletteWidget::setGnd(int entry, uint16_t s16Color)
{
    // (Note we invert this palette in terms of order for usability)
    setRoadColor(&pal->gnd[levelData->gndPal][pal->GND_LENGTH - (entry >> 1) - 1], entry ^ 1, s16Color);
}

// Select Specific Ground Palette
void LevelPaletteWidget::setGndIndex(int value)
{
    levelData->gndPal = value;
    refreshGndPalette();
    emit refreshPreview();
    emit refreshPalette();
}

// Select Specific Road Palette
void LevelPaletteWidget::setRoadIndex(int value)
{
    levelData->roadPal = value;
    refreshRoadPalette();
    emit refreshPreview();
    emit refreshPalette();
}

void LevelPaletteWidget::setRoadColor(uint32_t* p, int entry, uint16_t s16Color)
{
    if (entry & 1)
    {
        *p &= 0xFFFF0000;
        *p |= s16Color;
    }
    else
    {
        *p &= 0x0000FFFF;
        *p |= (s16Color << 16);
    }

    emit refreshPalette();
}

// Copy Road 1 Palette to Road 2
void LevelPaletteWidget::copyRoad1Palette()
{
    uint32_t* roadPal = pal->road[levelData->roadPal];
    roadPal[LevelPalette::ROAD2]   = roadPal[LevelPalette::ROAD1];
    roadPal[LevelPalette::SIDE2]   = roadPal[LevelPalette::SIDE1];
    roadPal[LevelPalette::STRIPE2] = roadPal[LevelPalette::STRIPE1];
    roadPal[LevelPalette::CENTRE2] = roadPal[LevelPalette::CENTRE1];
    refresh();
    emit refreshPreview();
    emit refreshPalette();
}

// Copy Road 2 Palette to Road 1
void LevelPaletteWidget::copyRoad2Palette()
{
    uint32_t* roadPal = pal->road[levelData->roadPal];
    roadPal[LevelPalette::ROAD1]   = roadPal[LevelPalette::ROAD2];
    roadPal[LevelPalette::SIDE1]   = roadPal[LevelPalette::SIDE2];
    roadPal[LevelPalette::STRIPE1] = roadPal[LevelPalette::STRIPE2];
    roadPal[LevelPalette::CENTRE1] = roadPal[LevelPalette::CENTRE2];
    refresh();
    emit refreshPreview();
    emit refreshPalette();
}

void LevelPaletteWidget::fillSky()
{
    const int startIndex = ui->sky->getSelectedIndex();

    if (startIndex == -1)
        return;

    const int length = pal->SKY_LENGTH * 2;
    QRgb* rgb = ui->sky->getPaletteData();
    QColor startColor = QColor(rgb[startIndex]);

    for (int i = startIndex; i < length; i++)
    {
        rgb[i] = startColor.rgb();
        const uint16_t color = Utils::convertToS16(rgb[i]);

        uint32_t *p = &pal->sky[levelData->skyPal][pal->SKY_LENGTH - 1 - (i >> 1)];
        *p = (i & 1) ? ((*p & 0x00000FFFF) + (color << 16))
                     : ((*p & 0xFFFFF0000) + color);
    }

    refreshSkyPalette();
    emit refreshPreview();
    emit refreshPalette();
}

void LevelPaletteWidget::interpolateSky()
{
    const int startIndex = ui->sky->getSelectedIndex();

    if (startIndex == -1)
        return;

    const int length = pal->SKY_LENGTH * 2;
    const int steps  = length - startIndex;

    QRgb* rgb = ui->sky->getPaletteData();

    QColor startColor = QColor(rgb[startIndex]);
    QColor endColor   = QColor(rgb[length-1]);

    const int r1 = startColor.red();
    const int g1 = startColor.green();
    const int b1 = startColor.blue();

    const int r2 = endColor.red();
    const int g2 = endColor.green();
    const int b2 = endColor.blue();

    int step = 0;

    for (int i = startIndex; i < length; i++)
    {
        int r = interpolate(r1, r2, step, steps);
        int g = interpolate(g1, g2, step, steps);
        int b = interpolate(b1, b2, step, steps);
        step++;

        //std::cout << r << "," << g << "," << b << std::endl;

        rgb[i] = QColor(r, g, b).rgb();
        const uint16_t color = Utils::convertToS16(rgb[i]);

        uint32_t *p = &pal->sky[levelData->skyPal][pal->SKY_LENGTH - 1 - (i >> 1)];
        *p = (i & 1) ? ((*p & 0x00000FFFF) + (color << 16))
                     : ((*p & 0xFFFFF0000) + color);
    }

    refreshSkyPalette();
    emit refreshPreview();
    emit refreshPalette();
}

void LevelPaletteWidget::interpolateGround()
{
    const int startIndex = 0;
    const int length = pal->GND_LENGTH * 2;
    const int steps  = 11;

    QRgb* rgb = ui->ground->getPaletteData();

    QColor startColor = QColor(rgb[startIndex]);
    QColor endColor   = QColor(rgb[length-1]);

    const int r1 = startColor.red();
    const int g1 = startColor.green();
    const int b1 = startColor.blue();

    const int r2 = endColor.red();
    const int g2 = endColor.green();
    const int b2 = endColor.blue();

    int step = 0;

    for (int i = startIndex; i < length; i += 2)
    {
        // First Pass
        int r = interpolate(r1, r2, step, steps);
        int g = interpolate(g1, g2, step, steps);
        int b = interpolate(b1, b2, step, steps);
        rgb[i] = QColor(r, g, b).rgb();
        const uint16_t color1 = Utils::convertToS16(rgb[i]);

        // Second Pass
        r = interpolate(r1, r2, step + 3, steps);
        g = interpolate(g1, g2, step + 3, steps);
        b = interpolate(b1, b2, step + 3, steps);
        rgb[i+1] = QColor(r, g, b).rgb();
        const uint16_t color2 = Utils::convertToS16(rgb[i+1]);

        uint32_t *p = &pal->gnd[levelData->gndPal][pal->GND_LENGTH - 1 - (i >> 1)];
        *p = color1 + (color2 << 16);

        step++;
    }

    refreshGndPalette();
    emit refreshPreview();
    emit refreshPalette();
}

// Interpolate between two values.
int LevelPaletteWidget::interpolate(int startValue, int endValue, int stepNumber, int lastStepNumber)
{
    return (endValue - startValue) * stepNumber / lastStepNumber + startValue;
}
