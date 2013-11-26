/***************************************************************************
    Render a single sprite chosen from the list.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QPainter>
#include <QDebug>

#include "previewwidget.hpp"

SpritePreviewWidget::SpritePreviewWidget(QWidget *parent) :
    QWidget(parent)
{
    image = NULL;
    setTransparency(false);
}

SpritePreviewWidget::~SpritePreviewWidget()
{
    if (image != NULL)
        delete image;
}

void SpritePreviewWidget::setTransparency(bool enabled)
{
    clearColor = enabled ? QColor(0, 0, 0, 0) : QColor(0, 0, 0, 255);
}

void SpritePreviewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (image != NULL)
        delete image;

    if (image == NULL)
        image = new QImage(width(), height(), QImage::Format_ARGB32);

}

void SpritePreviewWidget::clearSprite()
{
    if (image == NULL)
        return;
    image->fill(clearColor);
    update();
}

// Sprite Height is 0x7F + Height Helper
void SpritePreviewWidget::setSprite( const int bank,
                                     const int offset,
                                     const int width,
                                     const int height,
                                     const bool flip,
                                     const int pal)
{
    if (image == NULL)
        return;
    image->fill(clearColor);
    sprite.setSprite(bank, offset, width, height, flip, pal);

    // Paint sprite here.
    const int x = (image->width()  - width) / 2;
    const int y = (image->height() - height) / 2;

    QPainter painter(image);
    QRect target(x, y, sprite.image->width(), sprite.image->height());
    painter.drawImage(target, *sprite.image);
    update();
}

void SpritePreviewWidget::setSprite(const int bank,
                                    const int offset,
                                    const int width,
                                    const int height,
                                    const int pal)
{
    if (image == NULL)
        return;
    image->fill(clearColor);
    sprite.setSprite(bank, offset, width, height, pal);

    // Paint sprite here.
    const int x = (image->width()  - width) / 2;
    const int y = (image->height() - height) / 2;

    QPainter painter(image);
    QRect target(x, y, sprite.image->width(), sprite.image->height());
    painter.drawImage(target, *sprite.image);
    update();
}

void SpritePreviewWidget::paintEvent(QPaintEvent*)
{
    if (image == NULL)
        return;
    // Stretches image to target
    QRect target(0, 0, width(), height());
    QPainter painter(this);
    painter.drawImage(target, *image);
}
