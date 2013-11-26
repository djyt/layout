/***************************************************************************
    Import And Convert OutRun World Rev. B Rom Data

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QtCore/qmath.h> // Sqrt

#include "../leveldata.hpp"
#include "../sprites/spritelist.hpp"
#include "../sprites/spritesection.hpp"
#include "../height/heightlabels.hpp"
#include "outrunlabels.hpp"
#include "importoutrun.hpp"


// ------------------------------------------------------------------------------------------------
// OutRun Rev. B ROM Addresses
// ------------------------------------------------------------------------------------------------

// Individual Stage Addresses. These correspond to the level list in outrunlabels.hpp
const static uint32_t STAGE_LOOKUPS[] =
{
    0x1DD36, // 0:  Stage 1
    0x1B598, // 1:  Stage 2a
    0x1DB5E, // 2:  Stage 2b [Gateway]
    0x1B886, // 3:  Stage 3a
    0x1CD0E, // 4:  Stage 3b
    0x1C812, // 5:  Stage 3c
    0x1CB00, // 6:  Stage 4a
    0x1D45E, // 7:  Stage 4b
    0x1D1B8, // 8:  Stage 4c
    0x1BAB6, // 9:  Stage 4d
    0x1CFBA, // 10: Stage 5a
    0x1C004, // 11: Stage 5b
    0x1D710, // 12: Stage 5c
    0x1BD1A, // 13: Stage 5d
    0x1C4E4, // 14: Stage 5e
    0x1D904, // Coconut Unused
    0x1C2B6, // Gateway Unused
    0x1B7BC, // Unknown?

    0x1DFE0, // End Section 1
    0x1E03C, // End Section 2
    0x1E098, // End Section 3
    0x1E0E8, // End Section 4
    0x1E140, // End Section 5
};
const static uint32_t ROAD_SPLIT_LOOKUP   = 0x1DFA4; // Road Split Section
const static uint32_t HEIGHT_ADR          = 0x220A;  // Address of Height Segments
const static uint32_t SPRITE_MASTER_TABLE = 0x1A43C; // Address of Sprite Segments
const static uint32_t PALETTE             = 0x14ED8; // Palette Data
const static uint32_t PAL_SKY_TABLE       = 0x17590; // Sky Palette Data: Table of palette addresses (15 entries)
const static uint32_t PAL_GND_TABLE       = 0x17350; // Table of long addresses of ground colours (16 entries)

ImportOutRun::ImportOutRun()
{
    romsLoaded = false;
}

QList<QString> ImportOutRun::getLevelNames()
{
    QList<QString> levelList;
    for (int i = 0; i < 23; i++)
        levelList.push_back(LEVEL_LIST[i]);
    return levelList;
}

bool ImportOutRun::loadLevel(int id, const bool loadPatterns)
{
    // Normal Level
    if (id < NORMAL_LEVELS)
    {
        // --------------------------------------------------------------------------------------------
        // Import Palette
        // --------------------------------------------------------------------------------------------

        loadLevelPalette(id);

        // --------------------------------------------------------------------------------------------
        // Import Road Points
        // --------------------------------------------------------------------------------------------

        return loadLevelData(STAGE_LOOKUPS[id] + 24, levelData, LevelData::LEVEL_LENGTH, loadPatterns);
    }
    // End Section (No Palette Data)
    else
    {
        return loadLevelData(STAGE_LOOKUPS[id], levelData, levelData->length, loadPatterns);
    }
}

// Load Level Split Data Section. Does Not Contain Palette Info.
bool ImportOutRun::loadSplit(LevelData *level, const bool loadPatterns)
{
    if (!romsLoaded)
        return false;

    return loadLevelData(ROAD_SPLIT_LOOKUP, level, level->length, loadPatterns);
}

// Load Default End Section
bool ImportOutRun::loadEndSection(LevelData *level, const bool loadPatterns)
{
    if (!romsLoaded)
        return false;

    return loadLevelData(STAGE_LOOKUPS[NORMAL_LEVELS], level, level->length, loadPatterns);
}

bool ImportOutRun::loadLevelData(uint32_t stageAdr, LevelData* level, const int levelLength, const bool loadPatterns)
{
    level->points->clear();
    uint32_t adr = rom0.read32(stageAdr + 0);
    int count = 0;

    PathPoint prev;

    while (true)
    {
        int16_t pos = rom0.read16(&adr);

        if (pos == -1)
        {
            prev.length = levelLength - prev.pos;
            level->points->push_back(prev);
            break;
        }

        uint16_t curveInfo = rom0.read16(&adr);
        uint16_t curveType = rom0.read16(&adr);

        // Create Path Point
        PathPoint pp;
        pp.pos = pos;
        pp.angle_inc = guessAngle(curveInfo);

        if (curveType == 2)
            pp.angle_inc = -pp.angle_inc;

        if (count++ > 0)
        {
            prev.length = pos - prev.pos;

            // Split long segment lengths into two seperate sections
            if (prev.length > LevelData::SECTION_LENGTH_MAX)
            {
                int length = prev.length;

                prev.length = LevelData::SECTION_LENGTH_MAX;
                level->points->push_back(prev);

                prev.pos += LevelData::SECTION_LENGTH_MAX;
                prev.length = length - LevelData::SECTION_LENGTH_MAX;
            }

            level->points->push_back(prev);
        }
        prev = pp;
    }

    // --------------------------------------------------------------------------------------------
    // Import Width & Height Control Points
    // --------------------------------------------------------------------------------------------

    level->widthP.clear();
    level->heightP.clear();
    adr = rom0.read32(stageAdr + 4);

    while (true)
    {
        const uint16_t pos = rom0.read16(&adr);

        if (pos == 0x7FFF)
            break;

        const uint16_t type   = rom0.read16(&adr);
        const uint16_t value1 = rom0.read16(&adr);
        const uint16_t value2 = rom0.read16(&adr);

        ControlPoint cp;
        cp.pos    = pos;
        cp.type   = type;
        cp.value1 = value1; // id of height segment
        cp.value2 = value2;

        if (type == 0 && loadPatterns)
            level->heightP.push_back(cp);

        else if (type == 1)
            level->widthP.push_back(cp);
    }

    // --------------------------------------------------------------------------------------------
    // Import Sprite Section Control Points
    // --------------------------------------------------------------------------------------------

    if (loadPatterns)
    {
        level->spriteP.clear();
        adr = rom0.read32(stageAdr + 8);

        while (true)
        {
            const uint16_t pos = rom0.read16(&adr);

            if (pos == 0x7FFF)
                break;

            const uint8_t no_sprites   = rom0.read8(&adr); // Number Of Sprites In Segment
            const uint8_t sprite_index = rom0.read8(&adr); // Sprite Data Entry Number From Lookup Table

            ControlPoint cp;
            cp.pos    = pos;
            cp.value1 = no_sprites;
            cp.value2 = sprite_index;
            level->spriteP.push_back(cp);
        }
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
// guessAngle
//
// Routine to 'guess' the angle of a curved section of track.
//
// We generate this value so that track segments can be easily edited and the relevant metadata is
// created which doesn't exist in the raw output.
//
// There might be a better way to do this than brute force?
// -----------------------------------------------------------------------------------------------

int ImportOutRun::guessAngle(int curveInfo)
{
    qreal angle = 0;

    int minDiff = 1000;
    int bestAngle = -1;

    for (int angleInc = 0; angleInc < 300; angleInc++)
    {
        qreal inc = ((qreal) angleInc / 10000);

        const int x1 = qSin(angle) * LevelData::FIXED_ONE;
        const int y1 = qCos(angle) * LevelData::FIXED_ONE;
        const int x2 = qSin(angle + inc) * LevelData::FIXED_ONE;
        const int y2 = qCos(angle + inc) * LevelData::FIXED_ONE;

        const int xdiff = x2 - x1;
        const int ydiff = y2 - y1;

        const double l = qSqrt((xdiff*xdiff)+(ydiff*ydiff));
        const int curveInfoNew = l == 0 ? 0 : (1.0 / l) * LevelData::FIXED_ONE;

        int diff = qAbs(curveInfoNew - curveInfo);
        if (diff <= minDiff)
        {
            minDiff = diff;
            bestAngle = angleInc;

            // Found optimal angle
            if (diff == 0)
                return angleInc;
        }
    }

    // Error: Did Not Calculate Curve Info Correctly
    if (bestAngle == -1)
    {
        bestAngle = 0;
    }
    /*else if (minDiff != 0)
    {
        std::cout << "Had to guess " << bestAngle << std::endl;
    }*/
    return bestAngle;
}

QList<HeightSegment> ImportOutRun::loadHeightSections()
{
    QList<HeightSegment> list;
    list.clear();

    const static int HEIGHT_ENTRIES = 225;

    uint32_t adr_p = HEIGHT_ADR;

    for (int i = 0; i < HEIGHT_ENTRIES; i++)
    {
        HeightSegment seg;
        uint32_t adr = rom1.read32(&adr_p); // Read address of segment from pointer
        seg.type = rom1.read8(&adr);
        seg.name = QString(HEIGHT_LABELS[seg.type]).append(": Segment ").append(QString::number(i));
        seg.step = rom1.read8(&adr);

        switch (seg.type)
        {
        case 0:
            seg.value1 = rom1.read8(&adr); // down multiplier
            seg.value2 = rom1.read8(&adr); // up multiplier
            break;

        case 1:
        case 2:
        case 3:
            seg.value1 = rom1.read16(&adr); // delay
            seg.value2 = 0; // unused
            break;

        case 4:
            seg.value1 = rom1.read16(&adr); // New Height Position
            seg.value2 = 0; // unused
            break;
        }

        // Read Height Data
        if (seg.type != 4)
        {
            int16_t v;

            while ((v = rom1.read16(&adr)) != -1)
                seg.data.push_back(v);
        }
        list.push_back(seg);
    }

    return list;
}

void ImportOutRun::loadSharedPalette()
{
    uint32_t adr;
    // Road Palette Entries
    for (int pal = 0; pal < LevelPalette::ROAD_PALS; pal++)
    {
        adr = rom0.read32(STAGE_LOOKUPS[pal] + 4);
        levelData->pal->road[pal][LevelPalette::CENTRE1] = rom0.read32(&adr);
        levelData->pal->road[pal][LevelPalette::CENTRE2] = rom0.read32(adr);

        adr = rom0.read32(STAGE_LOOKUPS[pal] + 8);
        levelData->pal->road[pal][LevelPalette::STRIPE1] = rom0.read32(&adr);
        levelData->pal->road[pal][LevelPalette::STRIPE2] = rom0.read32(adr);

        adr = rom0.read32(STAGE_LOOKUPS[pal] + 12);
        levelData->pal->road[pal][LevelPalette::SIDE1] = rom0.read32(&adr);
        levelData->pal->road[pal][LevelPalette::SIDE2] = rom0.read32(adr);

        adr = rom0.read32(STAGE_LOOKUPS[pal] + 16);
        levelData->pal->road[pal][LevelPalette::ROAD1] = rom0.read32(&adr);
        levelData->pal->road[pal][LevelPalette::ROAD2] = rom0.read32(adr);
    }

    // Sky Palette Entries
    for (int pal = 0; pal < LevelPalette::SKY_PALS; pal++)
    {
        uint32_t src = rom0.read32(PAL_SKY_TABLE + (pal << 2));

        for (int i = 0; i < LevelPalette::SKY_LENGTH; i++)
            levelData->pal->sky[pal][i] = rom0.read32(&src);
    }

    // Ground Palette Entries
    for (int pal = 0; pal < LevelPalette::GND_PALS; pal++)
    {
        uint32_t src = rom0.read32(PAL_GND_TABLE + (pal << 2));

        for (int i = 0; i < LevelPalette::GND_LENGTH; i++)
            levelData->pal->gnd[pal][i] = rom0.read32(&src);
    }
}

void ImportOutRun::loadLevelPalette(int id)
{
    levelData->roadPal = std::min(id, LevelPalette::ROAD_PALS - 1);

    uint32_t adr = rom0.read32(STAGE_LOOKUPS[id] + 0);
    levelData->skyPal  = rom0.read16(adr);

    adr = rom0.read32(STAGE_LOOKUPS[id] + 20);
    levelData->gndPal = rom0.read16(adr);

    loadSharedPalette();
}

QList<SpriteFormat> ImportOutRun::loadSpriteList()
{
    QList<SpriteFormat> list;

    const static int SPRITE_ENTRIES = 150;
    const static int ZOOM_LEVEL     = 0x7F;

    uint32_t adr_p = SPRITELIST_ADR;

    for (int i = 0; i < SPRITE_ENTRIES; i++)
    {
        const uint32_t adr = rom0.read32(&adr_p); // Address of segment
        SpriteFormat entry;

        const OutrunSprite *src = &SPRITE_LIST[i];

        entry.name           = src->name.arg(i+1);
        entry.canBeHidden    = src->canBeHidden;
        entry.defaultPal     = src->pal;
        entry.defaultRoutine = src->routine;
        entry.defaultShadow  = src->shadow;

        entry.width  = rom0.read8(WH_TABLE + (ZOOM_LEVEL << 8) + rom0.read8(adr + 1));
        entry.height = rom0.read8(WH_TABLE + (ZOOM_LEVEL << 8) + rom0.read8(adr + 3));
        entry.bank   = rom0.read8(adr + 7);
        entry.offset = rom0.read16(adr + 8);
        list.push_back(entry);
    }

    return list;
}

uint8_t* ImportOutRun::getPaletteData()
{
    return &rom0.rom[PALETTE];
}

QList<SpriteSectionEntry> ImportOutRun::loadSpriteSections(int id)
{
    QList<SpriteSectionEntry> list;
    const static int TABLE_ENTRIES = 230;

    uint32_t adr_p    = SPRITE_MASTER_TABLE;
    const int ENTRIES = id == 0 ? TABLE_ENTRIES : id + 1;

    for (int i = 0; i < ENTRIES; i++)
    {
        uint32_t adr = rom0.read32(&adr_p); // Address of segment

        SpriteSectionEntry section;
        section.selected  = false;
        section.name      = SCENERY_LIST[i].arg(i+1);
        section.frequency = rom0.read16(&adr);
        int offset        = rom0.read16(&adr);

        SpriteEntry entry;

        // Iterate blocks of sprite in section
        while (offset >= 0)
        {
            entry.selected = false;
            entry.props    = rom0.read8 (adr + offset + 0);
            entry.x        = rom0.read8 (adr + offset + 1);
            entry.y        = rom0.read16(adr + offset + 2);
            entry.type     = rom0.read8 (adr + offset + 5);
            entry.pal      = rom0.read8 (adr + offset + 7);

            // Get rid of identical entry, change 14 to 10
            if (entry.props >> 4 == 14)
                entry.props = (entry.props & 0xF) + (10 << 4);
            // Force water entries to use water sprite
            else if (entry.props >> 4 == 3)
                entry.type = 62;
            // Hard Code Cloud Palette
            else if (entry.props >> 4 == 2)
                entry.pal = 205;

            // Invalid sprite indexes are used at times. Get rid of them!
            if (entry.type > 149)
                entry.type = 0;

            offset -= 8;

            section.sprites.push_front(entry);
        }

        if (id == 0 || i == id)
            list.push_back(section);
    }

    return list;
}

// ------------------------------------------------------------------------------------------------
// Rom Load & Unload
// ------------------------------------------------------------------------------------------------

void ImportOutRun::unloadRoms()
{
    rom0.unload();
    rom1.unload();
    tiles.unload();
    road.unload();
    sprites.unload();
}

bool ImportOutRun::loadRevBRoms(QString path)
{
    rom0.setRomPath(path.toStdString());
    rom1.setRomPath(path.toStdString());
    tiles.setRomPath(path.toStdString());
    road.setRomPath(path.toStdString());
    sprites.setRomPath(path.toStdString());

    // If incremented, a rom has failed to load.
    int status = 0;

    // Load Master CPU ROMs
    rom0.init(0x40000);
    status += rom0.load("epr-10381a.132", 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2);

    // Try alternate filename for this rom
    if (status)
    {
        rom0.load("epr-10381b.132", 0x20000, 0x10000, 0xbe8c412b, RomLoader::INTERLEAVE2);
        status--;
    }

    status += rom0.load("epr-10383b.117", 0x20001, 0x10000, 0x10a2014a, RomLoader::INTERLEAVE2);
    status += rom0.load("epr-10380b.133", 0x00000, 0x10000, 0x1f6cadad, RomLoader::INTERLEAVE2);
    status += rom0.load("epr-10382b.118", 0x00001, 0x10000, 0xc4c3fa1a, RomLoader::INTERLEAVE2);

    // Load Slave CPU ROMs
    rom1.init(0x40000);
    status += rom1.load("epr-10327a.76", 0x00000, 0x10000, 0xe28a5baf, RomLoader::INTERLEAVE2);
    status += rom1.load("epr-10329a.58", 0x00001, 0x10000, 0xda131c81, RomLoader::INTERLEAVE2);
    status += rom1.load("epr-10328a.75", 0x20000, 0x10000, 0xd5ec5e5d, RomLoader::INTERLEAVE2);
    status += rom1.load("epr-10330a.57", 0x20001, 0x10000, 0xba9ec82a, RomLoader::INTERLEAVE2);

    // Load Non-Interleaved Tile ROMs
    tiles.init(0x30000);
    status += tiles.load("opr-10268.99",  0x00000, 0x08000, 0x95344b04);
    status += tiles.load("opr-10232.102", 0x08000, 0x08000, 0x776ba1eb);
    status += tiles.load("opr-10267.100", 0x10000, 0x08000, 0xa85bb823);
    status += tiles.load("opr-10231.103", 0x18000, 0x08000, 0x8908bcbf);
    status += tiles.load("opr-10266.101", 0x20000, 0x08000, 0x9f6f1a74);
    status += tiles.load("opr-10230.104", 0x28000, 0x08000, 0x686f5e50);

    // Load Non-Interleaved Road ROMs (2 identical roms, 1 for each road)
    road.init(0x10000);
    status += road.load("opr-10185.11", 0x000000, 0x08000, 0x22794426);
    status += road.load("opr-10186.47", 0x008000, 0x08000, 0x22794426);

    // Load Interleaved Sprite ROMs
    sprites.init(0x100000);
    status += sprites.load("mpr-10371.9",  0x000000, 0x20000, 0x7cc86208, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10373.10", 0x000001, 0x20000, 0xb0d26ac9, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10375.11", 0x000002, 0x20000, 0x59b60bd7, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10377.12", 0x000003, 0x20000, 0x17a1b04a, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10372.13", 0x080000, 0x20000, 0xb557078c, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10374.14", 0x080001, 0x20000, 0x8051e517, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10376.15", 0x080002, 0x20000, 0xf3b8f318, RomLoader::INTERLEAVE4);
    status += sprites.load("mpr-10378.16", 0x080003, 0x20000, 0xa1062984, RomLoader::INTERLEAVE4);

    // If status has been incremented, a rom has failed to load.
    return romsLoaded = status == 0;
}
