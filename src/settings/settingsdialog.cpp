/***************************************************************************
    Layout Settings Dialog

    Features:
    - Set Path to OutRun ROMs
    - Set Path to CannonBall Executable

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QFileDialog>
#include "settingsdialog.hpp"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->browseCannonBall, SIGNAL(clicked()), this, SLOT(browsePath()));
    connect(ui->browseRoms,       SIGNAL(clicked()), this, SLOT(browseRoms()));
    connect(ui->buttonBox,        SIGNAL(accepted()), this, SLOT(storeSettings())); // OK Button
    connect(ui->buttonBox,        SIGNAL(rejected()), this, SLOT(hide()));          // Cancel Button
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::show()
{
    QDialog::show();
    if (!cannonballPath.isNull())
        ui->linePath->setText(cannonballPath);

    if (!romPath.isNull())
        ui->lineRomPath->setText(romPath);
}

void SettingsDialog::browsePath()
{
    if (cannonballPathTemp.isEmpty() || cannonballPathTemp.isNull())
        cannonballPathTemp = cannonballPath;

    cannonballPathTemp = QFileDialog::getOpenFileName(this,
                          *new QString("Select CannonBall Executable"),
                          cannonballPathTemp);

    if (!cannonballPathTemp.isNull())
    {
        ui->linePath->setText(cannonballPathTemp);
    }
}

void SettingsDialog::browseRoms()
{
    if (romPathTemp.isEmpty() || romPathTemp.isNull())
        romPathTemp = romPath;

    romPathTemp = QFileDialog::getExistingDirectory(this,
                          *new QString("Select ROM Directory"),
                          romPathTemp,
                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!romPathTemp.isNull())
    {
        ui->lineRomPath->setText(romPathTemp);
    }
}

void SettingsDialog::storeSettings()
{
    if (!cannonballPathTemp.isNull())
        cannonballPath = cannonballPathTemp;

    if (!romPathTemp.isNull())
    {
        if (romPathTemp.compare(romPath) != 0)
        {
            romPath = romPathTemp;
            emit loadRoms();
        }
    }

    this->hide();
}
