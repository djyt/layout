/***************************************************************************
    Global Level Management.
    Handles data shared between levels.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/
#include <iostream>
#include "ui_levels.h"
#include "levels.hpp"

Levels::Levels(QWidget *parent, LevelPalette *roadPalette) :
    QWidget(parent),
    ui(new Ui::Levels)
{
    pal = roadPalette;

    ui->setupUi(this);

    // Setup List View (Note models should not be stored)
    ui->treeView->setModel(new QStandardItemModel());

    connect(ui->treeView->model(),          SIGNAL(itemChanged(QStandardItem*)),             this, SLOT(updateLevelLabels(QStandardItem*)));
    connect(ui->buttonSwitch,               SIGNAL(clicked()),                               this, SLOT(setLevel()));
    connect(ui->buttonNew,                  SIGNAL(clicked()),                               this, SLOT(newLevelButton()));
    connect(ui->buttonDelete,               SIGNAL(clicked()),                               this, SLOT(deleteLevel()));
    connect(ui->buttonMap,                  SIGNAL(clicked()),                               this, SLOT(mapLevel()));
    connect(ui->checkStartLine,             SIGNAL(clicked()),                               this, SIGNAL(refreshPreview()));
    connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(updateDeleteButton()));
    connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(updateMapButton()));
    connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(updateEditButton()));

    // Add all radio buttons and labels to an array for ease of use
    radios = new QRadioButton*[MAP_SLOTS];
    radios[0]  = ui->radio1;   // Normal Levels
    radios[1]  = ui->radio2b;
    radios[2]  = ui->radio2a;
    radios[3]  = ui->radio3c;
    radios[4]  = ui->radio3b;
    radios[5]  = ui->radio3a;
    radios[6]  = ui->radio4d;
    radios[7]  = ui->radio4c;
    radios[8]  = ui->radio4b;
    radios[9]  = ui->radio4a;
    radios[10] = ui->radio5e;
    radios[11] = ui->radio5d;
    radios[12] = ui->radio5c;
    radios[13] = ui->radio5b;
    radios[14] = ui->radio5a;
    radios[15] = ui->radio6e;  // End Sequence Levels
    radios[16] = ui->radio6d;
    radios[17] = ui->radio6c;
    radios[18] = ui->radio6b;
    radios[19] = ui->radio6a;

    for (int i = 0; i < MAP_SLOTS; i++)
        connect(radios[i], SIGNAL(clicked()), this, SLOT(updateMapButton()));

    labels = new QLabel*[MAP_SLOTS];
    labels[0]  = ui->label1;  // Normal Levels
    labels[1]  = ui->label2b;
    labels[2]  = ui->label2a;
    labels[3]  = ui->label3c;
    labels[4]  = ui->label3b;
    labels[5]  = ui->label3a;
    labels[6]  = ui->label4d;
    labels[7]  = ui->label4c;
    labels[8]  = ui->label4b;
    labels[9]  = ui->label4a;
    labels[10] = ui->label5e;
    labels[11] = ui->label5d;
    labels[12] = ui->label5c;
    labels[13] = ui->label5b;
    labels[14] = ui->label5a;
    labels[15] = ui->label6e; // End Sequence Levels
    labels[16] = ui->label6d;
    labels[17] = ui->label6c;
    labels[18] = ui->label6b;
    labels[19] = ui->label6a;
}

Levels::~Levels()
{
    delete ui;
    delete[] radios;
    radios = NULL;
    delete[] labels;
    labels = NULL;
    delete normalSection;
    delete splitSection;
    delete endSection;
}

void Levels::init()
{
    QStandardItemModel* model = (QStandardItemModel*) ui->treeView->model();
    model->clear();
    QStandardItem *parentItem = model->invisibleRootItem();

    // Add normal section entry
    normalSection = new QStandardItem("Levels");
    normalSection->setEditable(false);
    parentItem->appendRow(normalSection);

    // Add end section entry
    endSection = new QStandardItem("End Sections");
    endSection->setEditable(false);
    parentItem->appendRow(endSection);

    // Add split section entry
    splitSection = new QStandardItem("Split Section");
    splitSection->setEditable(false);
    splitSection->setCheckable(false);
    splitSection->setCheckState(Qt::Unchecked);
    parentItem->appendRow(splitSection);

    levelCount  = 0;
    endCount    = 0;
    activeLevel = 0;

    levels.clear();

    // Create split level. Manually add the checkpoint scenery entry.
    LevelData* split = new LevelData(pal, SPLIT);
    split->clear();
    ControlPoint checkPoint;
    checkPoint.pos    = 398;
    checkPoint.value1 = 4;  // number of sprites
    checkPoint.value2 = 0;  // index of checkpoint scenery pattern
    split->spriteP.push_back(checkPoint);
    levels.push_back(split);

    endSectionPath.clear();

    for (int i = 0; i < MAP_SLOTS; i++)
        levelMap[i] = -1;

    setDefaultPalette();
}

void Levels::setDefaultPalette()
{
    for (int i = 0; i < LevelPalette::ROAD_PALS; i++)
    {
        uint32_t* roadPal = pal->road[i];
        roadPal[LevelPalette::ROAD1]   = roadPal[LevelPalette::ROAD2]   = 0x79990999;
        roadPal[LevelPalette::SIDE1]   = roadPal[LevelPalette::SIDE2]   = 0x0FFF0999;
        roadPal[LevelPalette::STRIPE1] = roadPal[LevelPalette::STRIPE2] = 0x79990FFF;
        roadPal[LevelPalette::CENTRE1] = roadPal[LevelPalette::CENTRE2] = 0x79990999;
    }

    // Default sky colour
    for (int i = 0; i < LevelPalette::SKY_PALS; i++)
        for (int j = 0; j < LevelPalette::SKY_LENGTH; j++)
            pal->sky[i][j] = 0x4F904F90;

    // Default ground colour
    for (int i = 0; i < LevelPalette::GND_PALS; i++)
    {
        uint32_t* gndPal = pal->gnd[i];
        gndPal[0] = 0x89A789A;
        gndPal[1] = 0x89A789A;
        gndPal[2] = 0x9AB79AB;
        gndPal[3] = 0xABC7ABC;
        gndPal[4] = 0xBCD7BCD;
        gndPal[5] = 0xCDE7CDE;
        gndPal[6] = 0xCDE7CDE;
        gndPal[7] = 0xCDE7CDE;
    }
}

void Levels::setDefaultMapping()
{
    // Set Default Mapping (Every Stage mapped to first level)
    for (int i = 0; i < 15; i++)
        levelMap[i] = 0;
    for (int i = 15; i < MAP_SLOTS; i++)
        levelMap[i] = 1;

    getMappedLevelP(0)->startWidth = START_WIDTH_L1;

    updateLevelLabels(NULL);
}

// Select First Level In List
void Levels::selectFirstLevel()
{
    QStandardItemModel* model = (QStandardItemModel*) ui->treeView->model();
    levelData = levels[0];
    QModelIndex mi = model->index(0, 0);
    ui->treeView->setCurrentIndex(mi);
    ui->treeView->expandAll();

    radios[0]->setChecked(true);

    setLevel();
}

QString Levels::getLevelName(int index)
{
    if (index < getNumNormalLevels())
        return normalSection->child(index)->text();
    else if (index < getNumNormalLevels() + getNumEndSections())
        return endSection->child(index - getNumNormalLevels())->text();
    else
        return splitSection->text();
}

void Levels::newLevelButton()
{
    const int sectionIndex = getSectionIndex(true);

    // Normal Level
    if (sectionIndex == 0 || sectionIndex == 3)
    {
        newLevel();
    }
    // End Section
    else if (sectionIndex == 1)
    {
        newEndSection();
    }
}

void Levels::newLevel()
{
    newLevel(QString("Level %0").arg(++levelCount));
}

void Levels::newEndSection()
{
    newEndSection(QString("End %0").arg(++endCount));
}

void Levels::newLevel(QString name)
{
    LevelData* level = new LevelData(pal, NORMAL);
    level->clear();
    insertLevel(normalSection, name, level, normalSection->rowCount());
}

void Levels::newEndSection(QString name)
{
    LevelData* level = new LevelData(pal, END, &endSectionPath);
    level->clear();
    insertLevel(endSection, name, level, normalSection->rowCount() + endSection->rowCount());
}

void Levels::insertLevel(QStandardItem* parent, QString name, LevelData* level, int pos)
{
    // Update Mapping
    for (int i = 0; i < MAP_SLOTS; i++)
    {
        if (levelMap[i] >= pos)
            levelMap[i]++;
    }

    levels.insert(pos, level);
    QStandardItem* item = new QStandardItem(name);
    item->setCheckable(false);
    item->setCheckState(Qt::Unchecked);
    parent->appendRow(item);
    updateDeleteButton();
}

void Levels::deleteLevel()
{
    const int sectionIndex = getSectionIndex();

    if (sectionIndex == NORMAL)
    {
        const int deleteIndex  = getSelectedRow();

        // Update Mapping
        for (int i = 0; i < MAP_SLOTS; i++)
        {
            if (levelMap[i] >= deleteIndex)
            {
                levelMap[i]--;
                if (levelMap[i] < 0)
                    levelMap[i] = 0;
            }
        }

        // Delete entry from treeview
        normalSection->removeRow(deleteIndex);

        // Delete Data
        levels.removeAt(deleteIndex);

        if (deleteIndex == activeLevel)
            setLevel();
        else if (deleteIndex < activeLevel)
            activeLevel--;
    }
    else if (sectionIndex == END)
    {
        const int deleteIndex  = getSelectedRow() + getNumNormalLevels();

        // Update Mapping
        for (int i = 0; i < MAP_SLOTS; i++)
        {
            if (levelMap[i] >= deleteIndex)
            {
                levelMap[i]--;
                if (levelMap[i] < getNumNormalLevels())
                    levelMap[i] = getNumNormalLevels();
            }
        }

        // Delete entry from treeview
        endSection->removeRow(getSelectedRow());

        // Delete Data
        levels.removeAt(deleteIndex);

        if (deleteIndex == activeLevel)
            setLevel();
        else if (deleteIndex < activeLevel)
            activeLevel--;
    }

    updateLevelLabels(NULL);
    updateDeleteButton();
}

void Levels::setLevel()
{
    const int sectionIndex = getSectionIndex(true);
    if (sectionIndex != -1)
        setLevel(sectionIndex, getSelectedRow());
}

void Levels::setLevel(int section, int index)
{
    // Uncheck old items
    if (activeLevel != -1)
    {
        for (int i = 0; i < normalSection->rowCount(); i++)
            normalSection->child(i)->setCheckState(Qt::Unchecked);
        for (int i = 0; i < endSection->rowCount(); i++)
            endSection->child(i)->setCheckState(Qt::Unchecked);
        splitSection->setCheckState(Qt::Unchecked);
    }

    // Select new level
    if (section == NORMAL)
    {
        normalSection->child(index)->setCheckState(Qt::Checked);
        activeLevel = index;
    }
    else if (section == END)
    {
        endSection->child(index)->setCheckState(Qt::Checked);
        activeLevel = index + normalSection->rowCount();
    }
    else if (section == SPLIT)
    {
        splitSection->setCheckState(Qt::Checked);
        activeLevel = normalSection->rowCount() + endSection->rowCount();
    }

    levelData = levels[activeLevel];

    if (levelData->type == NORMAL)
    {
        if (getMappedLevel(0) == activeLevel)
            levelData->startWidth = START_WIDTH_L1;
        // Other Level
        else
            levelData->startWidth = START_WIDTH;
    }

    levelData->updatePathData();

    emit loadLevel();
}

void Levels::mapLevel()
{
    int index = getSelectedRow();

    if (index != -1)
    {
        int checked = getRadioIndex(); // Find index of level to map to
        if (checked != -1)
        {
            // Update label text
            if (checked < 15)
            {
                levelMap[checked] = index;
                labels[checked]->setText(normalSection->child(index)->text().left(LABEL_LENGTH));
            }
            else
            {
                levelMap[checked] = index + getNumNormalLevels();
                labels[checked]->setText(endSection->child(index)->text().left(LABEL_LENGTH));
            }
        }

        // The start width of a level is hard coded.
        if (getMappedLevel(0) == activeLevel)
            levelData->startWidth = START_WIDTH_L1;
        else
            levelData->startWidth = START_WIDTH;

        levelData->updateWidthData();
        emit refreshPreview();
    }
}

// Refresh list of names in mapping panel
void Levels::updateLevelLabels(QStandardItem*)
{
    for (int i = 0; i < MAP_SLOTS; i++)
    {
        int mapping = levelMap[i];
        if (mapping != -1)
        {
            if (mapping < normalSection->rowCount())
                labels[i]->setText(normalSection->child(mapping)->text().left(LABEL_LENGTH));
            else
                labels[i]->setText(endSection->child(mapping - getNumNormalLevels())->text().left(LABEL_LENGTH));
        }
    }
}

void Levels::renameLevel(int index, QString name)
{
    if (index < getNumNormalLevels())
        normalSection->child(index)->setText(name);
    else if (index < getNumNormalLevels() + getNumEndSections())
        endSection->child(index - normalSection->rowCount())->setText(name);
}

void Levels::setMappedLevel(int stage, int value)
{
    levelMap[stage] = value;
    // Update label text
    if (value < getNumNormalLevels())
        labels[stage]->setText(normalSection->child(value)->text().left(LABEL_LENGTH));
    else
        labels[stage]->setText(endSection->child(value - getNumNormalLevels())->text().left(LABEL_LENGTH));

}

// ------------------------------------------------------------------------------------------------
// TreeView Helper Functions
// ------------------------------------------------------------------------------------------------

int Levels::getSelectedRow()
{
    QModelIndex modelIndex = ui->treeView->selectionModel()->currentIndex();

    if (modelIndex.isValid())
        return modelIndex.row();
    else
        return -1;
}

int Levels::getSectionIndex(bool allowClickOnHeader)
{
    QModelIndex modelIndex = ui->treeView->selectionModel()->currentIndex();
    QModelIndex parent     = modelIndex.parent();

    // Clicked within section
    if (parent.isValid())
        return parent.row();
    // Otherwise clicked section heading
    else if (!allowClickOnHeader)
        return -1;
    else
        return modelIndex.row();
}

// Get the index of the radio button in the mapping section
// 0  to 14 (Normal Stage)
// 15 to 19 (End Sections)
int Levels::getRadioIndex()
{
    int checked = -1;
    // Find index of level to map to
    for (int i = 0; i < MAP_SLOTS; i++)
    {
        if (radios[i]->isChecked())
        {
            checked = i;
            break;
        }
    }
    return checked;
}

// ------------------------------------------------------------------------------------------------
// UI & Buttons
// ------------------------------------------------------------------------------------------------

// Show Delete Button if there is more than one normal level, and we're not over a special undeletable entry.
void Levels::updateDeleteButton()
{
    const int sectionIndex = getSectionIndex();

    // Normal Level
    if (sectionIndex == NORMAL)
    {
        ui->buttonDelete->setEnabled(normalSection->rowCount() > 1);
    }
    // End Section
    else if (sectionIndex == END)
    {
        ui->buttonDelete->setEnabled(endSection->rowCount() > 1);
    }
    // Clicked section heading or split
    else
    {
        ui->buttonDelete->setEnabled(false);
    }
}

// Show Map Button
void Levels::updateMapButton()
{
    const int sectionIndex = getSectionIndex();

    // Normal Level
    if (sectionIndex == NORMAL)
    {
        ui->buttonMap->setEnabled(getRadioIndex() < 15);
    }
    // End Section
    else if (sectionIndex == END)
    {
        ui->buttonMap->setEnabled(getRadioIndex() >= 15);
    }
    // Clicked section heading or split
    else
    {
        ui->buttonMap->setEnabled(false);
    }
}

void Levels::updateEditButton()
{
    ui->buttonSwitch->setEnabled(getSectionIndex()     == NORMAL ||
                                 getSectionIndex()     == END    ||
                                 getSectionIndex(true) == SPLIT);
}

// ------------------------------------------------------------------------------------------------
// Start Line Functions
// ------------------------------------------------------------------------------------------------

void Levels::setStartLine(bool enabled)
{
    ui->checkStartLine->setChecked(enabled);
}

bool Levels::displayStartLine()
{
    return ui->checkStartLine->isChecked();
}

bool Levels::levelContainsStartLine()
{
    return displayStartLine() && getMappedLevel(0) == getActiveLevel();
}

// ------------------------------------------------------------------------------------------------
// Functions to iterate and change internal level contents
// ------------------------------------------------------------------------------------------------

void Levels::swapScenerySections(int swap1, int swap2)
{
    foreach (LevelData* level, levels)
    {
        for (int i = 0; i < level->spriteP.size(); i++)
        {
            ControlPoint* cp = &level->spriteP[i];

            if (cp->value2 == swap1)
                cp->value2 = swap2;
            else if (cp->value2 == swap2)
                cp->value2 = swap1;
        }
    }
}

void Levels::remapScenerySections(int id, int mode)
{
    foreach (LevelData* level, levels)
    {
        QMutableListIterator<ControlPoint> iterator(level->spriteP);

        while (iterator.hasNext())
        {
            ControlPoint* cp = &iterator.next();

            // Delete specific id entries
            if (cp->value2 == id)
            {
                if (mode == 0)
                    iterator.remove();
                else
                    cp->value2++;
            }
            else if (cp->value2 > id)
            {
                // Scenery Section Was Deleted, Map Downwards
                if (mode == 0)
                {
                    cp->value2--;
                    if (cp->value2 < 0)
                        cp->value2 = 0;
                }
                // Scenery Section Was Inserted, Map Upwards
                else
                {
                    cp->value2++;
                }
            }
        }
    }
}

void Levels::swapHeightSections(int swap1, int swap2)
{
    foreach (LevelData* level, levels)
    {
        for (int i = 0; i < level->heightP.size(); i++)
        {
            ControlPoint* cp = &level->heightP[i];

            if (cp->value1 == swap1)
                cp->value1 = swap2;
            else if (cp->value1 == swap2)
                cp->value1 = swap1;
        }
    }
}


void Levels::remapHeightSections(int id, int mode)
{
    foreach (LevelData* level, levels)
    {
        QMutableListIterator<ControlPoint> iterator(level->heightP);

        while (iterator.hasNext())
        {
            ControlPoint* cp = &iterator.next();

            // Delete specific id entries
            if (cp->value1 == id)
            {
                if (mode == 0)
                    iterator.remove();
                else
                    cp->value1++;
            }

            else if (cp->value1 > id)
            {
                // Scenery Section Was Deleted, Map Downwards
                if (mode == 0)
                {
                    cp->value1--;
                    if (cp->value1 < 0)
                        cp->value1 = 0;
                }
                // Scenery Section Was Inserted, Map Upwards
                else
                {
                    cp->value1++;
                }
            }
        }
    }
}