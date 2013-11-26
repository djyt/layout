/***************************************************************************
    Height Widget. Render Height Section Representation.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QtWidgets>

#include "heightwidget.hpp"
#include "../utils.hpp"

HeightWidget::HeightWidget(QWidget *parent) :
    QWidget(parent)
{
    seg           = NULL;
    nearestPoint  = -1;
    selectedPoint = -1;
    yGap          = 1;

    image = new QImage(1024, 1024, QImage::Format_ARGB32);
    image->fill(QColor(255, 255, 255));

    setMouseTracking(true); // Receive mouse events even when button not pressed
}

HeightWidget::~HeightWidget()
{
    delete image;
}

void HeightWidget::setSegment(HeightSegment* seg)
{    
    this->seg = seg;
    selectPoint(-1);
    createSegment();
}

void HeightWidget::selectPoint(int index)
{
    selectedPoint = index;
    emit pointChanged(selectedPoint);
}

int HeightWidget::getSelectedPoint()
{
    return selectedPoint;
}

// ------------------------------------------------------------------------------------------------
// Segment Construction. Create list of points for rendering.
// ------------------------------------------------------------------------------------------------

// Create the segment data required for the rendering code
void HeightWidget::createSegment()
{
    yGap = 1;

    points.clear();

    if (seg != NULL)
    {
        switch (seg->type)
        {
        case 0:
        case 3:
            createHeightMap();
            break;

        case 1:
        case 2:
            createHeightDelay();
            break;

        case 4:
            createHorizonChange();
            break;
        }
    }

    update();
}

void HeightWidget::createHeightMap()
{
    int x = POINT_SIZE / 2;
    int y = 0;

    // Calculate x spacing to use between points
    const int NO_POINTS = seg->data.size() - 1;
    int xGap = 32;

    if (NO_POINTS * xGap > width() - POINT_SIZE)
    {
        xGap = ((width() - POINT_SIZE) / NO_POINTS);
    }

    // calculate y spacing to use between points
    int yMin   = INT_MAX;
    int yMax   = INT_MIN;
    int ySpace = y;
    for (int i = 0; i < seg->data.size(); i++)
    {
        ySpace += -seg->data.at(i);
        if (ySpace < yMin)
            yMin = ySpace;
        if (ySpace > yMax)
            yMax = ySpace;
    }

    int diff = std::max(std::abs(yMax), std::abs(yMin));

    yGap = (double) diff / ((height() / 2) - POINT_SIZE);
    if (yGap < 1)
        yGap = 1;

    y = height() / 2;

    // Iterate Height Entries
    for (int i = 0; i < seg->data.size(); i++)
    {
        y += -(seg->data.at(i) / yGap);
        points.push_back(QPoint(x, y));
        x += xGap;
    }
}

void HeightWidget::createHeightDelay()
{
    const int16_t y1 = -seg->data.at(0);
    const int16_t y2 = -seg->data.at(1);
    const int y      = height() / 2;
    points.push_back(QPoint(0 + POINT_SIZE,       y + (y1 / 16)));
    points.push_back(QPoint(width() - POINT_SIZE, y + (y2 / 16)));
}

void HeightWidget::createHorizonChange()
{
    const int16_t y1 = -seg->value1;
    const int16_t y2 = -seg->value1;
    const int y      = height() / 2;
    points.push_back(QPoint(0 + POINT_SIZE,       y + (y1 / 16)));
    points.push_back(QPoint(width() - POINT_SIZE, y + (y2 / 16)));
}

// ------------------------------------------------------------------------------------------------
// Rendering
// ------------------------------------------------------------------------------------------------

void HeightWidget::clear()
{
    image->fill(qRgb(255, 255, 255));
    update();
}

void HeightWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(image);
    drawSegment(&painter);

    QPainter blit(this);
    QRect dirtyRect = event->rect();
    blit.drawImage(dirtyRect, *image, dirtyRect);
}

void HeightWidget::resizeEvent(QResizeEvent *event)
{
    createSegment();
    QWidget::resizeEvent(event);
}


// ------------------------------------------------------------------------------------------------
// Draw Height Segment from previously generated set of points.
//
// Colour Reference From: http://www.wallpaper.com/images/98_graph_a_clarke_gf_2.jpg
// ------------------------------------------------------------------------------------------------
void HeightWidget::drawSegment(QPainter *painter)
{
    const int HH = height() / 2;

    // Draw Background
    painter->fillRect(0, 0,  width(), HH, QColor(0x01, 0x6F, 0xAC, 0xFF));
    painter->fillRect(0, HH, width(), HH, QColor(0x01, 0x54, 0x7E, 0xFF));

    // Draw Y-Scale Divisions
    QPen pen;
    pen.setWidth(1);
    pen.setColor(QColor(0x34, 0x8C, 0xBC, 255));
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);

    const double DIVISION = (64.0 / yGap);

    for (double y = HH; y < height(); y += DIVISION)
        painter->drawLine(0, y, width(), y);
    for (double y = HH - DIVISION; y >= 0; y -= DIVISION)
        painter->drawLine(0, y, width(), y);

    // Draw lines between points
    pen.setWidth(3);
    pen.setStyle(Qt::SolidLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setColor(QColor(0x80, 0xb1,0xd2, 255));
    painter->setPen(pen);

    for (int i = 0; i < points.size() - 1; i++)
        painter->drawLine(points.at(i), points.at(i+1));

    // Draw individual points
    pen.setWidth(0);
    painter->setPen(pen);

    for (int i = 0; i < points.size(); i++)
    {
        int size = POINT_SIZE / 2;
        if (i == selectedPoint)
            size += 1;
        else if (i == nearestPoint)
            size += 2;
        else
            size--;

        const int ALPHA = i == nearestPoint || i == selectedPoint ? 255 : 140;
        painter->setBrush(QColor(0xfc, 0xfe, 0xfd, ALPHA));
        painter->drawEllipse(points.at(i), size, size);
    }
}

// Get Nearest Control Point to Mouse Click
int HeightWidget::getNearestPoint(QMouseEvent *e)
{
    qreal distance = -1;
    int point = -1;

    // Clicked Existing Point?
    for (int i = 0; i < points.size(); i++)
    {
        qreal d = QLineF(e->pos(), points.at(i)).length();
        if ((distance < 0 && d < POINT_SIZE * 2) || d < distance)
        {
            distance = d;
            point = i;
        }
    }
    return point;
}

// ------------------------------------------------------------------------------------------------
// Focus Lost
// ------------------------------------------------------------------------------------------------

void HeightWidget::leaveEvent(QEvent*)
{
    nearestPoint = -1;
    update();
}

// ------------------------------------------------------------------------------------------------
// Mouse Control
// ------------------------------------------------------------------------------------------------

void HeightWidget::mousePressEvent(QMouseEvent *e)
{
    Qt::MouseButton button = e->button();

    if (button == Qt::LeftButton)
    {
        mousePress = true;
        selectPoint(nearestPoint);
    }

    update();
}

void HeightWidget::mouseReleaseEvent(QMouseEvent *)
{
    mousePress = false;
}

void HeightWidget::mouseMoveEvent(QMouseEvent *e)
{
    // Adjust mouse cursor when over a control point
    int newPoint = getNearestPoint(e);
    if (newPoint != nearestPoint)
    {
        nearestPoint = newPoint;
        update();
    }

    setCursor(nearestPoint != -1 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}
