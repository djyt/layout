/***************************************************************************
    Layout About Dialog

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QTimer>
#include "../globals.hpp"
#include "../import/romloader.hpp"
#include "ui_about.h"
#include "about.hpp"

About::About(QWidget *parent, RomLoader* rom0) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::About)
{
    ui->setupUi(this);
    setFixedSize(size());
    ui->sprite->setTransparency(true);

    connect(ui->buttonOK, SIGNAL(clicked()), this, SLOT(closeAbout()));

    if (rom0->rom != NULL)
    {
        setupFrames(rom0);
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(advanceFrame()));
        timer->start(150);
    }
    else
        currentFrame = -1;

   advanceFrame();
}

About::~About()
{
    delete ui;
}

void About::closeAbout()
{
    this->close();
}

// ------------------------------------------------------------------------------------------------
// Simple Animation Player
//
// Requires ROMs to be loaded.
// ------------------------------------------------------------------------------------------------

// Setup Animation Sequence With Required Frames
void About::setupFrames(RomLoader* rom0)
{
    // ROM Address of spin sequence
    uint32_t spin_anim_adr = 0x2424;

    // Sprite Zoom
    const static int ZOOM_LEVEL = 0x7F;

    // Read animation sequence and create sprites
    while (true)
    {
        SpriteFormat entry;
        uint32_t adr = rom0->read32(spin_anim_adr);
        entry.width  = rom0->read8(WH_TABLE + (ZOOM_LEVEL << 8) + rom0->read8(adr + 1));
        entry.height = rom0->read8(WH_TABLE + (ZOOM_LEVEL << 8) + rom0->read8(adr + 3));
        entry.bank   = rom0->read8(adr + 7);
        entry.offset = rom0->read16(adr + 8);
        pals.push_back(rom0->read8(spin_anim_adr + 4));
        sprites.push_back(entry);

        // End of animation sequence
        if (rom0->read8(spin_anim_adr + 7))
            break;
        // Advance to next entry
        else
            spin_anim_adr += 8;
    }

    currentFrame = 0;
}

// Advance animation sequence to next frame
void About::advanceFrame()
{
    if (currentFrame == -1)
        return;

    SpriteFormat entry = sprites.at(currentFrame);
    ui->sprite->setSprite(entry.bank, entry.offset, entry.width, entry.height, false, pals.at(currentFrame));

    if (++currentFrame >= sprites.length())
        currentFrame = 0;
}
