/***************************************************************************
    Sprite List

    - Contains a list of all available sprites and their rendering
      properties.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QStandardItemModel>
#include "spritelist.hpp"
#include "previewwidget.hpp"

SpriteList::SpriteList(QObject *parent, SpritePreviewWidget* sr, QListView* view) :
    QObject(parent)
{
    this->view    = view;
    previewWidget = sr;

    // Track all changes to the selection
    //connect(view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(itemSelected()));
    connect(view, SIGNAL(activated(QModelIndex)), this, SLOT(itemSelected()));
    connect(view, SIGNAL(clicked(QModelIndex)),   this, SLOT(itemSelected()));

    clearEntry();
}

void SpriteList::setList(QList<SpriteFormat> list)
{
    this->list = list;
    length = list.size();

    // Setup List View
    QStandardItemModel* model = new QStandardItemModel();
    for (int i = 0; i < length; i++)
    {
        QStandardItem *item = new QStandardItem(list.at(i).name);
        model->appendRow(item);
    }
    view->setModel(model);
}

// Get Name of Sprite At Index
QString SpriteList::getName(int index)
{
    return list.at(index).name;
}

QList <SpriteFormat>* SpriteList::getList()
{
    return &list;
}

void SpriteList::setEntry(int spriteIndex, bool flip, int pal)
{
    SpriteFormat entry = list.at(spriteIndex);
    previewWidget->setSprite(entry.bank, entry.offset, entry.width, entry.height, flip, pal);

    // Update Sprite List UI to select the sprite chosen.
    view->setCurrentIndex(view->model()->index(spriteIndex, 0));
}

// Version to not update the flip, and use the default palette
void SpriteList::setEntry(int spriteIndex)
{
    SpriteFormat entry = list.at(spriteIndex);
    previewWidget->setSprite(entry.bank, entry.offset, entry.width, entry.height, entry.defaultPal);

    // Update Sprite List UI to select the sprite chosen.
    view->setCurrentIndex(view->model()->index(spriteIndex, 0));
}

void SpriteList::clearEntry()
{
    previewWidget->clearSprite();
}

void SpriteList::itemSelected()
{
    QModelIndex index = view->selectionModel()->currentIndex();
    const int id = index.row();

    // Setup Preview Window
    setEntry(id);

    // Update Segment with new Sprite
    QStandardItemModel* model = (QStandardItemModel*) index.model();
    emit spriteSelected(model->invisibleRootItem()->child(id)->text(), id);
}

// Toggle sprites that are predefined as hideable.
void SpriteList::toggleHiddenSprites(bool hide)
{
    int row = 0;
    foreach(SpriteFormat spr, list)
    {
        if (spr.canBeHidden)
        {
            view->setRowHidden(row, hide);
        }
        row++;
    }
}
