#ifndef SPRITESECTION_HPP
#define SPRITESECTION_HPP

#include <QObject>

#include "../stdint.hpp"
#include "spriteformat.hpp"

class SpriteList;
class QTreeView;
class QStandardItemModel;
class QItemSelectionModel;
class PreviewPalette;
class Levels;

class SpriteSection : public QObject
{
    Q_OBJECT
public:
    QTreeView* view;

    explicit SpriteSection(QObject *parent, QList<SpriteSectionEntry>* sectionList, SpriteList *spriteList, QTreeView* view, Levels* levels);
    ~SpriteSection();
    void generateEntries();
    void generateEntries(QList<QString>, QList<QString>);
    QList<SpriteSectionEntry>* getSectionList();
    QString getSectionName(int);
    void setSectionName(int, QString);
    QString getSpriteName(int, int);
    void setSpriteName(int, int, QString);
    bool isSectionSelected();
    
signals:
    void updateDensity(int);
    void updateDensityL(bool);
    void updateFrequency(int);
    void updateSprProps(SpriteEntry*);
    void refreshPreview();
    void setInsertSecBut(bool); // Enable Insert/Duplicate Section Button

public slots:
    void newEntry();
    void deleteEntry();
    void duplicateEntry();
    void moveUp();
    void moveDown();
    void setFrequency(int);
    void setShadow(bool);
    void setHFlip(bool);
    void setSprite(QString, int);
    void setSpriteX(int);
    void setSpriteY(int);
    void setSpriteP(int);
    void setDrawRoutine(int);
    void itemSelected(int, bool expand = true);
    
private slots:
    void itemSelected();

private:
    QList<SpriteSectionEntry>* sectionList;
    SpriteList* spriteList;

    Levels* levels;

    // Used to disable updates when changing between sprite entries as otherwise you
    // end up with a situation where:
    // Sprite Section Class Changes Sprite -> Change Control Values -> Send Signal Notifying Change -> Sprite Section Class Updates Sprite Data
    //
    // You really want this to work in two ways:
    // 1. Sprite Section Class Changes Sprite -> Change Control Values                [disableUpdates = true]
    // 2. User Changes Control Values ->  Sprite Section Class Updates Sprite Data    [disableUpdates = false]
    bool disableUpdates;

    void clearSelection();
    SpriteSectionEntry* getCurrentSection();
    SpriteEntry* getCurrentSprite();
    int getCurrentSectionID();
    int getCurrentSpriteID();

    void recalculateDensity(SpriteSectionEntry* currentSection);
    void calculateDensity(SpriteSectionEntry* currentSection);

};

#endif // SPRITESECTION_HPP
