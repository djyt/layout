/***************************************************************************
    Render a single sprite chosen from the list.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/


#ifndef SPRITERENDER_HPP
#define SPRITERENDER_HPP

#include <QWidget>

#include "sprite.hpp"

class SpritePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpritePreviewWidget(QWidget *parent = 0);
    ~SpritePreviewWidget();

    void setTransparency(bool enabled);
    void clearSprite();
    void setSprite(const int bank, const int offset, const int width, const int height, const bool flip, const int pal = -1);
    void setSprite(const int bank, const int offset, const int width, const int height, const int pal);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

signals:
    
public slots:
    void recreate();

private:
    int bank;
    int offset;
    int w;
    int h;
    bool flip;
    int pal;

    Sprite sprite;
    QImage* image;
    QColor clearColor;

    void createImage();
};

#endif // SPRITERENDER_HPP
