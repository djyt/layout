/***************************************************************************
    RoadPathScene

    - Render a 2D Representation Of An OutRun Track.
    - Render Control Points.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QPainter>
#include <QGraphicsEllipseItem>

#include "roadpathwidget.hpp"
#include "leveldata.hpp"
#include "utils.hpp"
#include "roadpathscene.hpp"

RoadPathScene::RoadPathScene(RoadPathWidget *parent) :
    QGraphicsScene((QObject*) parent)
{
    this->parent = parent;
}

int RoadPathScene::getCPSize()
{
    return state == STATE_SCENERY ? SCENERY_SIZE : CP_SIZE;
}

bool RoadPathScene::isWidthHeightSceneMode()
{
    return state == STATE_WIDTH || state == STATE_HEIGHT || state == STATE_SCENERY;
}

void RoadPathScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    int highlightStart = -1;
    int highlightEnd   = -1;

    const int currentCP = parent->getCurrentCP();

    // Find selected section to highlight
    if (currentCP != -1)
    {
         // Highlight selected path section
         if (state == STATE_PATH)
         {
            highlightStart = 0;

            for (int i = 0; i <= currentCP; i++)
            {
                PathPoint rp = levelData->points->at(i);

                if (i == currentCP)
                {
                    highlightEnd = highlightStart + rp.length + 1;
                }
                else
                {
                    highlightStart += rp.length;
                }
            }
         }
         // Highlight selected height section
         else if (state == STATE_HEIGHT)
         {
            highlightStart = cps->at(currentCP).pos;
            highlightEnd   = highlightStart + parent->getHeightSegLength(currentCP) + 1;
         }


         if (highlightStart != -1 && highlightEnd != -1)
         {
             highlightStart = (highlightStart / LINE_OFFSET) * LINE_OFFSET;
             highlightEnd   = (highlightEnd  / LINE_OFFSET) * LINE_OFFSET;
         }
    }

    switch (state)
    {
        case STATE_VIEW:
            drawWidth(painter);
            drawPath(painter);
            break;

        case STATE_PATH:
            drawWidth(painter, highlightStart, highlightEnd);
            drawPath(painter, highlightStart, highlightEnd);
            drawPathPoints(painter);
            break;

        case STATE_WIDTH:
        case STATE_HEIGHT:
        case STATE_SCENERY:
            drawWidth(painter, highlightStart, highlightEnd);
            drawPath(painter, highlightStart, highlightEnd);
            drawControlPoints(painter, state == STATE_SCENERY ? SCENERY_SIZE : CP_SIZE);
            drawTooltip(painter);
            break;
    }
}

void RoadPathScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    const int roadPos = parent->getRoadPos();

    // Draw Road Position Marker
    if (roadPos != -1 && levelData->end_pos > 0)
    {
        WidthRender *wr = &levelData->width_render[roadPos];
        QPen pen;
        pen.setWidth(3);
        pen.setColor(QColor(0, 0, 255, 255));
        painter->setPen(pen);
        painter->drawLine(wr->road1_rhs, wr->road2_rhs);

        pen.setColor(QColor(0, 0, 255, 100));
        painter->setPen(pen);

        int pointTip = parent->getRoadPos() + 8;
        if (pointTip < levelData->end_pos)
        {
            painter->drawLine(wr->road1_rhs, levelData->posToPoint(pointTip));
            painter->drawLine(wr->road2_rhs, levelData->posToPoint(pointTip));
        }
    }
}

void RoadPathScene::drawPathPoints(QPainter* painter)
{
    QPen pen;
    pen.setWidth(2);

    // Draw the road path control points
    for (int i = 0; i < levelData->points->size(); i++)
    {
        if (i == parent->getCurrentCP())
        {
            pen.setColor(QColor(200, 100, 120, 200));
            painter->setBrush(QColor(250, 200, 210, 200));
        }
        else
        {
            pen.setColor(QColor(50, 100, 120, 200));
            painter->setBrush(QColor(200, 200, 210, 200));
        }

        painter->setPen(pen);

        PathPoint rp = levelData->points->at(i);
        painter->drawEllipse(QRectF(rp.p.x() - CP_SIZE,
                                    rp.p.y() - CP_SIZE,
                                    CP_SIZE*2, CP_SIZE*2));
    }
}

void RoadPathScene::drawControlPoints(QPainter* painter, const int size)
{
    QPen pen;
    pen.setWidth(2);

    // Draw the control points
    for (int i = 0; i < cps->size(); i++)
    {
        if (i == parent->getCurrentCP())
        {
            pen.setColor(QColor(200, 100, 120, 200));
            painter->setBrush(QColor(250, 200, 210, 200));
        }
        else
        {
            pen.setColor(QColor(50, 100, 120, 200));
            painter->setBrush(QColor(200, 200, 210, 200));
        }

        painter->setPen(pen);

        ControlPoint wp = cps->at(i);
        QPoint p = levelData->posToPoint(wp.pos);
        painter->drawEllipse(QRectF(p.x() - size,
                                    p.y() - size,
                                    size*2, size*2));
    }
}

void RoadPathScene::drawPath(QPainter* painter, int highlightStart, int highlightEnd)
{
    QPen pen;
    pen.setColor(QColor(0, 0, 0, 255));
    pen.setWidth(1);
    painter->setPen(pen);

    // Paint road path including highlighted section
    for (int i = 0; i < levelData->end_pos; i += LINE_OFFSET)
    {
        if (i == highlightStart)
        {
            pen.setColor(QColor(255, 64, 64, 255));
            painter->setPen(pen);
        }
        else if (i == highlightEnd)
        {
            pen.setColor(QColor(0, 0, 0, 255));
            painter->setPen(pen);
        }

        QPoint p1 = levelData->posToPoint(i);
        QPoint p2 = levelData->posToPoint(qMin(i+LINE_OFFSET, levelData->end_pos-1));
        painter->drawLine(p1, p2);
    }
}

void RoadPathScene::drawWidth(QPainter *painter, int highlightStart, int highlightEnd)
{
    const static int LINE_OFFSET = 6;

    bool colorToggle = false;

    // Paint Both Roads
    for (int i = 0; i < levelData->end_pos; i += LINE_OFFSET)
    {
        WidthRender* wr1 = &levelData->width_render[i];
        WidthRender* wr2 = &levelData->width_render[qMin(i+LINE_OFFSET, levelData->end_pos-1)];

        uint32_t color1 = levelData->pal->road[levelData->roadPal][LevelPalette::ROAD1];
        uint32_t color2 = levelData->pal->road[levelData->roadPal][LevelPalette::ROAD2];
        if (colorToggle)
        {
            color1 &= 0xFFFFF;
            color2 &= 0xFFFFF;
        }
        else
        {
            color1 >>= 16;
            color2 >>= 16;
        }

        colorToggle = !colorToggle;

        QColor convertedColor1(Utils::convertToQT(color1));
        QColor convertedColor2(Utils::convertToQT(color2));

        painter->setPen(convertedColor1);
        painter->setBrush(convertedColor1);

        const QPointF points1[4] =
        {
            wr1->road1_lhs, wr1->road1_rhs,
            wr2->road1_rhs, wr2->road1_lhs
        };
        painter->drawPolygon(points1, 4);

        painter->setPen(convertedColor1);
        painter->setBrush(convertedColor2);

        const QPointF points2[4] =
        {
            wr1->road2_lhs, wr1->road2_rhs,
            wr2->road2_rhs, wr2->road2_lhs
        };
        painter->drawPolygon(points2, 4);

        QPen pen;
        QColor color;
        if (i >= highlightStart && i <= highlightEnd)
        {
            color = QColor(128, 0, 0, 255);
        }
        else
        {
            color = QColor(0, 0, 0);
        }
        pen.setColor(color);
        pen.setWidth(2);
        painter->setPen(pen);
        painter->setBrush(color);

        // Draw separated roads when split
        if (wr1->split)
        {
            painter->drawLine(wr1->road1_lhs, wr2->road1_lhs);
            painter->drawLine(wr1->road2_lhs, wr2->road2_lhs);
        }

        painter->drawLine(wr1->road1_rhs, wr2->road1_rhs);
        painter->drawLine(wr1->road2_rhs, wr2->road2_rhs);
    }
}

// ------------------------------------------------------------------------------------------------
// Tooltips
// ------------------------------------------------------------------------------------------------

void RoadPathScene::clearTooltip()
{
    if (showTip)
    {
        showTip = false;
        tipText.clear();
        update();
    }
}

void RoadPathScene::createTooltip(QString text, QPoint p)
{
    if (text != tipText || p != tipPos)
    {
        showTip = true;
        tipText = text;
        tipPos  = p;
        update();
    }
}

void RoadPathScene::drawTooltip(QPainter* painter)
{
    if (!showTip) return;

    painter->setFont(QFont("Times", 8, QFont::Bold));

    const int w = painter->fontMetrics().boundingRect(tipText).width();
    const int h = painter->fontMetrics().height();
    const int x = tipPos.x() - (w >> 1);
    const int y = tipPos.y() - h - CP_SIZE;
    const int pad = 2;

    // Draw tooltip background
    painter->setPen(QColor(0,0,0));
    painter->fillRect(x, y, w + pad, h + pad, QBrush(QColor(192, 192, 255, 255)));

    // Draw tooltip text
    painter->setPen(QColor(0,0,0));
    painter->drawText(x + (pad/2), y + (pad/2), w, h, Qt::AlignTop, tipText);
}
