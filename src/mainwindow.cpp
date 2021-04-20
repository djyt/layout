/***************************************************************************
    Layout: A Track Editor for OutRun
    - Main Window Handler

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QGuiApplication>  // List of screens
#include <QScreen>
#include <QFileDialog>      // For file selection
#include <QStringList>      // For file selection
#include <QDir>             // For file selection
#include <QTextStream>      // Licensing dialog box
#include <QSignalMapper>
#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopServices> // URL Handling
#include <QUrl>

#include "generatexml.hpp"
#include "height/heightsection.hpp"
#include "height/heightwidget.hpp"
#include "sprites/spritelist.hpp"
#include "sprites/spritesection.hpp"
#include "sprites/sprite.hpp"
#include "levels/levels.hpp"
#include "import/importoutrun.hpp"
#include "export/exportcannonball.hpp"
#include "import/importdialog.hpp"
#include "settings/settingsdialog.hpp"
#include "about/about.hpp"
#include "levelpalettewidget/levelpalettewidget.hpp"
#include "utils.hpp"

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->parent    = parent;
    externalProcess = NULL;
    aboutDialog     = NULL;
    roadPalette     = new LevelPalette();
    exportCannon    = new ExportCannonball();
    importOutRun    = new ImportOutRun();
    importDialog    = new ImportDialog(this, "Import Level", importOutRun->getLevelNames(), true);
    settingsDialog  = new SettingsDialog(this);
    settings        = new QSettings("Reassembler", "LayOut");

    // --------------------------------------------------------------------------------------------
    // UI Setup
    // --------------------------------------------------------------------------------------------

    // Do this before other UI operations
    ui->setupUi(this);

    // Add additional tabs
    roadPaletteWidget = new LevelPaletteWidget(this, roadPalette);
    ui->editModeTabs->addTab(roadPaletteWidget, "Palette");

    levels = new Levels(this, roadPalette);
    levels->init();
    levels->newLevel();
    levels->newEndSection();
    levels->selectFirstLevel();
    ui->tabMain->addTab(levels, "Levels");

    ui->densityBar->setRange(0, 128);

    // Setup Path Ranges
    ui->lengthSpin->setRange (1, LevelData::SECTION_LENGTH_MAX);
    ui->lengthSlide->setRange(1, LevelData::SECTION_LENGTH_MAX);
    ui->angleSpin->setRange  (-LevelData::SECTION_ANGLE_MAX, LevelData::SECTION_ANGLE_MAX);
    ui->angleSlide->setRange (-LevelData::SECTION_ANGLE_MAX, LevelData::SECTION_ANGLE_MAX);

    // Setup Width Ranges
    ui->widthSpin-> setRange (0, 500);
    ui->widthSlide->setRange (0, 500);
    ui->speedSpin-> setRange (1, 64);
    ui->speedSlide->setRange (1, 64);

    // Setup Scenery Ranges
    ui->patternSpin->setRange (0, 255);
    ui->patternSlide->setRange(0, 255);

    // Setup Height Control Defaults
    ui->heightValueGroup->setEnabled(false);
    ui->heightPointsGroup->setEnabled(false);

    ui->heightCombo->setItemText(0, HEIGHT_LABELS[0]);
    ui->heightCombo->setItemText(1, HEIGHT_LABELS[1]);
    ui->heightCombo->setItemText(2, HEIGHT_LABELS[2]);
    ui->heightCombo->setItemText(3, HEIGHT_LABELS[3]);
    ui->heightCombo->setItemText(4, HEIGHT_LABELS[4]);

    // Setup Horizon Sections
    heightSection = new HeightSection(this, &heightSections, ui->heightListView, levels);
    ui->heightList->setModel(heightSection->view->model());

    // Setup Sprite Viewer
    spriteList = new SpriteList(this, ui->spritePreviewWidget, ui->spriteSelectList);
    spriteSection = new SpriteSection(this,
                                      &spriteSections,
                                      spriteList,
                                      ui->spriteTreeView,
                                      levels);
    ui->patternList->setModel(ui->spriteTreeView->model());

    setSpriteProps(NULL);

    // --------------------------------------------------------------------------------------------
    // Setup S16 Preview Window and connected widgets
    // --------------------------------------------------------------------------------------------
    ui->spinCameraX->setRange (RenderS16::CAMERA_X_MIN, RenderS16::CAMERA_X_MAX);
    ui->spinCameraY->setRange (RenderS16::CAMERA_Y_MIN, RenderS16::CAMERA_Y_MAX);

    // Setup Road Path Widget
    ui->roadPathWidget->setSpriteSection(spriteSection);
    ui->roadPathWidget->setHeightSection(heightSection);

    connect(ui->roadPathWidget,   SIGNAL(refreshPreview()),           ui->RenderS16Widget,      SLOT(redrawPos()));
    connect(ui->roadPathWidget,   SIGNAL(refreshPreview(int)),        ui->spinPosition,         SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(refreshPreview(int)),        ui->RenderS16Widget,      SLOT(setRoadPos(int)));
    connect(ui->roadPathWidget,   SIGNAL(setEndPos(int)),             this,                     SLOT(setPositionRange(int)));
    connect(ui->roadPathWidget,   SIGNAL(changeZoom(int)),            this,                     SLOT(setZoom(int)));

    connect(ui->RenderS16Widget,  SIGNAL(sendNewPosition(int)),       ui->spinPosition,         SLOT(setValue(int)));
    connect(ui->RenderS16Widget,  SIGNAL(sendCameraX(int)),           ui->spinCameraX,          SLOT(setValue(int)));
    connect(ui->RenderS16Widget,  SIGNAL(sendCameraY(int)),           ui->spinCameraY,          SLOT(setValue(int)));
    connect(ui->spinCameraX,      SIGNAL(valueChanged(int)),          ui->RenderS16Widget,      SLOT(setCameraX(int)));
    connect(ui->spinCameraY,      SIGNAL(valueChanged(int)),          ui->RenderS16Widget,      SLOT(setCameraY(int)));

    QSignalMapper* resetXMapper = new QSignalMapper(this);
    QSignalMapper* resetYMapper = new QSignalMapper(this);
    connect(ui->buttonResetX,SIGNAL(clicked()), resetXMapper, SLOT(map()));
    connect(ui->buttonResetY,SIGNAL(clicked()), resetYMapper, SLOT(map()));
    resetXMapper->setMapping(ui->buttonResetX, 0);
    resetYMapper->setMapping(ui->buttonResetY, 0);
    connect(resetXMapper, SIGNAL(mapped(int)), ui->spinCameraX, SLOT(setValue(int)));
    connect(resetYMapper, SIGNAL(mapped(int)), ui->spinCameraY, SLOT(setValue(int)));

    connect(ui->slidePosition,    SIGNAL(valueChanged(int)),          ui->spinPosition,         SLOT(setValue(int)));
    connect(ui->spinPosition,     SIGNAL(valueChanged(int)),          ui->slidePosition,        SLOT(setValue(int)));
    connect(ui->spinPosition,     SIGNAL(valueChanged(int)),          ui->RenderS16Widget,      SLOT(setRoadPos(int)));
    connect(ui->spinPosition,     SIGNAL(valueChanged(int)),          ui->roadPathWidget,       SLOT(setRoadPos(int)));
    connect(ui->checkScenery,     SIGNAL(toggled(bool)),              ui->RenderS16Widget,      SLOT(setSceneryGuides(bool)));
    connect(ui->comboGuidelines,  SIGNAL(currentIndexChanged(int)),   ui->RenderS16Widget,      SLOT(setGuidelines(int)));

    // --------------------------------------------------------------------------------------------
    // Road Path Tabs
    // --------------------------------------------------------------------------------------------
    connect(ui->editModeTabs,     SIGNAL(currentChanged(int)),        ui->roadPathWidget,       SLOT(setView(int)));

    connect(ui->insertAfter,      SIGNAL(clicked()),                  ui->roadPathWidget,       SLOT(insertPointAfter()));
    connect(ui->insertBefore,     SIGNAL(clicked()),                  ui->roadPathWidget,       SLOT(insertPointBefore()));
    connect(ui->roadPathWidget,   SIGNAL(enableControls(bool)),       this,                     SLOT(toggleControls(bool)));
    connect(ui->roadPathWidget,   SIGNAL(enableInsert(bool)),         ui->insertAfter,          SLOT(setEnabled(bool)));
    connect(ui->roadPathWidget,   SIGNAL(enableInsert(bool)),         ui->insertBefore,         SLOT(setEnabled(bool)));
    connect(ui->lengthSpin,       SIGNAL(valueChanged(int)),          ui->lengthSlide,          SLOT(setValue(int)));
    connect(ui->angleSpin,        SIGNAL(valueChanged(int)),          ui->angleSlide,           SLOT(setValue(int)));
    connect(ui->lengthSlide,      SIGNAL(valueChanged(int)),          ui->lengthSpin,           SLOT(setValue(int)));
    connect(ui->angleSlide,       SIGNAL(valueChanged(int)),          ui->angleSpin,            SLOT(setValue(int)));
    connect(ui->speedSpin,        SIGNAL(valueChanged(int)),          ui->speedSlide,           SLOT(setValue(int)));
    connect(ui->widthSpin,        SIGNAL(valueChanged(int)),          ui->widthSlide,           SLOT(setValue(int)));
    connect(ui->speedSlide,       SIGNAL(valueChanged(int)),          ui->speedSpin,            SLOT(setValue(int)));
    connect(ui->widthSlide,       SIGNAL(valueChanged(int)),          ui->widthSpin,            SLOT(setValue(int)));

    connect(ui->lengthSpin,       SIGNAL(valueChanged(int)),          ui->roadPathWidget,       SLOT(setLength(int)));
    connect(ui->angleSpin,        SIGNAL(valueChanged(int)),          ui->roadPathWidget,       SLOT(setAngle(int)));
    connect(ui->widthSpin,        SIGNAL(valueChanged(int)),          ui->roadPathWidget,       SLOT(setWidth(int)));
    connect(ui->speedSpin,        SIGNAL(valueChanged(int)),          ui->roadPathWidget,       SLOT(setWidthChange(int)));

    connect(ui->roadPathWidget,   SIGNAL(changeLength(int)),          ui->lengthSpin,           SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(changeAngle(int)),           ui->angleSpin,            SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(changeWidth(int)),           ui->widthSpin,            SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(changeWidthSpeed(int)),      ui->speedSpin,            SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(changePatternLength(int)),   ui->patternSpin,          SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(setPercentage(int)),         ui->progressBar,          SLOT(setValue(int)));

    // --------------------------------------------------------------------------------------------
    // Road Palette Setup
    // --------------------------------------------------------------------------------------------

    connect(roadPaletteWidget,    SIGNAL(refreshPalette()),           ui->RenderS16Widget,      SLOT(setupRoadPalettes()));
    connect(roadPaletteWidget,    SIGNAL(refreshPreview()),           ui->RenderS16Widget,      SLOT(redrawPos()));

    // --------------------------------------------------------------------------------------------
    // Height Pattern Editor
    // --------------------------------------------------------------------------------------------
    connect(ui->heightButton,     SIGNAL(clicked()),                  this,                     SLOT(editHeightPattern()));
    connect(ui->spinHeightValue,  SIGNAL(valueChanged(int)),          ui->slideHeightValue,     SLOT(setValue(int)));
    connect(ui->spinHeightValue,  SIGNAL(valueChanged(int)),          this,                     SLOT(setHeightPoint(int)));
    connect(ui->slideHeightValue, SIGNAL(valueChanged(int)),          ui->spinHeightValue,      SLOT(setValue(int)));
    connect(ui->roadPathWidget,   SIGNAL(selectHeightSection(int)),   this,                     SLOT(selectHeightPattern(int)));
    connect(ui->spinHeightStretch,SIGNAL(valueChanged(int)),          heightSection,            SLOT(setStretch(int)));
    connect(ui->spinHeightUp,     SIGNAL(valueChanged(int)),          heightSection,            SLOT(setStretchUp(int)));
    connect(ui->spinHeightDown,   SIGNAL(valueChanged(int)),          heightSection,            SLOT(setStretchDown(int)));
    connect(ui->spinHeightDelay,  SIGNAL(valueChanged(int)),          heightSection,            SLOT(setDelay(int)));
    connect(ui->buttonHPDelete,   SIGNAL(clicked()),                  heightSection,            SLOT(deleteHeightPoint()));
    connect(ui->buttonHPInsert,   SIGNAL(clicked()),                  heightSection,            SLOT(insertHeightPoint()));
    connect(ui->heightNew,        SIGNAL(clicked()),                  this,                     SLOT(newHeight()));
    connect(ui->heightDelete,     SIGNAL(clicked()),                  heightSection,            SLOT(deleteEntry()));
    connect(ui->heightDupe,       SIGNAL(clicked()),                  heightSection,            SLOT(duplicateEntry()));
    connect(ui->heightUp,         SIGNAL(clicked()),                  heightSection,            SLOT(moveUp()));
    connect(ui->heightDown,       SIGNAL(clicked()),                  heightSection,            SLOT(moveDown()));
    connect(heightSection,        SIGNAL(setInsertSecBut(bool)),      ui->heightNew,            SLOT(setEnabled(bool)));
    connect(heightSection,        SIGNAL(setInsertSecBut(bool)),      ui->heightDupe,           SLOT(setEnabled(bool)));
    connect(heightSection,        SIGNAL(setDeletePntBut(bool)),      ui->buttonHPDelete,       SLOT(setEnabled(bool)));
    connect(heightSection,        SIGNAL(setDeleteSecBut(bool)),      ui->heightDelete,         SLOT(setEnabled(bool)));
    connect(heightSection,        SIGNAL(updateSection(HeightSegment*)),this,                   SLOT(setHeightSection(HeightSegment*)));
    connect(heightSection,        SIGNAL(updateSection(HeightSegment*)),ui->heightWidget,       SLOT(setSegment(HeightSegment*)));
    connect(heightSection,        SIGNAL(refreshPreview()),           ui->RenderS16Widget,      SLOT(redrawPos()));
    connect(heightSection,        SIGNAL(refreshWidget()),            ui->heightWidget,         SLOT(createSegment()));
    connect(heightSection,        SIGNAL(sendNewPoint(int)),          ui->heightWidget,         SLOT(selectPoint(int)));
    connect(ui->heightWidget,     SIGNAL(pointChanged(int)),          this,                     SLOT(selectHeightPoint(int)));
    connect(ui->heightWidget,     SIGNAL(pointChanged(int)),          heightSection,            SLOT(setHeightPoint(int)));
    connect(ui->heightList,       SIGNAL(activated(QModelIndex)),     this,                     SLOT(setHeightPattern()));
    connect(ui->heightList,       SIGNAL(clicked(QModelIndex)),       this,                     SLOT(setHeightPattern()));

    // --------------------------------------------------------------------------------------------
    // Scenery Patterns
    // --------------------------------------------------------------------------------------------
    connect(ui->searchClear,      SIGNAL(clicked()),                  ui->searchScenery,        SLOT(clear()));
    connect(ui->searchScenery,    SIGNAL(textChanged(QString)),       this,                     SLOT(searchSceneryPattern(QString)));
    connect(ui->patternButton,    SIGNAL(clicked()),                  this,                     SLOT(editSceneryPattern()));
    connect(ui->patternSlide,     SIGNAL(valueChanged(int)),          ui->patternSpin,          SLOT(setValue(int)));
    connect(ui->patternSpin,      SIGNAL(valueChanged(int)),          ui->patternSlide,         SLOT(setValue(int)));
    connect(ui->patternSpin,      SIGNAL(valueChanged(int)),          ui->roadPathWidget,       SLOT(setPoint(int)));
    connect(ui->roadPathWidget,   SIGNAL(selectScenerySection(int)),  this,                     SLOT(selectSceneryPattern(int)));
    connect(ui->patternList,      SIGNAL(activated(QModelIndex)),     this,                     SLOT(setSceneryPattern()));
    connect(ui->patternList,      SIGNAL(clicked(QModelIndex)),       this,                     SLOT(setSceneryPattern()));

    // --------------------------------------------------------------------------------------------
    // Scenery Pattern Editor
    // --------------------------------------------------------------------------------------------
    connect(ui->checkHideSprites, SIGNAL(toggled(bool)),              spriteList,               SLOT(toggleHiddenSprites(bool)));
    connect(ui->checkSpriteShadow,SIGNAL(toggled(bool)),              spriteSection,            SLOT(setShadow(bool)));
    connect(ui->checkSpriteHFlip, SIGNAL(toggled(bool)),              spriteSection,            SLOT(setHFlip(bool)));
    connect(ui->spinSpriteX,      SIGNAL(valueChanged(int)),          spriteSection,            SLOT(setSpriteX(int)));
    connect(ui->spinSpriteY,      SIGNAL(valueChanged(int)),          spriteSection,            SLOT(setSpriteY(int)));
    connect(ui->spinSpriteP,      SIGNAL(valueChanged(int)),          spriteSection,            SLOT(setSpriteP(int)));
    connect(ui->spinSpriteP,      SIGNAL(valueChanged(int)),          ui->previewPaletteWidget, SLOT(setPalette(int)));
    connect(ui->previewPaletteWidget, SIGNAL(refreshPreview()),       ui->spritePreviewWidget,  SLOT(recreate()));
    connect(ui->comboRoutine,     SIGNAL(currentIndexChanged(int)),   spriteSection,            SLOT(setDrawRoutine(int)));
    connect(ui->comboRoutine,     SIGNAL(currentIndexChanged(int)),   this,                     SLOT(setPaletteSpin(int)));
    connect(spriteSection,        SIGNAL(refreshPreview()),           ui->RenderS16Widget,      SLOT(redrawPos()));
    connect(spriteSection,        SIGNAL(updateFrequency(int)),       this,                     SLOT(updateFrequencyGroup(int)));
    connect(spriteSection,        SIGNAL(updateSprProps(SpriteEntry*)),this,                    SLOT(setSpriteProps(SpriteEntry*)));
    connect(spriteSection,        SIGNAL(setInsertSecBut(bool)),      ui->sceneNew,             SLOT(setEnabled(bool)));
    connect(spriteSection,        SIGNAL(setInsertSecBut(bool)),      ui->sceneDupe,            SLOT(setEnabled(bool)));
    connect(spriteSection,        SIGNAL(updateDensity(int)),         ui->densityBar,           SLOT(setValue(int)));
    connect(spriteSection,        SIGNAL(updateDensityL(bool)),       ui->densityLabel,         SLOT(setVisible(bool)));
    connect(ui->sceneNew,         SIGNAL(clicked()),                  spriteSection,            SLOT(newEntry()));
    connect(ui->sceneDel,         SIGNAL(clicked()),                  spriteSection,            SLOT(deleteEntry()));
    connect(ui->sceneDupe,        SIGNAL(clicked()),                  spriteSection,            SLOT(duplicateEntry()));
    connect(ui->sceneUp,          SIGNAL(clicked()),                  spriteSection,            SLOT(moveUp()));
    connect(ui->sceneDown,        SIGNAL(clicked()),                  spriteSection,            SLOT(moveDown()));
    connect(ui->checkFreq00,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq01,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq02,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq03,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq04,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq05,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq06,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq07,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq08,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq09,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq10,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq11,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq12,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq13,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq14,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));
    connect(ui->checkFreq15,      SIGNAL(clicked()),                  this,                     SLOT(setFrequency()));

    // --------------------------------------------------------------------------------------------
    // Level Selector
    // --------------------------------------------------------------------------------------------
    connect(levels,               SIGNAL(loadLevel()),                this,                     SLOT(initLevel()));
    connect(levels,               SIGNAL(refreshPreview()),           ui->RenderS16Widget,      SLOT(redrawPos()));


    connect(ui->roadPathWidget,   SIGNAL(toggleLaneControls(bool)),   this,                     SLOT(toggleLaneControls(bool)));
    connect(ui->roadPathWidget,   SIGNAL(setStatusBar(QString)),      ui->statusBar,            SLOT(showMessage(QString)));

    // Connect other dialog boxes
    connect(importDialog,         SIGNAL(outputLevel(int)),           this,                     SLOT(importLevel(int)));
    connect(settingsDialog,       SIGNAL(loadRoms()),                 this,                     SLOT(initRomData()));

    // Setup Width Buttons to snap to a particular number of road lanes
    QSignalMapper* signalMapper = new QSignalMapper(this);
    connect(ui->laneButton1, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(ui->laneButton2, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(ui->laneButton3, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(ui->laneButton4, SIGNAL(pressed()), signalMapper, SLOT(map()));
    connect(ui->laneButton5, SIGNAL(pressed()), signalMapper, SLOT(map()));
    signalMapper->setMapping (ui->laneButton1, 0);
    signalMapper->setMapping (ui->laneButton2, 72);
    signalMapper->setMapping (ui->laneButton3, 142);
    signalMapper->setMapping (ui->laneButton4, 212);
    signalMapper->setMapping (ui->laneButton5, 310);
    signalMapper->setMapping (ui->laneButton5, 310);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(updateWidth(int)));

    xml = new GenerateXML(levels, heightSection, spriteSection);
    loadSettings();
    initRomData();
    on_actionNew_Project_triggered();
}

MainWindow::~MainWindow()
{
    stopExternalProcess();

    delete roadPaletteWidget;
    delete roadPalette;
    delete levels;
    delete spriteList;
    delete exportCannon;
    delete importOutRun;
    delete xml;
    delete settings;
    delete ui;
    delete importDialog;
    delete settingsDialog;
    delete aboutDialog;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    return QWidget::closeEvent(event);
}

// ------------------------------------------------------------------------------------------------
// Slots
// ------------------------------------------------------------------------------------------------

// Called on level switch to a level that already exists.
void MainWindow::initLevel()
{
    // Set spin position based on level length. This varies between normal and split levels.
    ui->spinPosition->setRange(0, levelData->length);

    if (importOutRun->romsLoaded)
    {
        ui->RenderS16Widget->init();
        ui->RenderS16Widget->setRoadPos(0);
    }

    ui->spinPosition->setValue(0);
    roadPaletteWidget->refresh();
    ui->roadPathWidget->init();
    ui->roadPathWidget->setView(ui->editModeTabs->currentIndex()); // also enables insert button correctly
    ui->roadPathWidget->update();
}

void MainWindow::newHeight()
{
    heightSection->newEntry(ui->heightCombo->currentIndex());
}

void MainWindow::selectHeightPattern(int index)
{
    ui->heightList->setCurrentIndex(ui->heightList->model()->index(index, 0));
}

void MainWindow::editHeightPattern()
{
    int selectedIndex = ui->heightList->selectionModel()->currentIndex().row();
    heightSection->setSection(selectedIndex);

    // Update to correct tab
    ui->tabMain->setCurrentIndex(1);
}

void MainWindow::setHeightSection(HeightSegment* section)
{
    ui->spinHeightStretch->setEnabled(true);
    ui->spinHeightStretch->setValue(section->step);

    switch (section->type)
    {
        case 0:
            ui->heightPointsGroup->setEnabled(true);
            ui->spinHeightUp->setEnabled(true);
            ui->spinHeightDown->setEnabled(true);
            ui->spinHeightDelay->setEnabled(false);
            ui->spinHeightUp->setValue(section->value1);
            ui->spinHeightDown->setValue(section->value2);
            break;

        case 1:
        case 2:
        case 3:
            ui->heightPointsGroup->setEnabled(false);
            ui->spinHeightUp->setEnabled(false);
            ui->spinHeightDown->setEnabled(false);
            ui->spinHeightDelay->setEnabled(true);
            ui->spinHeightDelay->setValue(section->value1);
            break;

        case 4:
            ui->heightPointsGroup->setEnabled(false);
            ui->spinHeightUp->setEnabled(false);
            ui->spinHeightDown->setEnabled(false);
            ui->spinHeightDelay->setEnabled(false);
            ui->spinHeightValue->setEnabled(true);
            ui->spinHeightValue->setValue(section->value1 / 32);
            break;
    }
}

void MainWindow::selectHeightPoint(int index)
{
    HeightSegment* section = heightSection->getCurrentSection();

    if (section != NULL && section->type == 4)
    {
       ui->heightValueGroup->setEnabled(true);
    }
    else
    {
        if (index == -1)
        {
            ui->buttonHPDelete->setEnabled(false);
            ui->heightValueGroup->setEnabled(false);
        }
        else
        {
            ui->heightValueGroup->setEnabled(true);
            ui->buttonHPDelete->setEnabled(true);
            ui->spinHeightValue->setValue(section->data.at(index) / 32);
        }
    }
}

void MainWindow::setHeightPoint(int value)
{
    HeightSegment* section = heightSection->getCurrentSection();

    switch (section->type)
    {
        case 0:
        case 3:
            section->data[ui->heightWidget->getSelectedPoint()] = value * 32;
            break;

        case 1:
        case 2:
            section->data[0] = section->data[1] = value * 32;
            break;

        case 4:
            section->value1 = value * 32;
            break;
    }
    ui->heightWidget->createSegment();
    ui->RenderS16Widget->redrawPos();
}

void MainWindow::setHeightPattern()
{
    int selectedIndex = ui->heightList->selectionModel()->currentIndex().row();

    int index = ui->roadPathWidget->getCurrentCP();
    if (index != -1)
    {
        ControlPoint* cp = &levelData->heightP[index];
        cp->value1 = selectedIndex;
        ui->RenderS16Widget->redrawPos();
    }
}

void MainWindow::toggleControls(bool enabled)
{
    ui->lengthSpin->setEnabled(enabled);
    ui->angleSpin->setEnabled(enabled);
    ui->lengthSlide->setEnabled(enabled);
    ui->angleSlide->setEnabled(enabled);
    ui->speedSpin->setEnabled(enabled);
    ui->widthSpin->setEnabled(enabled);
    ui->speedSlide->setEnabled(enabled);
    ui->widthSlide->setEnabled(enabled);
    ui->patternSpin->setEnabled(enabled);
    ui->patternSlide->setEnabled(enabled);
    ui->patternButton->setEnabled(enabled);
    ui->patternList->setEnabled(enabled);
    ui->searchClear->setEnabled(enabled);
    ui->searchScenery->setEnabled(enabled);
    ui->heightList->setEnabled(enabled);
    ui->heightButton->setEnabled(enabled);
}

// Toggle Width Buttons
void MainWindow::toggleLaneControls(bool enabled)
{
    ui->laneButton1->setEnabled(enabled);
    ui->laneButton2->setEnabled(enabled);
    ui->laneButton3->setEnabled(enabled);
    ui->laneButton4->setEnabled(enabled);
    ui->laneButton5->setEnabled(enabled);
}

// Update Width Control From Button
void MainWindow::updateWidth(int newWidth)
{
    ui->widthSpin->setValue(newWidth);
    ui->roadPathWidget->setWidth(newWidth);
}

void MainWindow::updateFrequencyGroup(int freq)
{
    ui->checkFreq00->setChecked(freq & 0x8000);
    ui->checkFreq01->setChecked(freq & 0x4000);
    ui->checkFreq02->setChecked(freq & 0x2000);
    ui->checkFreq03->setChecked(freq & 0x1000);
    ui->checkFreq04->setChecked(freq & 0x800);
    ui->checkFreq05->setChecked(freq & 0x400);
    ui->checkFreq06->setChecked(freq & 0x200);
    ui->checkFreq07->setChecked(freq & 0x100);

    ui->checkFreq08->setChecked(freq & 0x80);
    ui->checkFreq09->setChecked(freq & 0x40);
    ui->checkFreq10->setChecked(freq & 0x20);
    ui->checkFreq11->setChecked(freq & 0x10);
    ui->checkFreq12->setChecked(freq & 0x8);
    ui->checkFreq13->setChecked(freq & 0x4);
    ui->checkFreq14->setChecked(freq & 0x2);
    ui->checkFreq15->setChecked(freq & 0x1);
}

void MainWindow::setFrequency()
{
    int freq = 0;

    if (ui->checkFreq00->isChecked()) freq |= 0x8000;
    if (ui->checkFreq01->isChecked()) freq |= 0x4000;
    if (ui->checkFreq02->isChecked()) freq |= 0x2000;
    if (ui->checkFreq03->isChecked()) freq |= 0x1000;
    if (ui->checkFreq04->isChecked()) freq |= 0x800;
    if (ui->checkFreq05->isChecked()) freq |= 0x400;
    if (ui->checkFreq06->isChecked()) freq |= 0x200;
    if (ui->checkFreq07->isChecked()) freq |= 0x100;
    if (ui->checkFreq08->isChecked()) freq |= 0x80;
    if (ui->checkFreq09->isChecked()) freq |= 0x40;
    if (ui->checkFreq10->isChecked()) freq |= 0x20;
    if (ui->checkFreq11->isChecked()) freq |= 0x10;
    if (ui->checkFreq12->isChecked()) freq |= 0x8;
    if (ui->checkFreq13->isChecked()) freq |= 0x4;
    if (ui->checkFreq14->isChecked()) freq |= 0x2;
    if (ui->checkFreq15->isChecked()) freq |= 0x1;

    spriteSection->setFrequency(freq);
    ui->RenderS16Widget->redrawPos();
}

void MainWindow::setSpriteProps(SpriteEntry* spriteEntry)
{
    if (spriteEntry != NULL)
    {
        ui->spriteSelectList->setEnabled(true);
        ui->spritePositionBox->setEnabled(true);

        ui->checkSpriteHFlip->setChecked(spriteEntry->props & 1);
        ui->checkSpriteShadow->setChecked(spriteEntry->props & 2);
        ui->spinSpriteX->setValue(spriteEntry->x);
        ui->spinSpriteY->setValue(spriteEntry->y);
        ui->spinSpriteP->setValue(spriteEntry->pal);
        ui->comboRoutine->setCurrentIndex((spriteEntry->props >> 4) & 0xF);
        ui->previewPaletteWidget->setPalette(spriteEntry->pal);
        ui->frequencyGroup->setEnabled(true);
    }
    else
    {
        ui->spriteSelectList->setEnabled(false);
        ui->spritePositionBox->setEnabled(false);
        ui->previewPaletteWidget->setPalette(-1);
        ui->frequencyGroup->setEnabled(spriteSection->isSectionSelected());
    }
}

// Disable palette selection for certain routines
// This is a hack to work around an OutRun specific game engine hack where
// cloud sprites are hard-coded to use palette 205.
void MainWindow::setPaletteSpin(int routine)
{
    ui->spinSpriteP->setEnabled(routine != 2);
}

// Write New Scenery Pattern Value
void MainWindow::setSceneryPattern()
{
    int selectedIndex = ui->patternList->selectionModel()->currentIndex().row();

    int cp = ui->roadPathWidget->getCurrentCP();
    if (cp != -1 && selectedIndex != -1)
    {
        ControlPoint* sp = &levelData->spriteP[cp];
        sp->value2 = selectedIndex;
        ui->RenderS16Widget->redrawPos();
    }
}

void MainWindow::editSceneryPattern()
{
    int selectedIndex = ui->patternList->selectionModel()->currentIndex().row();
    spriteSection->itemSelected(selectedIndex);

    // Update to correct tab
    ui->tabMain->setCurrentIndex(2);
}

void MainWindow::searchSceneryPattern(QString searchTerm)
{
    const int rows = ui->spriteTreeView->model()->rowCount();

    for (int i = 0; i < rows; i++)
    {
        ui->patternList->setRowHidden(i, !spriteSection->getSectionName(i).contains(searchTerm, Qt::CaseInsensitive));
    }
}

void MainWindow::selectSceneryPattern(int index)
{
    ui->patternList->setCurrentIndex(ui->patternList->model()->index(index, 0));
}

void MainWindow::setPositionRange(int max)
{
    ui->spinPosition->setRange(0, max - 1);
    ui->slidePosition->setRange(0, max - 1);
}

void MainWindow::setZoom(int zoom)
{
    ui->actionZoom_In->setEnabled(zoom < RoadPathWidget::ZOOM_MAX);
    ui->actionZoom_Out->setEnabled(zoom > RoadPathWidget::ZOOM_MIN);
}

// ------------------------------------------------------------------------------------------------
// Menu Slots
// ------------------------------------------------------------------------------------------------

// New Project
void MainWindow::on_actionNew_Project_triggered()
{
    toggleControls(false);
    levelData->clear();
    heightSections.clear();

    ui->RenderS16Widget->setupRoadPalettes();
    roadPaletteWidget->refresh();
    ui->roadPathWidget->init();
    heightSection->generate();
    heightSection->newEntry(0);
    heightSection->setSection(0);
    spriteSections.clear();
    if (importOutRun->romsLoaded)
        spriteSections = importOutRun->loadSpriteSections(60); // Import checkpoint
    spriteSection->generateEntries();
    ui->roadPathWidget->setView(ui->editModeTabs->currentIndex()); // also enables insert button correctly
    ui->roadPathWidget->update();
    ui->RenderS16Widget->update(); // clear preview widget
    levels->init();
    importOutRun->loadSplit(levels->getSplit(), false);
    levels->getSplit()->updatePathData();
    levels->newLevel(); // Create default level
    levels->newEndSection();
    importOutRun->loadEndSection(levels->getEndSection(), false);
    levels->selectFirstLevel();
    levels->setDefaultMapping();
    file_loaded = false;
    this->setWindowTitle("Untitled - LayOut");
}

// Open Project
void MainWindow::on_actionOpen_Project_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
                            *new QString("Select a project file"),
                            projectPath,
                            *new QString("LayOut Projects (*.xml)"));

    if (!filename.isEmpty())
    {
        toggleControls(false);

        levels->init();
        heightSections.clear();
        projectPath = filename;
        xml->loadProject(filename);
        levels->selectFirstLevel();

        // Update HeightMap
        heightSection->generate();
        heightSection->setSection(0);

        ui->RenderS16Widget->init();
        ui->RenderS16Widget->setupRoadPalettes();
        roadPaletteWidget->refresh();

        ui->roadPathWidget->init();
        ui->roadPathWidget->setView(ui->editModeTabs->currentIndex()); // also enables insert button correctly
        ui->roadPathWidget->update();

        spriteSection->itemSelected(0, false);
        file_loaded = true;
        this->setWindowTitle(QFileInfo(filename).fileName() + " - LayOut");

    }
}

// Save Project
void MainWindow::on_actionSave_Project_triggered()
{
    if (!file_loaded || projectPath.isEmpty())
        on_actionSave_Project_As_triggered();
    else
        xml->saveProject(projectPath);
}

// Save Project As...
void MainWindow::on_actionSave_Project_As_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
                            *new QString("Select a project file"),
                            projectPath,
                            *new QString("LayOut Projects (*.xml)"));

    if (!filename.isEmpty())
    {
        file_loaded = true;
        projectPath = filename;
        xml->saveProject(filename);
        this->setWindowTitle(QFileInfo(filename).fileName() + " - LayOut");
    }
}

// Export to Cannonball
void MainWindow::on_actionCannonball_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
                            *new QString("Select an export file"),
                            exportPath,
                            *new QString("Cannonball Track Export (*.bin)"));

    if (!filename.isEmpty())
    {
        exportPath = filename;
        exportCannon->write(filename, levels, heightSections, spriteSections);
    }
}

// Exit
void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionZoom_To_100_triggered()
{
    ui->roadPathWidget->zoom = 100;
    ui->actionZoom_Out->setEnabled(true);
    ui->actionZoom_In->setEnabled(true);
    ui->roadPathWidget->setZoom(ui->roadPathWidget->zoom);
}

void MainWindow::on_actionZoom_In_triggered()
{
    if (ui->roadPathWidget->zoom < RoadPathWidget::ZOOM_MAX)
    {
        ui->roadPathWidget->zoom += 10;
        ui->roadPathWidget->setZoom(ui->roadPathWidget->zoom);
        ui->actionZoom_Out->setEnabled(true);
    }
    if (ui->roadPathWidget->zoom >= RoadPathWidget::ZOOM_MAX)
        ui->actionZoom_In->setEnabled(false);

}

void MainWindow::on_actionZoom_Out_triggered()
{
    if (ui->roadPathWidget->zoom > RoadPathWidget::ZOOM_MIN)
    {
        ui->roadPathWidget->zoom -= 10;
        ui->roadPathWidget->setZoom(ui->roadPathWidget->zoom);
        ui->actionZoom_In->setEnabled(true);
    }
    if (ui->roadPathWidget->zoom <= RoadPathWidget::ZOOM_MIN)
        ui->actionZoom_Out->setEnabled(false);
}

void MainWindow::on_actionFit_To_Window_triggered()
{
    ui->roadPathWidget->fitInView(ui->roadPathWidget->scene->sceneRect(), Qt::KeepAspectRatio);
    ui->roadPathWidget->zoom = ui->roadPathWidget->curScale() * 100;
    setZoom(ui->roadPathWidget->zoom);
}

// ------------------------------------------------------------------------------------------------
// Import
// ------------------------------------------------------------------------------------------------

// Import OutRun Heightmap
void MainWindow::on_load_outrun_heightmap_triggered()
{
    heightSections = importOutRun->loadHeightSections();
    heightSection->generate();
    heightSection->setSection(0);
}

// Import Road Path
void MainWindow::on_actionOutRun_Road_Path_triggered()
{
    importDialog->show();
}

// Import Road Split
void MainWindow::on_actionOutRun_Split_triggered()
{
    on_actionOutRun_Scenery_Patterns_triggered();
    importOutRun->loadSplit(levels->getSplit(), spriteSections.length() > 0);
    levels->setLevel(Levels::SPLIT);
}

void MainWindow::importLevel(int id)
{
    toggleControls(false);

    spriteSection->blockSignals(true);
    ui->roadPathWidget->blockSignals(true);

    if (id < ImportOutRun::NORMAL_LEVELS)
    {
        levels->newLevel(importOutRun->getLevelNames().at(id));
        levels->setLevel(Levels::NORMAL, levels->getNumNormalLevels() - 1);
    }
    else
    {
        levels->newEndSection(importOutRun->getLevelNames().at(id));
        levels->setLevel(Levels::END, levels->getNumEndSections() - 1);
    }

    heightSections.clear();
    on_load_outrun_heightmap_triggered();
    importOutRun->loadLevel(id);
    levelData->updatePathData();
    spriteSections = importOutRun->loadSpriteSections();
    spriteSection->generateEntries();
    ui->RenderS16Widget->init();
    ui->RenderS16Widget->setupRoadPalettes();
    roadPaletteWidget->refresh();

    spriteSection->blockSignals(false);
    ui->roadPathWidget->blockSignals(false);

    ui->roadPathWidget->init();
    ui->roadPathWidget->setView(ui->editModeTabs->currentIndex());
    spriteSection->itemSelected(0, false);
}

// Import Scenery Patterns
void MainWindow::on_actionOutRun_Scenery_Patterns_triggered()
{
    spriteSection->blockSignals(true);
    spriteList->blockSignals(true);
    ui->RenderS16Widget->blockSignals(true);

    spriteSections = importOutRun->loadSpriteSections();
    spriteSection->generateEntries();

    spriteSection->blockSignals(false);
    spriteList->blockSignals(false);
    ui->RenderS16Widget->blockSignals(false);
    spriteSection->itemSelected(0, false);
}

// Import Road Palette
void MainWindow::on_actionOutRun_Road_Palettes_triggered()
{
    importOutRun->loadSharedPalette();
    ui->RenderS16Widget->setupRoadPalettes();
    ui->RenderS16Widget->redrawPos();
    roadPaletteWidget->refresh();
}

// ------------------------------------------------------------------------------------------------
// Launch CannonBall External Process
// ------------------------------------------------------------------------------------------------

void MainWindow::on_actionCannonBall_Run_triggered()
{
    if (externalProcess == NULL || externalProcess->state() == QProcess::NotRunning)
    {
        stopExternalProcess();

        // If path not setup, use CannonBall path as a default
        if (exportRunPath.isNull() || exportRunPath.isEmpty())
            exportRunPath = QFileInfo(settingsDialog->cannonballPath).absolutePath();

        if (exportRunPath.isNull() || exportRunPath.isEmpty() || !QFileInfo(exportRunPath).isFile())
        {
            exportRunPath = QFileDialog::getSaveFileName(this,
                            *new QString("Select an export file"),
                            exportRunPath,
                            *new QString("Cannonball Track Export (*.bin)"));
        }

        if (!exportRunPath.isEmpty())
        {
            exportCannon->write(exportRunPath, levels, heightSections, spriteSections);
            externalProcess = new QProcess(this);
            QString program = settingsDialog->cannonballPath;
            QStringList arguments;
            arguments << "-file" << exportRunPath;
            // Forward output of program to the main output
            //externalProcess->setProcessChannelMode(QProcess::ForwardedChannels);
            externalProcess->setWorkingDirectory(QFileInfo(settingsDialog->cannonballPath).absolutePath());
            externalProcess->start(program, arguments);
        }
    }
    else
    {
        QMessageBox::warning(this, "CannonBall Launch Error", "CannonBall is already running!");
    }
}

void MainWindow::stopExternalProcess()
{
    if (externalProcess != NULL)
    {
        externalProcess->kill();
        delete externalProcess;
        externalProcess = NULL;
    }
}

void MainWindow::initRomData()
{
    bool success = false;

    if (!settingsDialog->romPath.isNull() && !settingsDialog->romPath.isEmpty())
    {
        QDir romPath = settingsDialog->romPath;

        if (romPath.isReadable())
        {
            if (importOutRun->loadRevBRoms(settingsDialog->romPath))
            {
                QRgb* spritePalette = Utils::convertSpritePalette(importOutRun->getPaletteData());
                Sprite::convertSpriteRom(importOutRun->sprites.rom, importOutRun->sprites.length, spritePalette);
                ui->RenderS16Widget->setData(levels, &heightSections, &spriteSections,
                                             &importOutRun->rom0, &importOutRun->sprites, &importOutRun->rom1, &importOutRun->road);
                ui->RenderS16Widget->setRoadPos(0);
                ui->previewPaletteWidget->setPaletteData(spritePalette, 8, 2);
                spriteList->setList(importOutRun->loadSpriteList());
                spriteList->toggleHiddenSprites(ui->checkHideSprites->isChecked());
                ui->RenderS16Widget->init();
                ui->RenderS16Widget->setRoadPos(0);
                on_actionNew_Project_triggered();
                ui->statusBar->showMessage("ROMs Successfully loaded");
                success = true;
            }
        }

        if (!success)
        {
            QMessageBox::warning(this, "ROM Load Failure", "Unable to load required ROM files.");
            ui->statusBar->showMessage("ERROR: Rom Load Failure");
        }
    }
    else
    {
        success = false;
        ui->statusBar->showMessage("WARNING: You must setup a rom path!");
        settingsDialog->show();
    }

    if (!success)
    {
        importOutRun->unloadRoms();
    }

    ui->tabMain->setEnabled(success);
    ui->menuImport->setEnabled(success);
    ui->menuExport->setEnabled(success);
    ui->actionNew_Project->setEnabled(success);
    ui->actionOpen_Project->setEnabled(success);
    ui->actionSave_Project->setEnabled(success);
    ui->actionSave_Project_As->setEnabled(success);
    ui->actionCannonball->setEnabled(success);
    ui->actionCannonBall_Run->setEnabled(success);
}

// ------------------------------------------------------------------------------------------------
// Load/Save GUI Configuration
// ------------------------------------------------------------------------------------------------

// On Windows, saved to: HKEY_CURRENT_USER\Software
void MainWindow::loadSettings()
{
    const int width = 800;
    const int height = 600;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect desk = screen->geometry();

    settings->beginGroup("MainWindow");
    resize(settings->value("size", QSize(width, height)).toSize());
    move(settings->value("pos", QPoint((desk.width() - width) >> 1, (desk.height() - height) >> 1)).toPoint());
    if (settings->value("maximized").toBool())
        showMaximized();
    projectPath = settings->value("lastPath").toString();
    exportPath  = settings->value("exportPath").toString();
    settingsDialog->cannonballPath = settings->value("cannonballPath").toString();
    settingsDialog->romPath = settings->value("romPath").toString();
    ui->comboGuidelines->setCurrentIndex(settings->value("guidelines").toInt());
    ui->checkScenery->setChecked(settings->value("highlightScenery").toBool());

    // Sprite Tab
    ui->checkHideSprites->setChecked(settings->value("hideSprites").toBool());
    settings->endGroup();
}

void MainWindow::saveSettings()
{
    settings->beginGroup("MainWindow");
    settings->setValue("size", size());
    settings->setValue("pos", pos());
    settings->setValue("maximized", isMaximized());
    settings->setValue("lastPath", projectPath);
    settings->setValue("exportPath", exportPath);
    settings->setValue("cannonballPath", settingsDialog->cannonballPath);
    settings->setValue("rompath", settingsDialog->romPath);
    settings->setValue("guidelines", ui->comboGuidelines->currentIndex());
    settings->setValue("highlightScenery", ui->checkScenery->isChecked());

    // Sprite Tab
    settings->setValue("hideSprites", ui->checkHideSprites->isChecked());
    settings->endGroup();
}

// Display LayOut Settings Dialog
void MainWindow::on_actionLayout_Preferences_triggered()
{
    settingsDialog->show();
}

// Display About Dialog
void MainWindow::on_actionAbout_LayOut_triggered()
{
    if (aboutDialog != NULL)
    {
        aboutDialog->close();
        delete aboutDialog;
    }

    aboutDialog    = new About(this, &importOutRun->rom0);
    aboutDialog->show();
}

// Display Licensing Information
void MainWindow::on_actionLicense_Information_triggered()
{
    QFile file(":/docs/res/license.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);
    QString contents = in.readAll();
    file.close();

    QMessageBox msgBox;
    // Hack to make the message box wider as setMinimumWidth doesn't work
    QSpacerItem* horizontalSpacer = new QSpacerItem(520, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    msgBox.setWindowTitle("LayOut Licensing Information");
    msgBox.setTextFormat(Qt::PlainText);
    msgBox.setText(contents);
    QGridLayout* layout = (QGridLayout*)msgBox.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msgBox.exec();
}

// Display Online Manual
void MainWindow::on_actionOnline_Manual_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/djyt/layout/wiki/LayOut-Manual", QUrl::TolerantMode));
}
