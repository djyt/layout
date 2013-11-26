/***************************************************************************
    Level Palette Widget

    Features:
    - UI to Set the palette of the sky, ground and road.
    - Interpolate between colours.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef LEVELPALETTEWIDGET_HPP
#define LEVELPALETTEWIDGET_HPP

#include <QWidget>
#include "stdint.hpp"

class PreviewPalette;
struct LevelPalette;

namespace Ui {
class RoadPaletteWidget;
}

class LevelPaletteWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit LevelPaletteWidget(QWidget *parent = 0, LevelPalette* roadPalette = NULL);
    ~LevelPaletteWidget();
    void refresh();

signals:
    void refreshPalette();
    void refreshPreview();

private slots:
    void setRoad1Back(int entry, uint16_t s16Color);
    void setRoad1Cent(int entry, uint16_t s16Color);
    void setRoad1Edge(int entry, uint16_t s16Color);
    void setRoad1Stripe(int entry, uint16_t s16Color);
    void setRoad2Back(int entry, uint16_t s16Color);
    void setRoad2Cent(int entry, uint16_t s16Color);
    void setRoad2Edge(int entry, uint16_t s16Color);
    void setRoad2Stripe(int entry, uint16_t s16Color);
    void setSky(int entry, uint16_t s16Color);
    void setSkyIndex(int value);
    void setGnd(int entry, uint16_t s16Color);
    void setGndIndex(int value);
    void setRoadIndex(int value);
    void copyRoad1Palette();
    void copyRoad2Palette();
    void fillSky();
    void interpolateSky();
    void interpolateGround();
    
private:
    Ui::RoadPaletteWidget *ui;
    LevelPalette *pal;

    void setupWidget(PreviewPalette* widget, uint32_t color);
    void setRoadColor(uint32_t *p, int entry, uint16_t s16Color);
    void refreshRoadPalette();
    void refreshSkyPalette();
    void refreshGndPalette();
    int interpolate(int startValue, int endValue, int stepNumber, int lastStepNumber);
};

#endif // LEVELPALETTEWIDGET_HPP
