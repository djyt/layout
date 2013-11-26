/***************************************************************************
    Height Widget. Render Height Section Representation

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef HEIGHTWIDGET_HPP
#define HEIGHTWIDGET_HPP

#include <QWidget>
#include "leveldata.hpp"

class HeightWidget : public QWidget
{
    Q_OBJECT
public:

    explicit HeightWidget(QWidget *parent = 0);
    ~HeightWidget();
    int getSelectedPoint();

protected:
    void leaveEvent(QEvent* e);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    
signals:
    void pointChanged(int);
    
public slots:
    void createSegment();
    void setSegment(HeightSegment *seg);
    void selectPoint(int);

private:
    // Render Target
    QImage* image;

    // Height Segment Data
    HeightSegment* seg;

    // Points In Segment
    QList<QPoint> points;

    // Control Point Siz
    const static int POINT_SIZE = 10;

    // Has mouse been pressed
    bool mousePress;

    // Closest point to mouse cursor
    int nearestPoint;

    // Selected point
    int selectedPoint;

    // Gap Between Y Markers
    double yGap;

    void clear();
    void createHeightMap();
    void createHeightDelay();
    void createHorizonChange();
    void drawSegment(QPainter* painter);
    int getNearestPoint(QMouseEvent *e);    
};

#endif // HEIGHTWIDGET_HPP
