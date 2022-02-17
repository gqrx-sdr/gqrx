/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include "receivers/defines.h"
#include "receivers/modulations.h"

#include "bookmarks.h"

class BookmarksTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum EColumns
    {
        COL_FREQUENCY = 0,
        COL_NAME,
        COL_TAGS,
        COL_LOCKED,
        COL_MODULATION,
        COL_FILTER_LOW,
        COL_FILTER_HIGH,
        COL_FILTER_TW,
        COL_AGC_ON,
        COL_AGC_TARGET,
        COL_AGC_MANUAL,
        COL_AGC_MAX,
        COL_AGC_ATTACK,
        COL_AGC_DECAY,
        COL_AGC_HANG,
        COL_AGC_PANNING,
        COL_AGC_PANNING_AUTO,
        COL_CW_OFFSET,
        COL_FM_MAXDEV,
        COL_FM_DEEMPH,
        COL_AM_DCR,
        COL_AMSYNC_DCR,
        COL_AMSYNC_PLL_BW,
        COL_NB1_ON,
        COL_NB1_THRESHOLD,
        COL_NB2_ON,
        COL_NB2_THRESHOLD,
        COL_REC_DIR,
        COL_REC_SQL_TRIGGERED,
        COL_REC_MIN_TIME,
        COL_REC_MAX_GAP,
        COLUMN_COUNT
    };

    explicit BookmarksTableModel(QObject *parent = 0);

    int rowCount ( const QModelIndex & parent = QModelIndex() ) const override;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const override;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override;
    Qt::ItemFlags flags ( const QModelIndex & index ) const override;

    BookmarkInfo* getBookmarkAtRow(int row) const;
    int GetBookmarksIndexForRow(int iRow);
    int GetRowForBookmarkIndex(int index);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

private:
    static QVariant dataFromBookmark(BookmarkInfo &info, int index);
    static bool bmCompare(const int a, const int b, int column, int order);
private:
    QList<int>    m_Bookmarks;
    int           m_sortCol;
    Qt::SortOrder m_sortDir;

signals:
public slots:
    void update();

};

#endif
