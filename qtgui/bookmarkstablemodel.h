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
#ifndef BOOKMARKSTABLEMODEL_H
#define BOOKMARKSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "bookmarks.h"


class BookmarksTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum EColumns
    {
        COL_FREQUENCY,
        COL_NAME,
        COL_MODULATION,
        COL_BANDWIDTH,
        COL_TAGS
    };

    explicit BookmarksTableModel(QObject *parent = 0);
    
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    Qt::ItemFlags flags ( const QModelIndex & index ) const;

    BookmarkInfo* getBookmarkAtRow(int row);
    int GetBookmarksIndexForRow(int iRow);

private:
    QList<BookmarkInfo*> m_Bookmarks;
    QMap<int,int> m_mapRowToBookmarksIndex;

signals:
public slots:
    void update();

};

#endif
