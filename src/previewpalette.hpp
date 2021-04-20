/***************************************************************************
    PreviewPalette Widget

    Features:
    - Visualize a System 16 Palette
    - Allow editing of individual entries with QT Color Picker

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef PREVIEWPALETTE_HPP
#define PREVIEWPALETTE_HPP

#include <QWidget>
#include "stdint.hpp"

class PreviewPalette : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewPalette(QWidget *parent = 0);
    ~PreviewPalette();
    void setSelection(bool);
    int getSelectedIndex();
    QRgb* getPaletteData();
    void setPaletteData(QRgb*, int columns, int rows = 1);
    void paintEvent(QPaintEvent *event);
    
signals:
    void sendColor(int entry, uint16_t newColor);
    void isSelected(bool);
    void refreshPreview();
    
public slots:
    void setPalette(int);

protected:
    void mousePressEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);
    void enterEvent(QEvent *);

private:

    // Render Surface
    QImage* image;

    // QT Format Palette Data
    QRgb* rgb;

    // Rows And Columns of palette data
    int rows, columns;

    // Visual size of each block
    const static int SIZE = 8;

    int pal;

    // Is user allowed to select palette entries?
    bool selectionEnabled;

    // Store select palette entry
    QPoint selectedPoint;
    int selectedIndex;
    
};

#endif // PREVIEWPALETTE_HPP
