/***************************************************************************
    Height Sections Representation.
    Height Sections are shared across all levels in a project.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QListView>
#include <QStandardItemModel>

#include "heightsection.hpp"
#include "../levels/levels.hpp"

HeightSection::HeightSection(QObject *parent, QList<HeightSegment>* sectionList, QListView* view, Levels *levels) :
    QObject(parent)
{
    this->sectionList = sectionList; // List of height sections
    this->view        = view;        // UI: The List view visualizing the sprites that the user can select them from
    this->levels      = levels;      // Height points are shared between levels

    // Setup List View (Note in QT models should not be stored.)
    view->setModel(new QStandardItemModel());

    generate();

    // Track all changes to the selection
    connect(view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(setCurrentSection()));
}

// Generate Model Data
void HeightSection::generate()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    model->clear();
    QStandardItem *parentItem = model->invisibleRootItem();

    foreach (HeightSegment entry, *sectionList)
        parentItem->appendRow(new QStandardItem(entry.name));

    heightPoint = -1;

    emit setDeleteSecBut(sectionList->size() > 1);
    emit setInsertSecBut(sectionList->size() < 255);
}

void HeightSection::setSection(int index)
{
    QModelIndex mi = view->model()->index(index, 0);
    view->setCurrentIndex(mi);
}

void HeightSection::setCurrentSection()
{
    HeightSegment* section = getCurrentSection();

    if (section != NULL)
        emit updateSection(section);
}

HeightSegment* HeightSection::getCurrentSection()
{
    QModelIndex index = view->selectionModel()->currentIndex();

    if (!index.isValid())
        return NULL;
    else
        return &(*sectionList)[index.row()];
}

QList<HeightSegment>* HeightSection::getSectionList()
{
    return sectionList;
}

QString HeightSection::getSectionName(int sectionIndex)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();

    if (sectionIndex < model->rowCount())
        return model->item(sectionIndex)->text();
    else
        return QString("invalid section index");
}

void HeightSection::setSectionName(int sectionIndex, QString text)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();

    if (sectionIndex < model->rowCount())
        model->item(sectionIndex)->setText(text);
}

// ------------------------------------------------------------------------------------------------
// Slots
// ------------------------------------------------------------------------------------------------

void HeightSection::setStretch(int value)
{
    HeightSegment* currentSection = getCurrentSection();
    currentSection->step = value;
    emit refreshPreview();
}

void HeightSection::setStretchUp(int value)
{
    HeightSegment* currentSection = getCurrentSection();
    currentSection->value1 = value;
    emit refreshPreview();
}

void HeightSection::setStretchDown(int value)
{
    HeightSegment* currentSection = getCurrentSection();
    currentSection->value2 = value;
    emit refreshPreview();
}

void HeightSection::setDelay(int value)
{
    HeightSegment* currentSection = getCurrentSection();
    currentSection->value1 = value;
    emit refreshPreview();
}

void HeightSection::setHeightPoint(int pos)
{
    HeightSegment* currentSection = getCurrentSection();
    heightPoint = pos;
    emit setDeletePntBut(currentSection->data.size() > 6 && pos != -1);
}

void HeightSection::insertHeightPoint()
{
    HeightSegment* currentSection = getCurrentSection();

    const int insertPoint = heightPoint == -1 ? 0 : heightPoint;
    currentSection->data.insert(insertPoint, 0);
    emit sendNewPoint(insertPoint);
    emit refreshPreview();
    emit refreshWidget();
}

void HeightSection::deleteHeightPoint()
{
    if (heightPoint != -1)
    {
        HeightSegment* currentSection = getCurrentSection();
        currentSection->data.removeAt(heightPoint);

        if (heightPoint >= currentSection->data.size())
            heightPoint--;

        emit sendNewPoint(heightPoint);
        emit refreshPreview();
        emit refreshWidget();
    }
}

// Create a new height entry
void HeightSection::newEntry(int type)
{
    HeightSegment newSection;
    newSection.name = QString(HEIGHT_LABELS[type]).append(" : New Section");
    newSection.type = type;

    switch (type)
    {
        case 0:
            newSection.step   = 3;
            newSection.value1 = 3;
            newSection.value2 = 3;
            for (int i = 0; i < 7; i++)
                newSection.data.push_front(0);
            break;

        case 1:
        case 2:
            newSection.step   = 20;
            newSection.value1 = 250;
            newSection.data.push_front(16 * 32);
            newSection.data.push_front(16 * 32);
            break;

        case 3:
            newSection.step   = 20;
            newSection.value1 = 250;
            for (int i = 0; i < 6; i++)
            {
                newSection.data.push_front(0);
                newSection.data.push_back(16 * 32);
            }
            newSection.data.push_back(16 * 32);
            break;

        case 4:
            newSection.step   = 10;
            newSection.value1 = 0;
            newSection.value2 = 0;
            break;

    }

    QStandardItemModel* model = (QStandardItemModel*) view->model();
    const int index           = view->selectionModel()->currentIndex().row();
    sectionList->insert(index+1, newSection);
    model->insertRow(index+1, new QStandardItem(newSection.name));
    levels->remapHeightSections(index+1, 1);
    emit setDeleteSecBut(sectionList->size() > 1);
    emit setInsertSecBut(sectionList->size() < 255);
}

// Delete a complete height section
void HeightSection::deleteEntry()
{
    // Delete entry from model
    QModelIndex modelIndex = view->selectionModel()->currentIndex();
    const int index = modelIndex.row();

    // Delete entry from model
    QAbstractItemModel *model = view->model();
    model->removeRow(index, modelIndex.parent());

    sectionList->removeAt(index);

    // Remap existing sections in height layout
    levels->remapHeightSections(index, 0);

    emit setDeleteSecBut(sectionList->size() > 1);
    emit setInsertSecBut(sectionList->size() < 255);
}

// Duplicate the height entry
void HeightSection::duplicateEntry()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex modelIndex = view->selectionModel()->currentIndex();
    const int index = modelIndex.row();

    HeightSegment currentSection = sectionList->at(index);
    QStandardItem* oldSection = model->item(index);
    QStandardItem* newSection = new QStandardItem(QString("Copy of " + oldSection->text()));

    // Duplicate Section Entries
    model->insertRow(index+1, newSection);
    sectionList->insert(index+1, currentSection);
    levels->remapHeightSections(index+1, 1);

    emit setDeleteSecBut(sectionList->size() > 1);
    emit setInsertSecBut(sectionList->size() < 255);
}

void HeightSection::moveUp()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex modelIndex = view->selectionModel()->currentIndex();
    const int index = modelIndex.row();

    // Can't move up if already in top position
    if (index <= 0)
        return;

    QStandardItem* itemParent = model->invisibleRootItem();
    QStandardItem* swap1 = itemParent->takeChild(index, 0);
    QStandardItem* swap2 = itemParent->takeChild(index-1, 0);

    itemParent->setChild(index, swap2);
    itemParent->setChild(index-1, swap1);

    sectionList->swap(index, index-1);
    levels->swapHeightSections(index, index-1);
    view->selectionModel()->setCurrentIndex(swap1->index(), QItemSelectionModel::ClearAndSelect);
}

void HeightSection::moveDown()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex modelIndex = view->selectionModel()->currentIndex();
    const int index = modelIndex.row();

    QStandardItem* itemParent = model->invisibleRootItem();
    if (index < itemParent->rowCount() - 1)
    {
        QStandardItem* swap1 = itemParent->takeChild(index, 0);
        QStandardItem* swap2 = itemParent->takeChild(index+1, 0);

        itemParent->setChild(index, swap2);
        itemParent->setChild(index+1, swap1);

        sectionList->swap(index, index+1);
        levels->swapHeightSections(index, index+1);
        view->selectionModel()->setCurrentIndex(swap1->index(), QItemSelectionModel::ClearAndSelect);
    }
}
