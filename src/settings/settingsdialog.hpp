/***************************************************************************
    Layout Settings Dialog

    Features:
    - Set Path to OutRun ROMs
    - Set Path to CannonBall Executable

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    QString cannonballPath;
    QString romPath;

    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

signals:
    void loadRoms();

public slots:
    void show();

private slots:
    void browsePath();
    void browseRoms();
    void storeSettings();
    
private:
    QString cannonballPathTemp;
    QString romPathTemp;

    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_HPP
