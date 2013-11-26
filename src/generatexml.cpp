/***************************************************************************
    Layout Project Save/Load Handler.

    Features:
    - Load & Save Project to XML File.

    References:
    http://www.developer.nokia.com/Community/Wiki/Generate_XML_programatically_in_Qt

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QFile>
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "leveldata.hpp"
#include "levels/levels.hpp"
#include "height/heightsection.hpp"
#include "sprites/spritesection.hpp"
#include "generatexml.hpp"


GenerateXML::GenerateXML(Levels* levels, HeightSection *heightSection, SpriteSection* spriteSection)
{
    this->levels         = levels;
    this->heightSection  = heightSection;
    this->spriteSection  = spriteSection;
    this->heightSections = heightSection->getSectionList();
    this->spriteSections = spriteSection->getSectionList();
}

// ------------------------------------------------------------------------------------------------
//                                               LOADING
// ------------------------------------------------------------------------------------------------

void GenerateXML::loadProject(QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(0, "Read Error", "Unable to open XML file");
        return;
    }

    QXmlStreamReader stream(&file);

    while (!stream.atEnd() && !stream.hasError())
    {
        QXmlStreamReader::TokenType token = stream.readNext();

        if (token == QXmlStreamReader::StartDocument)
            continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (stream.name()      == "settings")        readSettings(stream);
            else if (stream.name() == "levelList")       readLevelList(stream);
            else if (stream.name() == "levelMapping")    readLevelMappingData(stream);
            else if (stream.name() == "heightMaps")      readHeightMapData(stream);
            else if (stream.name() == "sceneryPatterns") readSceneryPatternData(stream);
            else if (stream.name() == "sharedPalettes")  readSharedPalettes(stream);
        }

        if (stream.hasError())
        {
            QMessageBox::critical(0, "Project Error", stream.errorString(), QMessageBox::Ok);
        }
    }

    stream.clear();
    file.close();
}

void GenerateXML::readSettings(QXmlStreamReader& stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "startLine")
        {
            QXmlStreamAttributes att = stream.attributes();
            int enabled = getAttInt(att, "enabled");
            levels->setStartLine(enabled);
        }
        else if (stream.isEndElement() && stream.name() == "settings")
            return;
    }
}

void GenerateXML::readLevelMappingData(QXmlStreamReader &stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "stage")
        {
            QXmlStreamAttributes att = stream.attributes();
            int stage = getAttInt(att, "id");
            int map   = getAttInt(att, "mapping");
            levels->setMappedLevel(stage, map);
        }
        else if (stream.isEndElement() && stream.name() == "levelMapping")
            return;
    }
}
void GenerateXML::readLevelList(QXmlStreamReader &stream)
{
    QList<LevelData*>* list = levels->getLevels();

    int endSectionsCreated = 0;
    int levelsCreated = 0;
    int type = 0;

    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        // Insert New Level & Switch To It
        if (stream.isStartElement() && stream.name() == "level")
        {
            QXmlStreamAttributes att = stream.attributes();
            QString name = getAttString(att, "name");
            type         = getAttInt(att, "type");
            switch (type)
            {
                case Levels::NORMAL:
                    levels->newLevel();
                    break;

                case Levels::END:
                    levels->newEndSection();
                    endSectionsCreated++;
                    break;

                case Levels::SPLIT:
                    levels->getSplit()->clear();
                    break;
            }

            levels->renameLevel(levelsCreated, name);
            levelData = (*list)[levelsCreated++];
        }
        else if (stream.isEndElement() && stream.name() == "level")
        {
            levelData->updatePathData();
        }
        else if (stream.isStartElement() && stream.name() == "roadPalette")
        {
            QXmlStreamAttributes att = stream.attributes();
            levelData->gndPal  = getAttInt(att, "ground");
            levelData->roadPal = getAttInt(att, "road");
            levelData->skyPal  = getAttInt(att, "sky");
        }
        else if (stream.isStartElement() && stream.name() == "pathData")
        {
            if (type != Levels::END || endSectionsCreated <= 1)
                readPathData(stream);
        }
        else if (stream.isStartElement() && stream.name() == "widthData")   readWidthData(stream);
        else if (stream.isStartElement() && stream.name() == "heightData")  readHeightData(stream);
        else if (stream.isStartElement() && stream.name() == "sceneryData") readSceneryData(stream);

        // We're done inserting levels
        else if (stream.isEndElement() && stream.name() == "levelList")
            return;
    }
}

void GenerateXML::readPathData(QXmlStreamReader& stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "point")
        {
            int index = 0;
            PathPoint rp;

            QXmlStreamAttributes att = stream.attributes();
            index        = getAttInt(att, "index");
            rp.length    = getAttInt(att, "length");
            rp.angle_inc = getAttInt(att, "angle");

            levelData->points->insert(index, rp);
        }

        if (stream.isEndElement() && stream.name() == "pathData")
            return;
    }
}

void GenerateXML::readWidthData(QXmlStreamReader& stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "point")
        {
            int index = 0;
            ControlPoint cp;

            QXmlStreamAttributes att = stream.attributes();
            index     = getAttInt(att, "index");
            cp.pos    = getAttInt(att, "pos");
            cp.value1 = getAttInt(att, "width");
            cp.value2 = getAttInt(att, "change");

            levelData->widthP.insert(index, cp);
        }

        if (stream.isEndElement() && stream.name() == "widthData")
            return;
    }
}

void GenerateXML::readHeightData(QXmlStreamReader& stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "point")
        {
            int index = 0;
            ControlPoint cp;

            QXmlStreamAttributes att = stream.attributes();
            index     = getAttInt(att, "index");
            cp.pos    = getAttInt(att, "pos");
            cp.value1 = getAttInt(att, "map");
            cp.value2 = getAttInt(att, "spinindex");

            levelData->heightP.insert(index, cp);
        }

        if (stream.isEndElement() && stream.name() == "heightData")
            return;
    }
}

void GenerateXML::readHeightMapData(QXmlStreamReader &stream)
{
    HeightSegment seg;

    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "entry")
        {            
            seg.data.clear();

            QXmlStreamAttributes att = stream.attributes();
            seg.name   = getAttString(att, "name");
            seg.type   = getAttInt   (att, "type");
            seg.step   = getAttInt   (att, "step");
            seg.value1 = getAttInt   (att, "value1");
            seg.value2 = getAttInt   (att, "value2");
        }
        else if (stream.isStartElement() && stream.name() == "data")
        {
            QXmlStreamAttributes att = stream.attributes();
            QString values = getAttString(att, "value");
            QStringList split = values.split(",", QString::SkipEmptyParts);

            foreach(QString s, split)
                seg.data.push_back((int16_t) s.toInt());
        }

        else if (stream.isEndElement() && stream.name() == "entry")
        {
            heightSections->push_back(seg);
        }

        else if (stream.isEndElement() && stream.name() == "heightMaps")
        {
            return;
        }
    }
}

void GenerateXML::readSceneryData(QXmlStreamReader& stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "point")
        {
            ControlPoint cp;

            QXmlStreamAttributes att = stream.attributes();
            cp.pos    = getAttInt(att, "pos");
            cp.value1 = getAttInt(att, "length");
            cp.value2 = getAttInt(att, "index");

            levelData->spriteP.push_back(cp);
        }

        if (stream.isEndElement() && stream.name() == "sceneryData")
            return;
    }
}

void GenerateXML::readSceneryPatternData(QXmlStreamReader &stream)
{
    spriteSections->clear();
    QList<QString> sectionNames;
    QList<QString> spriteNames;

    SpriteSectionEntry section;

    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        // Start Pattern
        if (stream.isStartElement() && stream.name() == "pattern")
        {
            section.sprites.clear();
            QXmlStreamAttributes att = stream.attributes();
            sectionNames.push_back(getAttString(att, "name"));
            section.selected  = false;
            section.frequency = getAttInt(att, "freq");
        }
        // End Pattern
        else if (stream.isEndElement() && stream.name() == "pattern")
        {
            spriteSections->push_back(section);
        }
        // Sprite Entry Within Pattern
        else if (stream.isStartElement() && stream.name() == "sprite")
        {
            SpriteEntry sprite;
            QXmlStreamAttributes att = stream.attributes();
            spriteNames.push_back(getAttString(att, "name"));
            sprite.selected = false;
            sprite.type     = getAttInt(att, "type");
            sprite.x        = getAttInt(att, "x");
            sprite.y        = getAttInt(att, "y");
            sprite.pal      = getAttInt(att, "pal");
            sprite.props    = getAttInt(att, "props");
            section.sprites.push_back(sprite);
        }
        // End
        else if (stream.isEndElement() && stream.name() == "sceneryPatterns")
        {
            spriteSection->generateEntries(sectionNames, spriteNames);
            return;
        }
    }
}

void GenerateXML::readSharedPalettes(QXmlStreamReader &stream)
{
    while (stream.readNext() && !stream.atEnd() && !stream.hasError())
    {
        if (stream.isStartElement() && stream.name() == "road")
        {
            QXmlStreamAttributes att = stream.attributes();
            QString values = getAttString(att, "value");
            QStringList split = values.split(",", QString::SkipEmptyParts);

            for (int i = 0; i < LevelPalette::ROAD_PALS; i++)
                for (int j = 0; j < LevelPalette::ROAD_LENGTH; j++)
                    levelData->pal->road[i][j] = (uint32_t) split.at(i*LevelPalette::ROAD_LENGTH+j).toInt();
        }
        else if (stream.isStartElement() && stream.name() == "ground")
        {
            QXmlStreamAttributes att = stream.attributes();
            QString values = getAttString(att, "value");
            QStringList split = values.split(",", QString::SkipEmptyParts);

            for (int i = 0; i < LevelPalette::GND_PALS; i++)
                for (int j = 0; j < LevelPalette::GND_LENGTH; j++)
                    levelData->pal->gnd[i][j] = (uint32_t) split.at(i*LevelPalette::GND_LENGTH+j).toInt();
        }
        else if (stream.isStartElement() && stream.name() == "sky")
        {
            QXmlStreamAttributes att = stream.attributes();
            QString values = getAttString(att, "value");
            QStringList split = values.split(",", QString::SkipEmptyParts);

            for (int i = 0; i < LevelPalette::SKY_PALS; i++)
                for (int j = 0; j < LevelPalette::SKY_LENGTH; j++)
                    levelData->pal->sky[i][j] = (uint32_t) split.at(i*LevelPalette::SKY_LENGTH+j).toInt();
        }
        else if (stream.isEndElement() && stream.name() == "sharedPalettes")
        {
            return;
        }
    }
}

// ------------------------------------------------------------------------------------------------
//                                               SAVING
// ------------------------------------------------------------------------------------------------

void GenerateXML::saveProject(QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(0, "Read only", "The file is in read only mode");
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.setAutoFormattingIndent(4);

    // Writes a document start with the XML version number version.
    stream.writeStartDocument();
    stream.writeStartElement("LayOut");
        stream.writeAttribute("version", QString::number(SAVE_VERSION));

    // --------------------------------------------------------------------------------------------
    //                                          LEVELS
    // --------------------------------------------------------------------------------------------
    stream.writeStartElement("settings");
        stream.writeStartElement("startLine");
            stream.writeAttribute("enabled", QString::number(levels->displayStartLine() ? 1 : 0));
        stream.writeEndElement();
    stream.writeEndElement();

    stream.writeStartElement("levelList");
        const int numberOfLevels = levels->getNumberOfLevels();
        stream.writeAttribute("numberOfLevels", QString::number(numberOfLevels));
        for (int i = 0; i < numberOfLevels; i++)
            writeLevel(stream, i);
    stream.writeEndElement();

    stream.writeStartElement("levelMapping");
        for (int i = 0; i < Levels::MAP_SLOTS; i++)
        {
            stream.writeStartElement("stage");
                stream.writeAttribute("id",      QString::number(i));
                stream.writeAttribute("mapping", QString::number(levels->getMappedLevel(i)));
            stream.writeEndElement();
        }

    stream.writeEndElement();

    // --------------------------------------------------------------------------------------------
    //                                        SHARED DATA
    // --------------------------------------------------------------------------------------------

    // Write HeightMaps
    stream.writeStartElement("heightMaps");
    int sectionIndex = 0;
    foreach (HeightSegment seg, *heightSections)
    {
        stream.writeStartElement("entry");
            stream.writeAttribute("name",   heightSection->getSectionName(sectionIndex++));
            stream.writeAttribute("type",   QString::number(seg.type));
            stream.writeAttribute("step",   QString::number(seg.step));
            stream.writeAttribute("value1", QString::number(seg.value1));
            stream.writeAttribute("value2", QString::number(seg.value2));

            stream.writeStartElement("data");
                QString values;
                foreach (int16_t d, seg.data)
                {
                    values.append(QString::number(d));
                    values.append(",");
                }
                stream.writeAttribute("value", values);
            stream.writeEndElement();
        stream.writeEndElement();
    }
    stream.writeEndElement();

    // Write Scenery Patterns
    stream.writeStartElement("sceneryPatterns");
    sectionIndex = 0;
    int spriteIndex  = 0;
    foreach (SpriteSectionEntry section, *spriteSections)
    {
        stream.writeStartElement("pattern");
            stream.writeAttribute("name", spriteSection->getSectionName(sectionIndex));
            stream.writeAttribute("freq", QString::number(section.frequency));
            //stream.writeAttribute("noSprites", QString::number(section.sprites.size()));

            spriteIndex = 0;
            foreach (SpriteEntry sprite, section.sprites)
            {
                stream.writeStartElement("sprite");
                    stream.writeAttribute("name", spriteSection->getSpriteName(sectionIndex, spriteIndex++));
                    stream.writeAttribute("type",  QString::number(sprite.type));
                    stream.writeAttribute("x",     QString::number(sprite.x));
                    stream.writeAttribute("y",     QString::number(sprite.y));
                    stream.writeAttribute("pal",   QString::number(sprite.pal));
                    stream.writeAttribute("props", QString::number(sprite.props));
                stream.writeEndElement();
            }
            sectionIndex++;
        stream.writeEndElement(); // end pattern
    }
    stream.writeEndElement(); // end sceneryPattern

    // Write Shared Palettes
    stream.writeStartElement("sharedPalettes");
        writePalette(stream, "road",   &levelData->pal->road[0][0], LevelPalette::ROAD_PALS, LevelPalette::ROAD_LENGTH);
        writePalette(stream, "ground", &levelData->pal->gnd[0][0],  LevelPalette::GND_PALS,  LevelPalette::GND_LENGTH);
        writePalette(stream, "sky",    &levelData->pal->sky[0][0],  LevelPalette::SKY_PALS,  LevelPalette::SKY_LENGTH);
    stream.writeEndElement(); // end sharedPalettes

    stream.writeEndDocument();
    file.close();
}

void GenerateXML::writeLevel(QXmlStreamWriter& stream, int index)
{
    QList<LevelData*>* list = levels->getLevels();
    LevelData* level = (*list)[index];
    stream.writeStartElement("level");
    stream.writeAttribute("name", levels->getLevelName(index));
    stream.writeAttribute("type", QString::number(level->type));

        // Write Level Palette Data
        stream.writeStartElement("roadPalette");
            stream.writeAttribute("ground", QString::number(level->gndPal));
            stream.writeAttribute("road",   QString::number(level->roadPal));
            stream.writeAttribute("sky",    QString::number(level->skyPal));
        stream.writeEndElement();

        // Create tag <pathData> - Subsequent calls to writeAttribute() will add attributes to this element.
        // Write Path Points
        stream.writeStartElement("pathData");
        for (int i = 0; i < level->points->size(); i++)
        {
            PathPoint rp = level->points->at(i);
            stream.writeStartElement("point");
                stream.writeAttribute("index",  QString::number(i));
                stream.writeAttribute("length", QString::number(rp.length));
                stream.writeAttribute("angle",  QString::number(rp.angle_inc));
            stream.writeEndElement();
        }
        stream.writeEndElement();

        // Write Widths
        stream.writeStartElement("widthData");
        for (int i = 0; i < level->widthP.size(); i++)
        {
            ControlPoint wp = level->widthP.at(i);
            stream.writeStartElement("point");
                stream.writeAttribute("index",  QString::number(i));
                stream.writeAttribute("pos",    QString::number(wp.pos));
                stream.writeAttribute("width",  QString::number(wp.value1));
                stream.writeAttribute("change", QString::number(wp.value2));
            stream.writeEndElement();
        }
        stream.writeEndElement();

        // Write Heights
        stream.writeStartElement("heightData");
        for (int i = 0; i < level->heightP.size(); i++)
        {
            ControlPoint cp = level->heightP.at(i);
            stream.writeStartElement("point");
                stream.writeAttribute("index",     QString::number(i));
                stream.writeAttribute("pos",       QString::number(cp.pos));
                stream.writeAttribute("map",       QString::number(cp.value1));
                stream.writeAttribute("spinindex", QString::number(cp.value2));
            stream.writeEndElement();
        }
        stream.writeEndElement();

        // Write Scenery Placements
        stream.writeStartElement("sceneryData");
        foreach (ControlPoint cp, level->spriteP)
        {
            stream.writeStartElement("point");
                stream.writeAttribute("pos",       QString::number(cp.pos));
                stream.writeAttribute("length",    QString::number(cp.value1));
                stream.writeAttribute("index",     QString::number(cp.value2));
            stream.writeEndElement();
        }
        stream.writeEndElement();

    stream.writeEndElement();
}

void GenerateXML::writePalette(QXmlStreamWriter& stream, const QString& name, uint32_t *data, const int pals, const int length)
{
    stream.writeStartElement(name);
        stream.writeAttribute("pals",   QString::number(pals));
        stream.writeAttribute("length", QString::number(length));
        QString values;
        for (int i = 0; i < pals; i++)
        {
            for (int j = 0; j < length; j++)
            {
                values.append(QString::number(data[i*length+j]));
                values.append(",");
            }
        }
        stream.writeAttribute("value", values);
    stream.writeEndElement();
}

int GenerateXML::getAttInt(QXmlStreamAttributes &att, QString s)
{
    if (att.hasAttribute(s))
        return att.value(s).toString().toInt();
    else
        return 0;
}

QString GenerateXML::getAttString(QXmlStreamAttributes &att, QString s)
{
    if (att.hasAttribute(s))
        return att.value(s).toString();
    else
        return NULL;
}
