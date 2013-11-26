/***************************************************************************
    Global Level Management.
    Handles data shared between levels.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef LEVELS_HPP
#define LEVELS_HPP

#include <QWidget>
#include "../globals.hpp"
#include "../leveldata.hpp"

class QRadioButton;
class QLabel;

namespace Ui {
class Levels;
}

class Levels : public QWidget
{
    Q_OBJECT
    
public:
    // Sections Headings
    static const int NORMAL = 0;
    static const int END    = 1;
    static const int SPLIT  = 2;

    // Slots in mapping array (15 levels + end sections)
    static const int MAP_SLOTS = 20;

    explicit Levels(QWidget *parent = 0, LevelPalette* roadPalette = NULL);
    ~Levels();
    void init();
    void setDefaultMapping();
    void setStartLine(bool enabled);
    bool displayStartLine();
    bool levelContainsStartLine();
    void selectFirstLevel();
    QString getLevelName(int index);
    void renameLevel(int index, QString name);
    void remapScenerySections(int, int mode = 0);
    void swapScenerySections(int, int);
    void remapHeightSections(int, int mode = 0);
    void swapHeightSections(int, int);
    void setMappedLevel(int stage, int value);
    void setLevel(int section, int index = 0);
    void newLevel();
    void newLevel(QString name);
    void newEndSection();
    void newEndSection(QString name);

    // Get Split Level
    LevelData* getSplit()                 { return levels[normalSection->rowCount() + endSection->rowCount()]; }

    // Get Default End Level
    LevelData* getEndSection()            { return levels[normalSection->rowCount()]; }

    // Get All Levels
    QList<LevelData*>* getLevels()        { return &levels; }

    // Get Total Number Of Levels Including Split and End Sections
    int getNumberOfLevels()               { return levels.size(); }

    // get Number of Normal Levels
    int getNumNormalLevels()              { return normalSection->rowCount(); }

    // Get Number Of End Section Levels
    int getNumEndSections()               { return endSection->rowCount(); }

    // Get The Active Level Opened In The Editor
    int getActiveLevel()                  { return activeLevel; }

    // Get The Level Mapped To this Stage
    int getMappedLevel(int stage)         { return levelMap[stage]; }

    // Get A Pointer To The Level Mapped To This Stage
    LevelData* getMappedLevelP(int stage) { return levels[levelMap[stage]]; }

signals:
    void loadLevel();
    void refreshPreview();

public slots:
    void setLevel();
    void newLevelButton();

private slots:
    void deleteLevel();
    void mapLevel();
    void updateLevelLabels(QStandardItem* item = NULL);
    void updateDeleteButton();
    void updateMapButton();
    void updateEditButton();

private:
    Ui::Levels *ui;

    // Shared Level Palette Data
    LevelPalette* pal;

    // List of Levels
    QList<LevelData*> levels;

    // Shared path for level end section
    QList<PathPoint> endSectionPath;

    // Mapped Levels
    int levelMap[MAP_SLOTS];

    // Length of text labels in mapping UI
    static const int LABEL_LENGTH = 16;

    // Level Mappign Radio Buttons And Labels
    QRadioButton** radios;
    QLabel** labels;

    QStandardItem* normalSection;
    QStandardItem* splitSection;
    QStandardItem* endSection;

    // Used for level naming. This is NOT the number of levels.
    int levelCount;
    int endCount;

    // The Level currently being edited
    int activeLevel;

    void setDefaultPalette();
    void insertLevel(QStandardItem *parent, QString name, LevelData* level, int pos);
    void loadLevel(const char* name);
    int getSelectedRow();
    int getSectionIndex(bool allowClickOnHeader = false);
    int getRadioIndex();
};

#endif // LEVELS_HPP
