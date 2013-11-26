/***************************************************************************
    Sprite List

    - Contains a list of all available sprites and their rendering
      properties.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef SPRITELIST_HPP
#define SPRITELIST_HPP

#include <QList>
#include <QListView>
#include "../stdint.hpp"
#include "spriteformat.hpp"

class SpritePreviewWidget;

class SpriteList : public QObject
{
Q_OBJECT

public:
    int length;
    explicit SpriteList(QObject *parent, SpritePreviewWidget* sr, QListView *listView);
    void setList(QList<SpriteFormat> list);
    void setEntry(int spriteIndex, bool flip, int pal);
    void setEntry(int spriteIndex);
    void clearEntry();
    QString getName(int);
    QList <SpriteFormat>* getList();

signals:
    void spriteSelected(QString, int);

public slots:
    void toggleHiddenSprites(bool enabled);

private slots:
    void itemSelected();

private:
    SpritePreviewWidget* previewWidget;
    QList <SpriteFormat> list;
    QListView* view;

};

#endif // SPRITELIST_HPP
