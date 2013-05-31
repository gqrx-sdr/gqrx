#include "frequencylisttablemodel.h"
#include <QFile>
#include <QStringList>

FrequencyListTableModel::FrequencyListTableModel(QString dir, QObject *parent) :
    QAbstractTableModel(parent),
    freqTableDir(dir)
{
}

bool FrequencyListTableModel::load(QString filename)
{
    //table.append(Row( 98.5e6, "Radio Bayern 3", 400e3, "WFM"));
    //table.append(Row( 99.0e6, "Radio Oe3", 400e3, "WFM"));

    // Read from comma-separated file:
    table.clear();
    QFile file(freqTableDir + filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!file.atEnd())
        {
            QString line = file.readLine();
            if(line.trimmed().left(1).compare("#") != 0)
            {
                QStringList strings = line.split(",");
                if(strings.count() == Row::iNumColumns)
                {
                    Row row;
                    row.frequency  = strings[0].toInt();
                    row.name       = strings[1].trimmed();
                    row.modulation = strings[2].trimmed();
                    row.bandwidth  = strings[3].toInt();
                    table.append(row);
                }
            }
        }
        file.close();
        emit layoutChanged();
        return true;
    }
    else
    {
        emit layoutChanged();
        return false;
    }
}

int FrequencyListTableModel::rowCount ( const QModelIndex & /*parent*/ ) const
{
    return table.count();
}
int FrequencyListTableModel::columnCount ( const QModelIndex & /*parent*/ ) const
{
    return Row::iNumColumns;
}

QVariant FrequencyListTableModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch(section)
        {
        case COL_FREQUENCY:
            return QString("Frequency");
            break;
        case COL_NAME:
            return QString("Name");
            break;
        case COL_MODULATION:
            return QString("Modulation");
            break;
        case COL_BANDWIDTH:
            return QString("Bandwidth");
            break;
        }
    }
    if(orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        return section;
    }
    return QVariant();
}

QVariant FrequencyListTableModel::data ( const QModelIndex & index, int role ) const
{
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case COL_FREQUENCY:
            return table[index.row()].frequency;
            break;
        case COL_NAME:
            return table[index.row()].name;
            break;
        case COL_MODULATION:
            return table[index.row()].modulation;
            break;
        case COL_BANDWIDTH:
            return table[index.row()].bandwidth;
            break;
        }
    }
    return QVariant();
}

Qt::ItemFlags FrequencyListTableModel::flags ( const QModelIndex & index ) const
{
  return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}
