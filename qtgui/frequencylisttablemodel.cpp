#include "frequencylisttablemodel.h"
#include <QFile>
#include <QStringList>

FrequencyListTableModel::FrequencyListTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

bool FrequencyListTableModel::load(QString configDir, QString filename)
{
    //table.append(Row( 98.5e6, "Radio Bayern 3", 400e3, "WFM"));
    //table.append(Row( 99.0e6, "Radio Oe3", 400e3, "WFM"));

    // Read from comma-separated file:
    QString pathAndFilename = configDir + "/frequency-list/" + filename + ".csv";
    QFile file(pathAndFilename);
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
        return true;
    }
    else
    {
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
        case 0:
            return QString("Frequency");
            break;
        case 1:
            return QString("Name");
            break;
        case 2:
            return QString("Modulation");
            break;
        case 3:
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
        case 0:
            return table[index.row()].frequency;
            break;
        case 1:
            return table[index.row()].name;
            break;
        case 2:
            return table[index.row()].modulation;
            break;
        case 3:
            return table[index.row()].bandwidth;
            break;
        }
    }
    return QVariant();
}

void FrequencyListTableModel::activated(const QModelIndex & index )
{
    qint64 freq = table[index.row()].frequency;
    emit newFrequency(freq);
}
