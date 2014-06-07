/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2013 Christian Lindner DL2VCL, Stefano Leucci.
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
#include <QFile>
#include <QStringList>
#include "bookmarks.h"
#include "bookmarkstablemodel.h"
#include "dockrxopt.h"


BookmarksTableModel::BookmarksTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

bool BookmarksTableModel::load(QString filename)
{
    //FIXME
    bool result = Bookmarks::load(filename);
    update();
    return result;
}

int BookmarksTableModel::rowCount ( const QModelIndex & /*parent*/ ) const
{
    return m_Bookmarks.size();
}
int BookmarksTableModel::columnCount ( const QModelIndex & /*parent*/ ) const
{
    return 5;
}

QVariant BookmarksTableModel::headerData ( int section, Qt::Orientation orientation, int role ) const
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
        case COL_TAGS:
            return QString("Tag");
            break;
        }
    }
    if(orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        return section;
    }
    return QVariant();
}

QVariant BookmarksTableModel::data ( const QModelIndex & index, int role ) const
{
    BookmarkInfo& info = *m_Bookmarks[index.row()];

    if(role==Qt::BackgroundColorRole)
    {
        QColor bg(info.tag->color);
        bg.setAlpha(0x60);
        return bg;
    }
    else if(role == Qt::DisplayRole || role==Qt::EditRole)
    {
        switch(index.column())
        {
        case COL_FREQUENCY:
                return info.frequency;
        case COL_NAME:
                return (role==Qt::EditRole)?QString(info.name):info.name;
        case COL_MODULATION:
                return info.modulation;
        case COL_BANDWIDTH:
            return (info.bandwidth==0)?QVariant(""):QVariant(info.bandwidth);
         case COL_TAGS:
            return (role==Qt::EditRole)?QString(info.tag->name):info.tag->name;
        }
    }
    return QVariant();
}

bool BookmarksTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(role==Qt::EditRole)
    {
        BookmarkInfo &info = *m_Bookmarks[index.row()];
        switch(index.column())
        {
        case COL_FREQUENCY:
            {
                info.frequency = value.toInt();
                emit dataChanged(index, index);
            }
            break;
        case COL_NAME:
            {
                info.name = value.toString();
                emit dataChanged(index, index);
                return true;
            }
            break;
        case COL_MODULATION:
            {
                Q_ASSERT(!value.toString().contains(";")); // may not contain a comma because tablemodel is saved as comma-separated file (csv).
                if(DockRxOpt::IsModulationValid(value.toString()))
                {
                    info.modulation = value.toString();
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_BANDWIDTH:
            {
                info.bandwidth = value.toInt();
                emit dataChanged(index, index);
            }
            break;
        case COL_TAGS:
            {
                info.tag = &Bookmarks::findOrAddTag(value.toString().trimmed());
                emit dataChanged(index, index);
                return true;
            }
            break;
        }
        return true; // return true means success
    }
    return false;
}

Qt::ItemFlags BookmarksTableModel::flags ( const QModelIndex & index ) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    return flags;
}

void BookmarksTableModel::update()
{
    m_Bookmarks.clear();
    for(int i=0; i<Bookmarks::size(); i++)
    {
        BookmarkInfo& info = Bookmarks::getBookmark(i);

        if(info.tag->active)
            m_Bookmarks.append(&info);
    }

    emit layoutChanged();
}

BookmarkInfo *BookmarksTableModel::getBookmarkAtRow(int row)
{
    return m_Bookmarks[row];
}
