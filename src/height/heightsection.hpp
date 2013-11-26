/***************************************************************************
    Height Sections Representation.
    Height Sections are shared across all levels in a project.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef HEIGHTSECTION_HPP
#define HEIGHTSECTION_HPP

#include <QObject>

#include "heightlabels.hpp"
#include "../stdint.hpp"
#include "../leveldata.hpp"

class QListView;
class Levels;

class HeightSection : public QObject
{
    Q_OBJECT
public:
    QListView* view;

    explicit HeightSection(QObject *parent, QList<HeightSegment>* sectionList, QListView* view, Levels* levels);
    void generate();
    HeightSegment* getCurrentSection();
    QList<HeightSegment>* getSectionList();
    QString getSectionName(int);
    void setSectionName(int, QString);
    
signals:
    void updateSection(HeightSegment*);
    void refreshPreview();
    void refreshWidget();       // Refresh the Widget Representing The Height Section
    void sendNewPoint(int);
    void setDeletePntBut(bool); // Enable Delete Point Button
    void setDeleteSecBut(bool); // Enable Delete Section Button
    void setInsertSecBut(bool); // Enable Insert/Duplicate Section Button
    
public slots:
    void setSection(int);
    void setStretch(int);
    void setStretchUp(int);
    void setStretchDown(int);
    void setDelay(int);
    void setHeightPoint(int);
    void insertHeightPoint();
    void deleteHeightPoint();
    void newEntry(int);
    void deleteEntry();
    void duplicateEntry();
    void moveUp();
    void moveDown();

private slots:
    void setCurrentSection();

private:
    // Height Section List
    QList<HeightSegment>* sectionList;

    // Reference to complete list of levels. Height points are shared between levels.
    Levels* levels; 

    // Height Point Selected
    int heightPoint;
};

#endif // HEIGHTSECTION_HPP
