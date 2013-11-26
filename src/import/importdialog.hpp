/***************************************************************************
    Import Level Dialog

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef IMPORTDIALOG_HPP
#define IMPORTDIALOG_HPP

#include <QDialog>

namespace Ui {
class ImportDialog;
}

class ImportDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ImportDialog(QWidget *parent, QString windowTitle, QList<QString> itemList, bool showWarning = false);
    ~ImportDialog();
    
signals:
    void outputLevel(int);

private slots:
    void selectLevel();

private:
    Ui::ImportDialog *ui;
};

#endif // IMPORTDIALOG_HPP
