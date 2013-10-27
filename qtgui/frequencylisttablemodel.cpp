/* -*- c++ -*- */
/*
 * Copyright 2013 Christian Lindner DL2VCL.  
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "frequencylisttablemodel.h"
#include "applications/gqrx/bookmarks.h"
#include <QFile>
#include <QStringList>


FrequencyListTableModel::FrequencyListTableModel(QString dir, QObject *parent) :
    QAbstractTableModel(parent),
    freqTableDir(dir)
{
}

bool FrequencyListTableModel::load(QString filename)
{
        bool result = Bookmarks::load(freqTableDir + filename);
        emit layoutChanged();
        return result;
}

int FrequencyListTableModel::rowCount ( const QModelIndex & /*parent*/ ) const
{
    return Bookmarks::size();
}
int FrequencyListTableModel::columnCount ( const QModelIndex & /*parent*/ ) const
{
    return 4;
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
        BookmarkInfo& info = Bookmarks::getBookmark(index.row());
        switch(index.column())
        {
        case COL_FREQUENCY:
            {
                qint64 value = info.frequency;
                if(value == 0) return QString("");
                else return value;
                break;
            }
        case COL_NAME:
            {
                return info.name;
                break;
            }
        case COL_MODULATION:
            {
                return info.modulation;
                break;
            }
        case COL_BANDWIDTH:
            {
                qint64 value = info.bandwidth;
                if(value == 0) return QString("");
                else return value;
                break;
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags FrequencyListTableModel::flags ( const QModelIndex & index ) const
{
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void FrequencyListTableModel::update()
{
    emit layoutChanged();
}
