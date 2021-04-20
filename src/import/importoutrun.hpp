/***************************************************************************
    Import And Convert OutRun World Rev. B Rom Data

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef IMPORTOUTRUN_HPP
#define IMPORTOUTRUN_HPP

#include "importbase.hpp"
#include "romloader.hpp"

class LevelData;

class ImportOutRun : public ImportBase
{
public:
    // Number of normal levels, i.e. not end sections
    static const int NORMAL_LEVELS = 18;

    bool romsLoaded;

    // OutRun Romset
    RomLoader rom0;
    RomLoader rom1;
    RomLoader tiles;
    RomLoader sprites;
    RomLoader road;

    ImportOutRun();
    virtual ~ImportOutRun();

    QList<QString> getLevelNames();
    void unloadRoms();
    bool loadRevBRoms(QString path);
    bool loadLevel(int id, const bool loadPatterns = true);
    bool loadSplit(LevelData* level,const bool loadPatterns = true);
    bool loadEndSection(LevelData* level,const bool loadPatterns = true);
    bool loadLevelData(uint32_t stageAdr, LevelData* level, const int levelLength, const bool loadPatterns);
    QList<HeightSegment> loadHeightSections();
    void loadSharedPalette();
    void loadLevelPalette(int id);
    QList<SpriteFormat> loadSpriteList();
    uint8_t* getPaletteData();
    QList<SpriteSectionEntry> loadSpriteSections(int id = 0);

private:
    int guessAngle(int);
};

#endif // IMPORTOUTRUN_HPP
