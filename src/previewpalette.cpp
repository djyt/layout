/***************************************************************************
    PreviewPalette Widget

    Features:
    - Visualize a System 16 Palette
    - Allow editing of individual entries with QT Color Picker

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>

#include "utils.hpp"
#include "previewpalette.hpp"

PreviewPalette::PreviewPalette(QWidget *parent) :
    QWidget(parent)
{
    rgb   = NULL;
    image = new QImage(width(), height(), QImage::Format_RGB32);
    pal   = -1;
    rows  = columns = 0;

    setSelection(false);
}

PreviewPalette::~PreviewPalette()
{
    if (image != NULL)
        delete image;
}

void PreviewPalette::setSelection(bool enabled)
{
    selectionEnabled = enabled;

    if (!selectionEnabled)
    {
        selectedPoint.setX(-1);
        selectedPoint.setY(-1);
        selectedIndex = -1;
    }

    emit isSelected(false);
}

int PreviewPalette::getSelectedIndex()
{
    return selectedIndex;
}

QRgb* PreviewPalette::getPaletteData()
{
    return rgb;
}

void PreviewPalette::setPaletteData(QRgb* rgb, int columns, int rows)
{
    if (this->rgb)
        delete this->rgb;

    this->rgb = rgb;
    this->columns = columns;
    this->rows    = rows;

    const int w = columns * SIZE;
    const int h = rows    * SIZE;
    delete image;
    image = new QImage(w, h, QImage::Format_RGB32);
}

void PreviewPalette::setPalette(int pal)
{
    this->pal = pal != -1 ? pal * 16 : -1;
    update();
}

void PreviewPalette::paintEvent(QPaintEvent*)
{
    if (image == NULL)
        return;

    image->fill(0);

    if (rgb != NULL && pal != -1)
    {
        QPainter imgPainter(image);

        int p = pal;

        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < columns; x++)
            {
                imgPainter.fillRect(x * SIZE, y * SIZE, SIZE, SIZE, QColor(rgb[p++]));

                if (x == selectedPoint.x() && y == selectedPoint.y())
                {
                    QPen pen(QColor(255,255,255,128));
                    pen.setWidth(1);
                    imgPainter.setPen(pen);
                    imgPainter.drawRect((x * SIZE) + 1, (y * SIZE) + 1, SIZE - 3, SIZE - 3);
                    pen.setColor(QColor(0, 0, 0, 128));
                    imgPainter.setPen(pen);
                    imgPainter.drawRect(x * SIZE, y * SIZE, SIZE - 1, SIZE - 1);
                }
            }
        }
    }

    // Stretches image to target
    QRect target(0, 0, width(), height());
    QPainter painter(this);
    painter.drawImage(target, *image);
}

void PreviewPalette::mousePressEvent(QMouseEvent *e)
{
    if (rgb != NULL && pal != -1)
    {
        // Establish which palette entry we're in
        const int col = e->position().x() / (width()  / columns);
        const int row = e->position().y() / (height() / rows);
        const int entry = col + (row * columns);
        const int srcColor = pal + entry;

        const Qt::MouseButton button = e->button();
        if (button == Qt::LeftButton)
        {
            // Launch a colour picker for the palette entry
            QColor color = QColorDialog::getColor(QColor(rgb[srcColor]), this);

            // Valid colour selected, output a signal for the new colour
            if (color.isValid())
            {
                uint16_t s16Color = Utils::convertToS16(color);
                rgb[srcColor] = color.rgb();
                emit sendColor(entry, s16Color);
                emit refreshPreview();
            }
        }
        else if (button == Qt::RightButton && selectionEnabled)
        {
            if (selectedPoint.x() != col || selectedPoint.y() != row)
            {
                selectedPoint.setX(col);
                selectedPoint.setY(row);
                selectedIndex = col + (row * columns);
                emit isSelected(true);
            }
            else
            {
                selectedPoint.setX(-1);
                selectedPoint.setY(-1);
                selectedIndex = -1;
                emit isSelected(false);
            }
            update();
        }
    }
}

void PreviewPalette::enterEvent(QEvent*)
{
    setCursor(Qt::PointingHandCursor);
}

void PreviewPalette::leaveEvent(QEvent*)
{
    setCursor(Qt::ArrowCursor);
}
