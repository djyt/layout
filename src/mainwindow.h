/***************************************************************************
    Layout: A Track Editor for OutRun
    - Main Window Handler

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "leveldata.hpp"

namespace Ui {
class MainWindow;
}

class QProcess;
class QStandardItem;
class QString;
class QSettings;

class ImportDialog;
class SettingsDialog;
class About;
class GenerateXML;
class ExportCannonball;
class ImportOutRun;
class HeightModel;
class HeightSection;
class LevelPaletteWidget;
class SpriteList;
class SpriteSection;
struct SpriteEntry;
struct SpriteSectionEntry;
struct HeightEntry;
class Levels;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void toggleControls(bool);
    void toggleLaneControls(bool);

private slots:
    void on_actionExit_triggered();
    void on_actionSave_Project_triggered();
    void on_actionOpen_Project_triggered();
    void on_actionCannonball_triggered();
    void on_actionSave_Project_As_triggered();
    void on_actionNew_Project_triggered();
    void on_load_outrun_heightmap_triggered();
    void on_actionOutRun_Road_Path_triggered();
    void on_actionOutRun_Scenery_Patterns_triggered();
    void on_actionOutRun_Road_Palettes_triggered();
    void on_actionZoom_In_triggered();
    void on_actionZoom_Out_triggered();
    void on_actionFit_To_Window_triggered();
    void on_actionZoom_To_100_triggered();
    void on_actionLayout_Preferences_triggered();
    void on_actionCannonBall_Run_triggered();

    void updateWidth(int);
    void updateFrequencyGroup(int);
    void setFrequency();

    void newHeight();
    void selectHeightPattern(int);
    void setHeightPattern();
    void editHeightPattern();
    void selectHeightPoint(int);
    void setHeightPoint(int);

    void setSceneryPattern();
    void editSceneryPattern();
    void searchSceneryPattern(QString searchTerm);
    void selectSceneryPattern(int);
    void setSpriteProps(SpriteEntry*);
    void setHeightSection(HeightSegment*);
    void setPaletteSpin(int);

    void setPositionRange(int);
    void setZoom(int);

    void importLevel(int);
    void initLevel();
    void initRomData();

    void on_actionAbout_LayOut_triggered();

    void on_actionLicense_Information_triggered();

    void on_actionOnline_Manual_triggered();

    void on_actionOutRun_Split_triggered();

protected:
    void closeEvent(QCloseEvent* event);

private:
    QWidget* parent;
    QSettings *settings;
    QString projectPath;
    QString exportPath;
    QString exportRunPath;
    GenerateXML *xml;
    Ui::MainWindow *ui;
    ImportDialog* importDialog;
    SettingsDialog* settingsDialog;
    About* aboutDialog;

    ExportCannonball *exportCannon;
    ImportOutRun* importOutRun;

    HeightSection* heightSection;
    LevelPaletteWidget* roadPaletteWidget;
    LevelPalette* roadPalette;

    Levels* levels;

    // Height Map Data
    QList<HeightSegment> heightSections;

    QList<SpriteSectionEntry> spriteSections;
    SpriteList* spriteList;
    SpriteSection* spriteSection;

    bool file_loaded;

    QProcess* externalProcess;

    void stopExternalProcess();
    void loadSettings();
    void saveSettings();
};

#endif // MAINWINDOW_H
