/***************************************************************************
    Layout About Dialog

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef ABOUT_HPP
#define ABOUT_HPP

#include <QDialog>
#include "../sprites/spriteformat.hpp"

class RomLoader;

namespace Ui {
class About;
}

class About : public QDialog
{
    Q_OBJECT
    
public:
    explicit About(QWidget *parent = 0, RomLoader *rom0 = NULL);
    ~About();
    
private slots:
    void closeAbout();
    void advanceFrame();

private:
    Ui::About *ui;

    // Animation Sprites
    QList <SpriteFormat> sprites;

    // Animation Palettes
    QList <int>  pals;

    // Current Animation Frame
    int currentFrame;

    // Function Prototypes
    void setupFrames(RomLoader* rom0);
};

#endif // ABOUT_HPP
