/***************************************************************************
    RoadPathWidget

    - Handles the 2D Representation of an OutRun Track
    - Handles Editing Control Points And User Interaction

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef ROADPATH_HPP
#define ROADPATH_HPP

#include <QGraphicsView>
#include "roadpathscene.hpp"
#include "height/heightsection.hpp"
#include "sprites/spritesection.hpp"

class RoadPathWidget : public QGraphicsView
{
    Q_OBJECT

public:
    // The 2D Visualisation Of The Scene
    RoadPathScene* scene;

    static const int ZOOM_MIN = 20;
    static const int ZOOM_MAX = 300;
    int zoom;

    RoadPathWidget(QWidget *parent = 0);
    ~RoadPathWidget();
    void init();
    void setHeightSection(HeightSection* section);
    void setSpriteSection(SpriteSection* section);
    void setStatePath();
    void setStateWidth();
    void setStateHeight();
    void setStateScenery();
    void setStateView();
    void setZoom(int percentZoom);
    inline qreal curScale() const {return transform().m11();}
    int getCurrentCP();
    int getRoadPos();
    void lock();
    int getHeightSegLength(int);

signals:
    void setStatusBar(QString);
    void changeLength(int);
    void changeAngle(int);
    void changeWidth(int);
    void changeWidthSpeed(int);
    void changePatternLength(int);
    void enableControls(bool);
    void enableInsert(bool);
    void toggleLaneControls(bool);
    void refreshPreview(int);
    void refreshPreview();
    void selectHeightSection(int);
    void selectScenerySection(int);
    void setPercentage(int);
    void setEndPos(int);
    void changeZoom(int);

public slots:
    void setView(int value);
    void setRoadPos(int value);
    void setLength(int value);
    void setWidth(int value);
    void setPoint(int value);
    void setAngle(int value);
    void setWidthChange(int value);
    void insertPointAfter();
    void insertPointBefore();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    HeightSection* heightSection;
    SpriteSection* spriteSection;

    // Selected Points for each mode
    int* activePoint;
    int activePathPoint;
    int activeWidthPoint;
    int activeHeightPoint;
    int activeSceneryPoint;

    // Mouse is pressed
    bool mousePress;

    // Current road position of mouse pointer
    int mousePos;

    // Stored road position
    int roadPos;

    // Has point been dragged?
    bool pointDragged;

    // Do not attempt to centre the preview window
    bool centreViewPoint;

    qreal prevAngle;

    int  getPosInLevel(QPointF);
    void updateStatus(int);
    void updateControls();

    void updateScene();
    int getNearestRP(QPointF mousePos);
    int getNearestCP(QPointF mousePos);
    void roadLengthChanged();
    void manipulatePoint(int point, int pos);
};

#endif // ROADPATH_HPP
