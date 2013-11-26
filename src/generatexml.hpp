/***************************************************************************
    Layout Project Save/Load Handler.

    Features:
    - Load & Save Project to XML File.

    References:
    http://www.developer.nokia.com/Community/Wiki/Generate_XML_programatically_in_Qt

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef GENERATEXML_H
#define GENERATEXML_H

#include <QList>
#include "stdint.hpp"

class QXmlStreamAttributes;
class QXmlStreamReader;
class QXmlStreamWriter;
class QString;

class Levels;
class LevelData;
class HeightSection;
struct HeightSegment;
class SpriteSection;
struct SpriteSectionEntry;

class GenerateXML
{
public:
    GenerateXML(Levels* levels, HeightSection* heightSection, SpriteSection *spriteSection);

    void loadProject(QString& filename);
    void saveProject(QString& filename);

private:
    // Internal LayOut Save Format
    const static int SAVE_VERSION = 1;

    Levels* levels;
    HeightSection* heightSection;
    SpriteSection* spriteSection;
    QList<HeightSegment>* heightSections;
    QList<SpriteSectionEntry>* spriteSections;

    void readSettings(QXmlStreamReader& stream);
    void readLevelMappingData(QXmlStreamReader& stream);
    void readLevelList(QXmlStreamReader& stream);
    void readPathData(QXmlStreamReader& stream);
    void readWidthData(QXmlStreamReader& stream);
    void readHeightData(QXmlStreamReader& stream);
    void readHeightMapData(QXmlStreamReader& stream);
    void readSceneryData(QXmlStreamReader& stream);
    void readSceneryPatternData(QXmlStreamReader& stream);
    void readSharedPalettes(QXmlStreamReader& stream);

    void writeLevel(QXmlStreamWriter& stream, int index);
    void writePalette(QXmlStreamWriter& stream, const QString& name, uint32_t *data, const int pals, const int length);

    int getAttInt(QXmlStreamAttributes &att, QString s);
    QString getAttString(QXmlStreamAttributes &att, QString s);
};

#endif // GENERATEXML_H
