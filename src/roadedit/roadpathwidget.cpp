/***************************************************************************
    RoadPathWidget

    - Handles the 2D Representation of an OutRun Track
    - Handles Editing Control Points And User Interaction

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QtWidgets>
#include "globals.hpp"
#include "roadpathwidget.hpp"
#include "leveldata.hpp"

RoadPathWidget::RoadPathWidget(QWidget *parent)
    : QGraphicsView(parent)
{ 
    setMouseTracking(true); // Receive mouse events even when button not pressed

    scene = new RoadPathScene(this);
    this->setOptimizationFlag(QGraphicsView::DontSavePainterState);
    this->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    //this->setRenderHint(QPainter::Antialiasing);

    prevAngle = 0;
    zoom      = 100;
}

RoadPathWidget::~RoadPathWidget()
{
    delete scene;
}

void RoadPathWidget::init()
{
    scene->state       = RoadPathScene::STATE_PATH;
    activePathPoint    = -1;
    activeWidthPoint   = -1;
    activeHeightPoint  = -1;
    activeSceneryPoint = -1;
    activePoint        = &activePathPoint;
    centreViewPoint    = true;
    mousePress         = false;
    mousePos           = -1;
    roadPos            = 0;

    this->setScene(scene);
    this->setAlignment(Qt::AlignCenter);

    roadLengthChanged();
    scene->setSceneRect(levelData->getPathRect());
    this->centerOn(levelData->posToPoint(0));
    this->setDragMode(QGraphicsView::ScrollHandDrag);
    this->show();
}

void RoadPathWidget::setHeightSection(HeightSection* section)
{
    heightSection = section;
}

void RoadPathWidget::setSpriteSection(SpriteSection* section)
{
    spriteSection = section;
}

// ------------------------------------------------------------------------------------------------
// RoadPathScene::STATEs
// ------------------------------------------------------------------------------------------------

// Prevents controls from updating and causing problems when changing RoadPathScene::STATEs.
void RoadPathWidget::lock()
{
    scene->state = RoadPathScene::STATE_LOCK;
}

void RoadPathWidget::setStatePath()
{
    scene->state = RoadPathScene::STATE_PATH;
    activePoint = &activePathPoint;
    updateControls();
    updateScene();
}

void RoadPathWidget::setStateWidth()
{
    scene->state = RoadPathScene::STATE_WIDTH;
    activePoint = &activeWidthPoint;
    scene->cps = &levelData->widthP;
    levelData->updateWidthData();
    updateControls();
    updateScene();
}

void RoadPathWidget::setStateHeight()
{
    scene->state = RoadPathScene::STATE_HEIGHT;
    activePoint = &activeHeightPoint;
    scene->cps = &levelData->heightP;
    updateControls();
    updateScene();
}

void RoadPathWidget::setStateScenery()
{
    scene->state = RoadPathScene::STATE_SCENERY;
    activePoint = &activeSceneryPoint;
    scene->cps = &levelData->spriteP;
    updateControls();
    updateScene();
}

void RoadPathWidget::setStateView()
{
    scene->state = RoadPathScene::STATE_VIEW;
    updateScene();
}


void RoadPathWidget::setZoom(int percentZoom)
{
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    qreal targetScale = (qreal)percentZoom / 100.0;
    qreal scaleFactor = targetScale / transform().m11();
    scale(scaleFactor, scaleFactor);
}

// ------------------------------------------------------------------------------------------------
// Slots: Control Inputs
// ------------------------------------------------------------------------------------------------

// Select view of road
void RoadPathWidget::setView(int view)
{
    switch (view)
    {
        case 0:  setStatePath();    break;
        case 1:  setStateWidth();   break;
        case 2:  setStateHeight();  break;
        case 3:  setStateScenery(); break;
        default: setStateView();    break;
    }
}

void RoadPathWidget::setRoadPos(int value)
{
    if (value < levelData->end_pos)
        roadPos = value;

    if (centreViewPoint)
    {
        this->centerOn(levelData->posToPoint(roadPos));

        // Rotate To Angle
        /*if (roadPos != -1 && roaddata->end_pos > 0)
        {
            WidthRender *wr = &roaddata->width_render[roadPos];
            QLineF line (wr->road2_rhs, wr->road1_rhs);
            qreal newAngle = line.angle();

            if (std::abs(newAngle - prevAngle) > 5)
            {
                this->rotate(-prevAngle);
                this->rotate(line.angle());
                prevAngle = line.angle();
            }
        }*/
    }
    else
    {
        centreViewPoint = true;
    }
    updateScene();
}

// Set New Segment Length Value
void RoadPathWidget::setLength(int value)
{
    if (*activePoint != -1)
    {
        PathPoint* rp = &(*levelData->points)[*activePoint];

        // Update Length
        if (value != rp->length)
        {
            levelData->changePathLength(*activePoint, value);
            levelData->updatePathData();
            scene->setSceneRect(levelData->getPathRect());
            roadLengthChanged();
            emit refreshPreview(roadPos);
            updateScene();
        }
    }
}

// Set New Segment Width Value
void RoadPathWidget::setWidth(int value)
{
    if (*activePoint != -1)
    {
        ControlPoint cp = scene->cps->at(*activePoint);
        if (value != cp.value1)
        {
            cp.value1 = value;
            scene->cps->replace(*activePoint, cp);
            levelData->updateWidthData();
            emit refreshPreview(roadPos);
            updateScene();
        }
    }
}

// Set New Segment Point Value (Used for Height & Scenery)
void RoadPathWidget::setPoint(int value)
{
    if (*activePoint != -1)
    {
        ControlPoint cp = scene->cps->at(*activePoint);
        if (value != cp.value1)
        {
            cp.value1 = value;
            scene->cps->replace(*activePoint, cp);
            emit refreshPreview();
        }
    }
}

void RoadPathWidget::setAngle(int value)
{
    if (*activePoint != -1)
    {
        PathPoint rp = levelData->points->at(*activePoint);

        // Update Angle
        if (value != rp.angle_inc)
        {
            rp.angle_inc = value;
            levelData->points->replace(*activePoint, rp);
            levelData->updatePathData();
            scene->setSceneRect(levelData->getPathRect());
            emit refreshPreview(roadPos);
            updateScene();
        }
    }
}

// Set Width Change Speed
void RoadPathWidget::setWidthChange(int value)
{
    if (*activePoint != -1)
    {
        ControlPoint wp = scene->cps->at(*activePoint);
        if (value != wp.value2)
        {
            wp.value2 = value;
            scene->cps->replace(*activePoint, wp);
            levelData->updateWidthData();
            emit refreshPreview(roadPos);
            updateScene();
        }
    }
}

void RoadPathWidget::insertPointAfter()
{
    levelData->insertPathPoint((*activePoint) + 1);
    (*activePoint)++;
    levelData->updatePathData();
    scene->setSceneRect(levelData->getPathRect());

    PathPoint rp = levelData->points->at(*activePoint);
    this->centerOn(levelData->posToPoint(rp.pos));

    roadLengthChanged();

    emit refreshPreview(roadPos);
    updateScene();
    updateControls();
}

void RoadPathWidget::insertPointBefore()
{
    if (*activePoint < 0)
        *activePoint = 0;
    levelData->insertPathPoint(*activePoint);
    levelData->updatePathData();
    scene->setSceneRect(levelData->getPathRect());

    PathPoint rp = levelData->points->at(*activePoint);
    this->centerOn(levelData->posToPoint(rp.pos));

    roadLengthChanged();

    emit refreshPreview(roadPos);
    updateScene();
    updateControls();
}

void RoadPathWidget::manipulatePoint(int point, int pos)
{
    if (scene->state == RoadPathScene::STATE_PATH)
    {
        // Delete Existing Point
        if (point != -1)
        {
            levelData->deletePathPoint(point);
            *activePoint = -1;
            levelData->updatePathData();
            scene->setSceneRect(levelData->getPathRect());
            roadLengthChanged();
            emit refreshPreview(roadPos);
        }
        // Split Existing Point
        else
        {
            *activePoint = levelData->splitPathPoints(pos);
        }
        updateScene();
        updateControls();
    }
    else if (scene->isWidthHeightSceneMode())
    {
        if (point != -1)
        {
            scene->cps->removeAt(point);
            *activePoint = -1;

            if (scene->state == RoadPathScene::STATE_WIDTH)
                levelData->updateWidthData();

            emit refreshPreview(roadPos);
            updateScene();
            updateControls();
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Signals: Control Outputs
// ------------------------------------------------------------------------------------------------

void RoadPathWidget::updateControls()
{
    if (scene->state == RoadPathScene::STATE_PATH)
    {
        emit toggleLaneControls(false);

        // No active point selected
        if (*activePoint == -1)
        {
            emit enableControls(false);
        }
        else
        {
            emit enableControls(true);
            PathPoint rp = levelData->points->at(*activePoint);
            updateStatus(rp.pos);
            emit changeLength(rp.length);
            emit changeAngle(rp.angle_inc);
        }
    }
    else if (scene->state == RoadPathScene::STATE_WIDTH)
    {
        if (*activePoint == -1)
        {
            emit enableControls(false);
            emit toggleLaneControls(false);
        }
        else
        {
            emit enableControls(true);
            emit toggleLaneControls(true);
            ControlPoint cp = scene->cps->at(*activePoint);
            updateStatus(cp.pos);
            emit changeWidth(cp.value1);
            emit changeWidthSpeed(cp.value2);
        }
    }
    else if (scene->state == RoadPathScene::STATE_HEIGHT)
    {
        emit toggleLaneControls(false);

        if (*activePoint == -1)
        {
            emit enableControls(false);
        }
        else
        {
            emit enableControls(true);
            ControlPoint cp = scene->cps->at(*activePoint);
            updateStatus(cp.pos);
            emit selectHeightSection(cp.value1);
        }
    }
    else if (scene->state == RoadPathScene::STATE_SCENERY)
    {
        emit toggleLaneControls(false);

        if (*activePoint == -1)
        {
            emit enableControls(false);
        }
        else
        {
            emit enableControls(true);
            emit selectScenerySection(levelData->spriteP.at(*activePoint).value2);
            ControlPoint cp = scene->cps->at(*activePoint);
            emit changePatternLength(cp.value1);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Other Helper Functions
// ------------------------------------------------------------------------------------------------

int RoadPathWidget::getCurrentCP()
{
    return *activePoint;
}

int RoadPathWidget::getRoadPos()
{
    return roadPos;
}

int RoadPathWidget::getNearestRP(QPointF mousePos)
{
    qreal distance = -1;
    int point = -1;

    for (int i = 0; i < levelData->points->size(); i++)
    {
        PathPoint rp = levelData->points->at(i);
        qreal d = QLineF(mousePos, rp.p).length();
        if ((distance < 0 && d < RoadPathScene::CP_SIZE * 2) || d < distance)
        {
            distance = d;
            point = i;
        }
    }

    return point;
}

// Get Nearest Control Point to Mouse Click
int RoadPathWidget::getNearestCP(QPointF mousePos)
{
    qreal distance = -1;
    int point = -1;

    // Clicked Existing Point?
    for (int i = 0; i < scene->cps->size(); i++)
    {
        ControlPoint wp = scene->cps->at(i);
        QPoint p = levelData->posToPoint(wp.pos);
        qreal d = QLineF(mousePos, p).length();
        if ((distance < 0 && d < scene->getCPSize() * 2) || d < distance)
        {
            distance = d;
            point = i;
        }
    }
    return point;
}

int RoadPathWidget::getPosInLevel(QPointF pos)
{
    int insert_pos = -1;
    qreal distance = -1;

    // More intelligent detection of position when tracks are overlapped
    // Search using the current active point as a basis
    if (scene->isWidthHeightSceneMode())
    {
        if (*activePoint != -1)
        {
            int activePos = scene->cps->at(*activePoint).pos;

            int startPos = qMax(0, activePos - 20);
            int endPos   = qMin(levelData->end_pos, activePos + 20);

            for (int i = startPos; i < endPos; i++)
            {
                QPoint p = levelData->posToPoint(i);
                qreal d = QLineF(pos, p).length();
                if ((distance < 0 && d < RoadPathScene::CP_SIZE * 2) || d < distance)
                {
                    distance   = d;
                    insert_pos = i;
                }
            }

            if (insert_pos != -1)
                return insert_pos;
        }
    }


    for (int i = 0; i < levelData->end_pos; i++)
    {
        QPoint p = levelData->posToPoint(i);

        qreal d = QLineF(pos, p).length();
        if ((distance < 0 && d < RoadPathScene::CP_SIZE * 2) || d < distance)
        {
            distance   = d;
            insert_pos = i;
        }
    }
    return insert_pos;
}

void RoadPathWidget::updateStatus(int insert_pos)
{
    if (insert_pos != -1)
    {
        QString pos = QString("Level Position: %1").arg(insert_pos);
        emit setStatusBar(pos);
    }
}

void RoadPathWidget::roadLengthChanged()
{
    // Toggle insert button based on whether we have space for more road
    emit enableInsert(levelData->end_pos < levelData->length);

    int percentage = ((float) levelData->end_pos / (float) levelData->length) * 100.0f;
    emit setPercentage(percentage);
    emit setEndPos(levelData->end_pos);
}

// Get length of a particular height section
int RoadPathWidget::getHeightSegLength(int index)
{
    // Length of each height segment
    const static double POS_LENGTH = 10 * 12;

    ControlPoint cp   = levelData->heightP.at(index);
    QList<HeightSegment>* heightMaps = heightSection->getSectionList();
    HeightSegment seg = heightMaps->at(cp.value1);

    switch (seg.type)
    {
    // ----------------------------------------------------------------------------------------
    // HEIGHTMAP TYPE 0: STANDARD ELEVATION
    // ----------------------------------------------------------------------------------------
    case 0:
    {
        double heightPos = 0;

        // Iterate height map to calculate which segment we've reached
        // segmentIndex will contain the correct segment within the heightmap
        for (int i = 0; i < seg.data.size(); i++)
        {
            int16_t v = seg.data.at(i);

            double segLength = 0;

            if (v == 0)     segLength = POS_LENGTH / seg.step;
            else if (v < 0) segLength = POS_LENGTH / (seg.step * seg.value1); // could be wrong way around
            else if (v > 0) segLength = POS_LENGTH / (seg.step * seg.value2); // could be wrong way around

            double increment = HEIGHT_LENGTH / segLength;
            heightPos += increment;
        }

        return (int) heightPos;

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
        const double segLength3 = (segLength * 2) + delayLength;

        return (int) segLength3;
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

        return (int) segLength3;
    }
    break;
    // ----------------------------------------------------------------------------------------
    // CHANGE HORIZON
    // ----------------------------------------------------------------------------------------
    case 4:
    {
        const double segLength = HEIGHT_LENGTH / (POS_LENGTH / seg.step);
        return (int) segLength;
    }
    break;

    } // end switch

    return 0;
}


// ------------------------------------------------------------------------------------------------
// Rendering
// ------------------------------------------------------------------------------------------------

void RoadPathWidget::updateScene()
{
    scene->clear();

    if (levelData->end_pos < 1)
        return;

    scene->update();
}

// ------------------------------------------------------------------------------------------------
// Mouse Control
// ------------------------------------------------------------------------------------------------

void RoadPathWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (scene->state == RoadPathScene::STATE_VIEW || scene->state == RoadPathScene::STATE_LOCK)
        return;

    Qt::MouseButton button = e->button();

    if (button == Qt::LeftButton)
    {
        QPointF mp = mapToScene(e->pos());
        int point = (scene->state == RoadPathScene::STATE_PATH) ? getNearestRP(mp) : getNearestCP(mp);
        manipulatePoint(point, getPosInLevel(mp));
    }
}

void RoadPathWidget::mousePressEvent(QMouseEvent *e)
{
    QGraphicsView::mousePressEvent(e);

    if (scene->state == RoadPathScene::STATE_VIEW || scene->state == RoadPathScene::STATE_LOCK)
        return;

    Qt::MouseButton button = e->button();
    pointDragged = false;

    if (button == Qt::LeftButton)
    {
        mousePress = true;
        QPointF mp = mapToScene(e->pos());

        if (scene->state == RoadPathScene::STATE_PATH)
        {
            int point = getNearestRP(mp);

            // Point Selected
            if (point != -1)
            {
                *activePoint = point;
                viewport()->setCursor(Qt::PointingHandCursor);
            }

            updateScene();
        }
        else if (scene->isWidthHeightSceneMode())
        {
            if (mousePos != -1)
                viewport()->setCursor(Qt::PointingHandCursor);

            // Determine whether we have clicked to insert a new point, or have clicked existing point
            int point = getNearestCP(mp);

            // Point Not Found: Insert New Point
            if (point == -1)
            {
                int insertPos = getPosInLevel(mp);
                if (insertPos >= 0)
                {
                    switch (scene->state)
                    {
                    case RoadPathScene::STATE_WIDTH:
                        *activePoint = levelData->insertWidthPoint(insertPos);
                        updateScene();
                        break;

                    case RoadPathScene::STATE_HEIGHT:
                        *activePoint = levelData->insertHeightPoint(insertPos);
                        emit refreshPreview();
                        updateScene();
                        break;

                    case RoadPathScene::STATE_SCENERY:
                        if (spriteSection->getSectionList()->size() > 0)
                        {
                            *activePoint = levelData->insertSceneryPoint(insertPos);
                            emit refreshPreview();
                            updateScene();
                        }
                        break;
                    }
                }
                else
                    updateScene();
            }
            else
            {
                *activePoint = point;
                updateScene();
            }
        }
        updateControls();
    }
    else if (button == Qt::RightButton)
    {
        // We're at a genuine position in the level
        if (mousePos != -1)
        {
            centreViewPoint = false;
            roadPos = mousePos;
            emit refreshPreview(roadPos);
        }
    }
}

void RoadPathWidget::mouseReleaseEvent(QMouseEvent *e)
{
    QGraphicsView::mouseReleaseEvent(e);

    mousePress = false;

    if (scene->state == RoadPathScene::STATE_VIEW || scene->state == RoadPathScene::STATE_LOCK)
        return;

    if (pointDragged)
    {
        pointDragged = false;
        if (roadPos != -1)
            emit refreshPreview(roadPos);

        this->setDragMode(QGraphicsView::ScrollHandDrag);
    }

    if (mousePos != -1)
        viewport()->setCursor(Qt::PointingHandCursor);
}


void RoadPathWidget::mouseMoveEvent(QMouseEvent *e)
{
    QGraphicsView::mouseMoveEvent(e);

    // View Mode: Do Nothing
    if (scene->state == RoadPathScene::STATE_VIEW || scene->state == RoadPathScene::STATE_LOCK)
        return;

    // Path Mode: Set Cursor Appropriate Cursor
    if (scene->state == RoadPathScene::STATE_PATH)
    {
        QPointF mp = mapToScene(e->pos());
        const int roadPoint = getNearestRP(mp);
        if (roadPoint != -1)
            viewport()->setCursor(Qt::PointingHandCursor);
        else if (viewport()->cursor().shape() == Qt::PointingHandCursor)
            viewport()->setCursor(Qt::OpenHandCursor);

    }

    mousePos = getPosInLevel(mapToScene(e->pos()));

    // Mouse Position Is Over Road.
    if (mousePos != -1)
    {
        updateStatus(mousePos);

        if (scene->isWidthHeightSceneMode())
            viewport()->setCursor(Qt::PointingHandCursor);
    }
    // Mouse Position Is Outside Road
    else
    {
        scene->clearTooltip();
        if (viewport()->cursor().shape() == Qt::PointingHandCursor)
            viewport()->setCursor(Qt::OpenHandCursor);

        emit setStatusBar("Ready");
        return;
    }

    if (scene->isWidthHeightSceneMode())
    {
        // Create Tooltips for width, height & scenery items
        if (!mousePress)
        {
            int point = getNearestCP(mapToScene(e->pos()));
            if (point != -1)
            {
                QPoint p = levelData->posToPoint(scene->cps->at(point).pos);

                if (scene->state == RoadPathScene::STATE_WIDTH)
                {
                    scene->createTooltip(QString("Width: %1").arg(scene->cps->at(point).value1), p);
                }
                if (scene->state == RoadPathScene::STATE_SCENERY)
                {
                    int sceneryIndex = scene->cps->at(point).value2;
                    scene->createTooltip(spriteSection->getSectionName(sceneryIndex), p);
                }
                else if (scene->state == RoadPathScene::STATE_HEIGHT)
                {
                    int heightIndex = scene->cps->at(point).value1;
                    scene->createTooltip(heightSection->getSectionName(heightIndex), p);
                }
            }
            else
            {
                scene->clearTooltip();
            }
        }
        // Insert New Width, height or scenery point
        else if (mousePress && *activePoint != -1)
        {
            pointDragged = true;
            this->setDragMode(QGraphicsView::NoDrag);

            ControlPoint wp = scene->cps->at(*activePoint);
            wp.pos = mousePos;

            // Check whether to erase the upcoming point
            // if we've dragged past it.
            if (*activePoint < scene->cps->size() - 1)
            {
                if (scene->cps->at(*activePoint+1).pos < mousePos)
                    scene->cps->removeAt(*activePoint+1);
            }

            // Check whether to erase the previous point
            // if we've dragged past it
            if (*activePoint > 0)
            {
               if (scene->cps->at(*activePoint-1).pos > mousePos)
                   scene->cps->removeAt(--(*activePoint));
            }

            scene->cps->replace(*activePoint, wp);

            if (scene->state == RoadPathScene::STATE_WIDTH)
                levelData->updateWidthData();
            updateScene();
        }
    }
}

// Scroll the road
void RoadPathWidget::wheelEvent(QWheelEvent *event)
{
    // Zoom when CTRL is held down
    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        QPoint numDegrees = event->angleDelta();

        if (!numDegrees.isNull())
        {
            int change = numDegrees.y();
            if (std::abs(numDegrees.y()) >= 120)
                change /= 120;
            else
                change = numDegrees.y() > 0 ? 1 : -1;

            zoom += (change * 5);

            if (zoom > ZOOM_MAX)
                zoom = ZOOM_MAX;
            else if (zoom < ZOOM_MIN)
                zoom = ZOOM_MIN;

            setZoom(zoom);

            emit changeZoom(zoom);
        }

        event->accept();
    }
    // Otherwise scroll around the view
    else
        QGraphicsView::wheelEvent(event);
}
