#ifndef LEVELITEM_HPP
#define LEVELITEM_HPP

#include <QStandardItem>

class LevelItem : public QStandardItem
{
    Q_OBJECT
public:
    explicit LevelItem(QObject *parent = 0);

    void setData(const QVariant& value, int role);
    
signals:
    
public slots:
    
};

#endif // LEVELITEM_HPP
