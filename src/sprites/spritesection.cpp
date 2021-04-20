/***************************************************************************
    Section of Multiple Sprites.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QTreeView>
#include <QStandardItemModel>

#include "../leveldata.hpp"
#include "../previewpalette.hpp"
#include "../levels/levels.hpp"

#include "spritesection.hpp"
#include "spritelist.hpp"

SpriteSection::SpriteSection(QObject *parent, QList<SpriteSectionEntry>* sectionList, SpriteList* spriteList, QTreeView* view, Levels *levels) :
    QObject(parent)
{
    this->sectionList = sectionList; // List of sprite sections
    this->spriteList  = spriteList;  // Sprite Manager
    this->view        = view;        // UI: The Tree view visualizing the sprites that the user can select them from
    this->levels      = levels;

    disableUpdates = false;

    // Setup List View
    view->setModel(new QStandardItemModel()); // Note models should not be stored.

    // Track all changes to the selection
    connect(view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(itemSelected()));

    // Connect selections in the sprite list to the section
    connect(spriteList, SIGNAL(spriteSelected(QString, int)), this, SLOT(setSprite(QString, int)));
}

SpriteSection::~SpriteSection()
{
    //delete model;
}


// Generate Model Data
void SpriteSection::generateEntries()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    model->clear();
    QStandardItem *parentItem = model->invisibleRootItem();
    int sectionID = 0;

    foreach (SpriteSectionEntry entry, *sectionList)
    {
        calculateDensity(&(*sectionList)[sectionID++]);
        QStandardItem *item = new QStandardItem(entry.name);

        // Lookup Sprites Attached To Each Section
        foreach (SpriteEntry sprite, entry.sprites)
            item->appendRow(new QStandardItem(spriteList->getName(sprite.type)));

        parentItem->appendRow(item);
    }

    view->resizeColumnToContents(0);
    emit setInsertSecBut(sectionList->size() < 255);
    clearSelection();
}

void SpriteSection::generateEntries(QList<QString> sectionNames, QList<QString> spriteNames)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    model->clear();
    QStandardItem *parentItem = model->invisibleRootItem();

    int sectionID = 0;
    int spriteID  = 0;

    foreach (SpriteSectionEntry entry, *sectionList)
    {
        calculateDensity(&(*sectionList)[sectionID]);
        QStandardItem *item = new QStandardItem(sectionNames.at(sectionID++));

        // Lookup Sprites Attached To Each Section
        for (int i = 0; i < entry.sprites.length(); i++)
            item->appendRow(new QStandardItem(spriteNames.at(spriteID++)));

        parentItem->appendRow(item);
    }
    emit setInsertSecBut(sectionList->size() < 255);
    clearSelection();
}

void SpriteSection::clearSelection()
{
    view->selectionModel()->clear();
    updateSprProps(NULL);
    emit updateDensity(0);
    emit updateDensityL(false);
    spriteList->clearEntry();
}

QList<SpriteSectionEntry>* SpriteSection::getSectionList()
{
    return sectionList;
}

QString SpriteSection::getSectionName(int sectionIndex)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();

    if (sectionIndex < model->rowCount())
        return model->item(sectionIndex)->text();
    else
        return QString("invalid section index");
}

void SpriteSection::setSectionName(int sectionIndex, QString text)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();

    if (sectionIndex < model->rowCount())
        model->item(sectionIndex)->setText(text);
}

QString SpriteSection::getSpriteName(int sectionIndex, int spriteIndex)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();

    if (sectionIndex < model->rowCount())
    {
        QStandardItem* parent = model->item(sectionIndex);
        if (spriteIndex < parent->rowCount())
            return parent->child(spriteIndex)->text();
    }

    return QString("invalid sprite index");
}

void SpriteSection::setSpriteName(int sectionIndex, int spriteIndex, QString text)
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    if (sectionIndex < model->rowCount())
    {
        QStandardItem* parent = model->item(sectionIndex);
        if (spriteIndex < parent->rowCount())
            parent->child(spriteIndex)->setText(text);
    }
}

// Select specific bit of scenery
void SpriteSection::itemSelected(int index, bool expand)
{
    QModelIndex mi = view->model()->index(index, 0);
    view->setCurrentIndex(mi);

    if (expand)
        view->expand(mi);
}

void SpriteSection::itemSelected()
{
    disableUpdates = true;

    // Deselect any previous selections
    for (int i = 0; i < sectionList->length(); i++)
    {
        if ((*sectionList)[i].selected)
        {
           (*sectionList)[i].selected = false;

            QList<SpriteEntry>* se = &(*sectionList)[i].sprites;

            for (int j = 0; j < se->length(); j++)
            {
                if ((*se)[j].selected)
                    (*se)[j].selected = false;
            }
        }
    }

    SpriteSectionEntry* currentSection = getCurrentSection();
    SpriteEntry* currentSprite = getCurrentSprite();

    // Clicked on an individual sprite entry
    if (currentSprite != NULL)
    {
        currentSection->selected = true;
        currentSprite->selected  = true;
        spriteList->setEntry(currentSprite->type, currentSprite->props & 1, currentSprite->pal);
        emit updateSprProps(currentSprite);
    }
    // Section Heading
    else
    {
        if (currentSection != NULL)
        {
            //std::cout << "Setting current section on " << std::endl;
            currentSection->selected = true;

            for (int i = 0; i < currentSection->sprites.length(); i++)
                currentSection->sprites[i].selected = true;
        }
        spriteList->clearEntry();
        emit updateSprProps(NULL);
    }

    // Clicked on section heading
    if (currentSection != NULL)
    {
        updateFrequency(currentSection->frequency);
        emit updateDensity(currentSection->density);
        emit updateDensityL(currentSection->density > SpriteSectionEntry::DENSITY_MAX);
    }
    else
    {
        emit updateDensity(0);
        emit updateDensityL(false);
    }

    emit refreshPreview();
    disableUpdates = false;
}

// ----------------------------------------------------------------------------
// Calculate the populated density of a scenery pattern
//
// The OutRun engine can only draw a finite number of sprites on the screen.
// This calculates the sprite density which consists of:
//
// - The frequency value
// - Whether shadows are enabled
// ----------------------------------------------------------------------------

void SpriteSection::calculateDensity(SpriteSectionEntry* currentSection)
{
    // Calculate Density
    uint16_t freq = currentSection->frequency;
    int density = 0;
    int spriteIndex = currentSection->sprites.length() - 1;

    for (int i = 0; i < 64; i++)
    {
        const int draw = freq & 0x8000;
        freq = ((freq << 1) | ((freq & 0x8000) >> 15)) & 0xFFFF;

        if (draw)
        {
            if (currentSection->sprites[spriteIndex].props & 2) // additional shadow
                density += 2;
            else
                density++;

            if (--spriteIndex < 0)
                spriteIndex = currentSection->sprites.length() - 1;
        }
    }
    currentSection->density = density;
    //std::cout << "Density is " << density << std::endl;
}

void SpriteSection::recalculateDensity(SpriteSectionEntry *currentSection)
{
    calculateDensity(currentSection);
    emit updateDensity(currentSection->density);
    emit updateDensityL(currentSection->density > SpriteSectionEntry::DENSITY_MAX);
}

bool SpriteSection::isSectionSelected()
{
    QModelIndex index = view->selectionModel()->currentIndex();
    return index.isValid();
}

SpriteSectionEntry* SpriteSection::getCurrentSection()
{
    QModelIndex index = view->selectionModel()->currentIndex();

    if (!index.isValid())
        return NULL;

    QModelIndex parent = index.parent();

    return &(*sectionList)[parent.isValid() ? parent.row() : index.row()];
}

SpriteEntry* SpriteSection::getCurrentSprite()
{
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    if (parent.isValid() /*&& index.row() < sectionList->at(parent.row()).sprites.size()*/)
    {
        //std::cout << parent.row() << ":" << index.row() << std::endl;
        return &(*sectionList)[parent.row()].sprites[index.row()];
    }
    else
        return NULL;
}

int SpriteSection::getCurrentSectionID()
{
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    return parent.isValid() ? parent.row() : index.row();
}

int SpriteSection::getCurrentSpriteID()
{
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    if (parent.isValid() /*&& index.row() < sectionList->at(parent.row()).sprites.size()*/)
        return index.row();
    else
        return -1;
}

// ------------------------------------------------------------------------------------------------
// BUTTONS
// ------------------------------------------------------------------------------------------------

// Create either a new sprite entry or a complete selection depending on what was selected
void SpriteSection::newEntry()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    const int prow = parent.row();
    const int irow = index.row();

    SpriteEntry newSpriteEntry;
    newSpriteEntry.pal   = 0;
    newSpriteEntry.props = 0;
    newSpriteEntry.type  = 0;
    newSpriteEntry.x     = 0;
    newSpriteEntry.y     = 0;

    // Create new sprite entry
    if (parent.isValid())
    {
        QStandardItem* itemParent = model->item(prow);
        QStandardItem* newSprite = new QStandardItem("New Object");
        itemParent->insertRow(irow+1, newSprite);
        (*sectionList)[prow].sprites.insert(irow+1, newSpriteEntry);
        recalculateDensity(getCurrentSection());

        // Select the entry we've just inserted
        QModelIndex mi = view->model()->index(irow+1, 0, view->model()->index(prow, 0));
        view->setCurrentIndex(mi);
    }
    else
    {
        SpriteSectionEntry newSectionEntry;
        newSectionEntry.frequency = 0xCCCC;
        newSectionEntry.sprites.push_front(newSpriteEntry);
        recalculateDensity(&newSectionEntry);

        QStandardItem* newSection = new QStandardItem("New Section");
        newSection->appendRow(new QStandardItem("New Object"));

        model->insertRow(irow+1, newSection);
        sectionList->insert(irow+1, newSectionEntry);
        levels->remapScenerySections(irow+1, 1);
        emit setInsertSecBut(sectionList->size() < 255);

        // Select the entry we've just inserted
        QModelIndex mi = view->model()->index(irow+1, 0);
        view->setCurrentIndex(mi);
    }
}

// Delete either a sprite entry or a complete section depending on what was selected
void SpriteSection::deleteEntry()
{
    // Delete entry from model
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    const int prow = parent.row();
    const int irow = index.row();

    // Delete entry from model
    QAbstractItemModel *model = view->model();
    model->removeRow(irow, index.parent());

    // Remove sprite from internal list
    if (parent.isValid())
    {
        (*sectionList)[prow].sprites.removeAt(irow);
        recalculateDensity(getCurrentSection());
    }
    // Remove section from internal list
    else
    {
        sectionList->removeAt(irow);

        // Remap existing sections in sprite layout
        levels->remapScenerySections(irow, 0);
        emit setInsertSecBut(sectionList->size() < 255);
    }

    emit refreshPreview();
}

// Duplicate either a sprite entry or a complete section depending on what was selected
void SpriteSection::duplicateEntry()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    const int prow = parent.row();
    const int irow = index.row();

    // Duplicate Sprite Entry
    if (parent.isValid())
    {
        SpriteEntry currentSprite = (*sectionList)[prow].sprites.at(irow);
        QStandardItem* itemParent = model->item(prow);
        QStandardItem* oldSprite = itemParent->child(irow);
        QStandardItem* newSprite = new QStandardItem(oldSprite->text());
        itemParent->insertRow(irow+1, newSprite);
        (*sectionList)[prow].sprites.insert(irow+1, currentSprite);
        recalculateDensity(getCurrentSection());
    }
    // Duplicate Section
    else
    {
        SpriteSectionEntry currentSection = sectionList->at(irow);
        QStandardItem* oldSection = model->item(irow);
        QStandardItem* newSection = new QStandardItem(QString("Copy of " + oldSection->text()));

        // Lookup Sprites Attached To Each Section
        for (int i = 0; i < oldSection->rowCount(); i++)
        {
            newSection->appendRow(new QStandardItem(oldSection->child(i)->text()));
        }

        // Duplicate Section Entries
        model->insertRow(irow+1, newSection);
        sectionList->insert(irow+1, currentSection);
        levels->remapScenerySections(irow+1, 1);
        emit setInsertSecBut(sectionList->size() < 255);
    }
}

void SpriteSection::moveUp()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    const int prow = parent.row();
    const int irow = index.row();

    // Can't move up if already in top position
    if (irow <= 0)
        return;

    // Move Sprite Up
    if (parent.isValid())
    {
        QStandardItem* itemParent = model->item(prow);
        QStandardItem* swap1 = itemParent->takeChild(irow, 0);
        QStandardItem* swap2 = itemParent->takeChild(irow-1, 0);

        itemParent->setChild(irow, swap2);
        itemParent->setChild(irow-1, swap1);

        (*sectionList)[prow].sprites.swapItemsAt(irow, irow-1);
         view->selectionModel()->setCurrentIndex(swap1->index(), QItemSelectionModel::ClearAndSelect);
    }
    // Move Section Up
    else
    {
        QStandardItem* itemParent = model->invisibleRootItem();
        QStandardItem* swap1 = itemParent->takeChild(irow, 0);
        QStandardItem* swap2 = itemParent->takeChild(irow-1, 0);

        itemParent->setChild(irow, swap2);
        itemParent->setChild(irow-1, swap1);

        sectionList->swapItemsAt(irow, irow-1);
        levels->swapScenerySections(irow, irow-1);
        view->selectionModel()->setCurrentIndex(swap1->index(), QItemSelectionModel::ClearAndSelect);
    }
}

void SpriteSection::moveDown()
{
    QStandardItemModel* model = (QStandardItemModel*) view->model();
    QModelIndex index = view->selectionModel()->currentIndex();
    QModelIndex parent = index.parent();

    const int prow = parent.row();
    const int irow = index.row();

    // Move Sprite Down
    if (parent.isValid())
    {
        QStandardItem* itemParent = model->item(prow);
        if (irow < itemParent->rowCount() - 1)
        {
            QStandardItem* swap1 = itemParent->takeChild(irow, 0);
            QStandardItem* swap2 = itemParent->takeChild(irow+1, 0);

            itemParent->setChild(irow, swap2);
            itemParent->setChild(irow+1, swap1);

            (*sectionList)[prow].sprites.swapItemsAt(irow, irow+1);
             view->selectionModel()->setCurrentIndex(swap1->index(), QItemSelectionModel::ClearAndSelect);
        }
    }
    // Move Section Down
    else
    {
        QStandardItem* itemParent = model->invisibleRootItem();
        if (irow < itemParent->rowCount() - 1)
        {
            QStandardItem* swap1 = itemParent->takeChild(irow, 0);
            QStandardItem* swap2 = itemParent->takeChild(irow+1, 0);

            itemParent->setChild(irow, swap2);
            itemParent->setChild(irow+1, swap1);

            sectionList->swapItemsAt(irow, irow+1);
            levels->swapScenerySections(irow, irow+1);
            view->selectionModel()->setCurrentIndex(swap1->index(), QItemSelectionModel::ClearAndSelect);
        }
    }
}

void SpriteSection::setFrequency(int freq)
{
    if (disableUpdates) return;

    SpriteSectionEntry* currentSection = getCurrentSection();

    if (currentSection != NULL)
    {
        currentSection->frequency = freq;
        recalculateDensity(getCurrentSection());
    }
}

void SpriteSection::setShadow(bool enabled)
{
    if (disableUpdates) return;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {
        currentSprite->props = (currentSprite->props & 0xFD) + (enabled ? 2 : 0);
        recalculateDensity(getCurrentSection());
        emit refreshPreview();
    }
}

void SpriteSection::setHFlip(bool enabled)
{
    if (disableUpdates) return;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {
        currentSprite->props = (currentSprite->props & 0xFE) + (enabled ? 1 : 0);
        spriteList->setEntry(currentSprite->type, currentSprite->props & 1, currentSprite->pal);
        emit refreshPreview();
    }
}

// Update Sprite Entry In Section From Sprite List Selector
void SpriteSection::setSprite(QString name, int type)
{
    //if (disableUpdates) return;
    disableUpdates = true;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {
        QStandardItemModel* model = (QStandardItemModel*) view->model();
        QStandardItem *section    = model->invisibleRootItem()->child(getCurrentSectionID());
        QStandardItem *sprite     = section->child(getCurrentSpriteID());
        sprite->setText(name);

        // Update Sprite Entry in section with defaults (palette, draw routine)
        const SpriteFormat *listEntry = &spriteList->getList()->at(type);
        currentSprite->type  = type;
        currentSprite->pal   = listEntry->defaultPal;
        currentSprite->props = (currentSprite->props & 0x0D) + (listEntry->defaultRoutine << 4) + listEntry->defaultShadow;
        recalculateDensity(getCurrentSection());

        emit updateSprProps(currentSprite);
        emit refreshPreview();
    }

    disableUpdates = false;
}

void SpriteSection::setSpriteX(int x)
{
    if (disableUpdates) return;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {
        currentSprite->x = (int8_t) x;
        emit refreshPreview();
    }
}

void SpriteSection::setSpriteY(int y)
{
    if (disableUpdates) return;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {
        currentSprite->y = (int16_t) y;
        emit refreshPreview();
    }
}

void SpriteSection::setSpriteP(int pal)
{
    if (disableUpdates) return;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {
        currentSprite->pal = pal;
        spriteList->setEntry(currentSprite->type, currentSprite->props & 1, currentSprite->pal);
        emit refreshPreview();
    }
}

void SpriteSection::setDrawRoutine(int routine)
{
    if (disableUpdates) return;

    SpriteEntry* currentSprite = getCurrentSprite();

    if (currentSprite != NULL)
    {        
        currentSprite->props = (currentSprite->props & 0xF) + (routine << 4);

        // OutRun specific fix. Sprites that use draw routine 2 are hard-coded to use palette 205 in the game code.
        if (routine == 2)
        {
            currentSprite->pal = 205;
            spriteList->setEntry(currentSprite->type, currentSprite->props & 1, currentSprite->pal);
        }

        emit refreshPreview();
    }
}
