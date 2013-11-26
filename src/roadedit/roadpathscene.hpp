/***************************************************************************
    RoadPathScene

    - Render a 2D Representation Of An OutRun Track.
    - Render Control Points.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef ROADPATHSCENE_HPP
#define ROADPATHSCENE_HPP

#include <QGraphicsScene>

class RoadPathWidget;
struct ControlPoint;

class RoadPathScene : public QGraphicsScene
{
    Q_OBJECT
public:

    // Viewing States
    static const int STATE_PATH  = 0;       // Edit Track Path
    static const int STATE_WIDTH = 1;       // Edit Track Width
    static const int STATE_HEIGHT = 2;      // Edit Track Height
    static const int STATE_SCENERY = 3;     // Edit Track Scenery
    static const int STATE_VIEW = 4;        // View Track Only
    static const int STATE_LOCK  = 5;       // Do Not View/Edit
    int state;

    static const int CP_SIZE      = 10;     // General Control Point Size
    static const int SCENERY_SIZE = 6;      // Scenery Control Point Size

    // Control Points (These Change Depending On The Viewing State)
    QList<ControlPoint> *cps;

    explicit RoadPathScene(RoadPathWidget *parent = 0);
    bool isWidthHeightSceneMode();
    void createTooltip(QString text, QPoint p);
    void clearTooltip();
    int getCPSize();
    
signals:
    
public slots:

protected:
    void drawBackground(QPainter *painter, const QRectF &rect);
    void drawForeground(QPainter *painter, const QRectF &rect);

private:
    RoadPathWidget* parent;

    // Offset to use when drawing lines
    const static int LINE_OFFSET = 6;

    // Custom tooltip
    bool showTip;
    QString tipText;
    QPoint  tipPos;

    void drawControlPoints(QPainter* painter, const int size = CP_SIZE);
    void drawPathPoints(QPainter* painter);
    void drawPath(QPainter *painter, int highlightStart = -1, int highlightEnd = -1);
    void drawWidth(QPainter *painter, int highlightStart = -1, int highlightEnd = -1);
    void drawTooltip(QPainter* painter);  
};

#endif // ROADPATHSCENE_HPP
