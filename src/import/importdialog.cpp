/***************************************************************************
    Import Level Dialog

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <QStandardItemModel>
#include "ui_importdialog.h"
#include "importdialog.hpp"

// Setup as tool window with smaller title bar
ImportDialog::ImportDialog(QWidget *parent, QString windowTitle, QList<QString> itemList, bool showWarning) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::ImportDialog)
{
    ui->setupUi(this);

    // Setup List View
    QStandardItemModel* model = new QStandardItemModel();
    for (int i = 0; i < itemList.length(); i++)
    {
        QStandardItem *item = new QStandardItem(itemList.at(i));
        item->setEditable(false);
        model->appendRow(item);
    }

    ui->listView->setModel(model);

    // Default to selecting first item
    QModelIndex mi = model->index(0, 0);
    ui->listView->setCurrentIndex(mi);
    this->setWindowTitle(windowTitle);

    if (!showWarning)
        this->ui->label->hide();

    connect(this, SIGNAL(accepted()), this, SLOT(selectLevel()));
}

ImportDialog::~ImportDialog()
{
    delete ui;
}

void ImportDialog::selectLevel()
{
    QModelIndex index = ui->listView->selectionModel()->currentIndex();
    const int id = index.row();
    emit outputLevel(id);
}
