#-------------------------------------------------
#
# LayOut QMake File
#
#-------------------------------------------------

# Set the path to where the source tree is stored
INCLUDEPATH += c:/coding/qt/lastwave/LayOut

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LayOut
TEMPLATE = app

# Icons
RC_ICONS = res/icon48x48.ico   # Windows
ICON     = res/icon48x48.icns  # Mac

SOURCES += main.cpp\
        mainwindow.cpp \
        import/romloader.cpp \
        roadedit/roadpathwidget.cpp \
        leveldata.cpp \
        generatexml.cpp \
        export/exportcannonball.cpp \
        import/importoutrun.cpp \
        sprites/spritelist.cpp \
        sprites/spritesection.cpp \
        sprites/sprite.cpp \
        sprites/previewwidget.cpp \
        preview/oroad.cpp \
        preview/hwroad.cpp \
        preview/renders16.cpp \
        preview/osprite.cpp \
        preview/osprites.cpp \
        preview/olevelobjs.cpp \
        preview/hwsprites.cpp \
        height/heightwidget.cpp \
        height/heightsection.cpp \
        import/importdialog.cpp \
        levelpalettewidget/levelpalettewidget.cpp \
        previewpalette.cpp \
        utils.cpp \
        levels/levels.cpp \
        roadedit/roadpathscene.cpp \
        settings/settingsdialog.cpp \
        about/about.cpp

HEADERS += mainwindow.h \
        import/romloader.hpp \
        stdint.hpp \
        roadedit/roadpathwidget.hpp \
        leveldata.hpp \
        generatexml.hpp \
        export/exportcannonball.hpp \
        import/importbase.hpp \
        export/exportbase.hpp \
        import/importoutrun.hpp \
        sprites/spritelist.hpp \
        sprites/spritesection.hpp \
        sprites/sprite.hpp \
        sprites/previewwidget.hpp \
        preview/oroad.hpp \
        preview/hwroad.hpp \
        globals.hpp \
        preview/renders16.hpp \
        preview/ozoom_lookup.hpp \
        preview/oentry.hpp \
        preview/osprites.hpp \
        preview/olevelobjs.hpp \
        preview/hwsprites.hpp \
        controlpoint.hpp \
        sprites/spriteformat.hpp \
        import/outrunlabels.hpp \
        height/heightwidget.hpp \
        height/heightsection.hpp \
        height/heightlabels.hpp \
        import/importdialog.hpp \
        levelpalettewidget/levelpalettewidget.hpp \
        previewpalette.hpp \
        utils.hpp \
        levels/levels.hpp \
        height/heightformat.hpp \
        roadedit/roadpathscene.hpp \
        settings/settingsdialog.hpp \
        about/about.hpp \
        levels/levelpalette.hpp

FORMS   += mainwindow.ui \
        import/importdialog.ui \
        levelpalettewidget/levelpalettewidget.ui \
        levels/levels.ui \
        settings/settingsdialog.ui \
        about/about.ui

RESOURCES += \
    layout.qrc

