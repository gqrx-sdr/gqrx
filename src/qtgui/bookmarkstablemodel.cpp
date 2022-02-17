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
#include <functional>
#include <QFile>
#include <QStringList>
#include "bookmarks.h"
#include "bookmarkstablemodel.h"
#include "dockrxopt.h"


BookmarksTableModel::BookmarksTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_sortCol(0),
    m_sortDir(Qt::AscendingOrder)
{
}

int BookmarksTableModel::rowCount ( const QModelIndex & /*parent*/ ) const
{
    return m_Bookmarks.size();
}
int BookmarksTableModel::columnCount ( const QModelIndex & /*parent*/ ) const
{
    return COLUMN_COUNT;
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
        case COL_TAGS:
            return QString("Tag");
            break;
        case COL_LOCKED:
            return QString("Auto");
            break;
        case COL_MODULATION:
            return QString("Modulation");
            break;
        case COL_FILTER_LOW:
            return QString("Filter Low");
            break;
        case COL_FILTER_HIGH:
            return QString("Filter High");
            break;
        case COL_FILTER_TW:
            return QString("Filter Tw");
            break;
        case COL_AGC_ON:
            return QString("AGC");
            break;
        case COL_AGC_TARGET:
            return QString("AGC Target");
            break;
        case COL_AGC_MANUAL:
            return QString("AGC Manual");
            break;
        case COL_AGC_MAX:
            return QString("AGC Max Gain");
            break;
        case COL_AGC_ATTACK:
            return QString("AGC Attack");
            break;
        case COL_AGC_DECAY:
            return QString("AGC Decay");
            break;
        case COL_AGC_HANG:
            return QString("AGC Hang");
            break;
        case COL_AGC_PANNING:
            return QString("Panning");
            break;
        case COL_AGC_PANNING_AUTO:
            return QString("Autopanning");
            break;
        case COL_CW_OFFSET:
            return QString("CW Offset");
            break;
        case COL_FM_MAXDEV:
            return QString("FM Deviation");
            break;
        case COL_FM_DEEMPH:
            return QString("FM Deemphasis");
            break;
        case COL_AM_DCR:
            return QString("AM DCR");
            break;
        case COL_AMSYNC_DCR:
            return QString("AM SYNC DCR");
            break;
        case COL_AMSYNC_PLL_BW:
            return QString("AM SYNC PLL BW");
            break;
        case COL_NB1_ON:
            return QString("NB1 ON");
            break;
        case COL_NB1_THRESHOLD:
            return QString("NB1 Threshold");
            break;
        case COL_NB2_ON:
            return QString("NB2 ON");
            break;
        case COL_NB2_THRESHOLD:
            return QString("NB2 Threshold");
            break;
        case COL_REC_DIR:
            return QString("REC Directory");
            break;
        case COL_REC_SQL_TRIGGERED:
            return QString("REC SQL-triggered");
            break;
        case COL_REC_MIN_TIME:
            return QString("REC Min Time");
            break;
        case COL_REC_MAX_GAP:
            return QString("REC Max Gap");
            break;
        }
    }
    if(orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        return section;
    }
    return QVariant();
}

QVariant BookmarksTableModel::dataFromBookmark(BookmarkInfo &info, int index)
{
    switch(index)
    {
    case COL_FREQUENCY:
            return info.frequency;
    case COL_NAME:
            return info.name;
    case COL_TAGS:
        {
            QString strTags;
            for(int iTag=0; iTag<info.tags.size(); ++iTag)
            {
                if(iTag!=0)
                {
                    strTags.append(",");
                }
                TagInfo::sptr tag = info.tags[iTag];
                strTags.append(tag->name);
            }
            return strTags;
        }
    case COL_LOCKED:
            return info.get_freq_lock();
    case COL_MODULATION:
            return info.modulation;
    case COL_FILTER_LOW:
            return info.get_filter_low();
    case COL_FILTER_HIGH:
            return info.get_filter_high();
    case COL_FILTER_TW:
            return info.get_filter_tw();
    case COL_AGC_ON:
            return info.get_agc_on();
    case COL_AGC_TARGET:
            return info.get_agc_target_level();
    case COL_AGC_MANUAL:
            return info.get_agc_manual_gain();
    case COL_AGC_MAX:
            return info.get_agc_max_gain();
    case COL_AGC_ATTACK:
            return info.get_agc_attack();
    case COL_AGC_DECAY:
            return info.get_agc_decay();
    case COL_AGC_HANG:
            return info.get_agc_hang();
    case COL_AGC_PANNING:
            return info.get_agc_panning();
    case COL_AGC_PANNING_AUTO:
            return info.get_agc_panning_auto();
    case COL_CW_OFFSET:
            return info.get_cw_offset();
    case COL_FM_MAXDEV:
            return info.get_fm_maxdev();
    case COL_FM_DEEMPH:
            return info.get_fm_deemph() * 1e6;
    case COL_AM_DCR:
            return info.get_am_dcr();
    case COL_AMSYNC_DCR:
            return info.get_amsync_dcr();
    case COL_AMSYNC_PLL_BW:
            return info.get_amsync_pll_bw();
    case COL_NB1_ON:
            return info.get_nb_on(1);
    case COL_NB1_THRESHOLD:
            return info.get_nb_threshold(1);
    case COL_NB2_ON:
            return info.get_nb_on(2);
    case COL_NB2_THRESHOLD:
            return info.get_nb_threshold(2);
    case COL_REC_DIR:
            return QString::fromStdString(info.get_audio_rec_dir());
    case COL_REC_SQL_TRIGGERED:
            return info.get_audio_rec_sql_triggered();
    case COL_REC_MIN_TIME:
            return info.get_audio_rec_min_time();
    case COL_REC_MAX_GAP:
            return info.get_audio_rec_max_gap();
    }
    return 0;
}

QVariant BookmarksTableModel::data ( const QModelIndex & index, int role ) const
{
    BookmarkInfo &info = *getBookmarkAtRow(index.row());

    if(role==Qt::BackgroundRole)
    {
        QColor bg(info.GetColor());
        bg.setAlpha(0x60);
        return bg;
    }

    else if(role == Qt::DisplayRole || role==Qt::EditRole)
    {
        return BookmarksTableModel::dataFromBookmark(info, index.column());
    }
    return QVariant();
}

bool BookmarksTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role==Qt::EditRole)
    {
        BookmarkInfo &info = *getBookmarkAtRow(index.row());
        switch (index.column())
        {
        case COL_FREQUENCY:
            {
                info.frequency = value.toLongLong();
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
        case COL_TAGS:
            {
                info.tags.clear();
                QString strValue = value.toString();
                QStringList strList = strValue.split(",");
                for (int i = 0; i < strList.size(); ++i)
                {
                    QString strTag = strList[i].trimmed();
                    info.tags.append( Bookmarks::Get().findOrAddTag(strTag) );
                }
                emit dataChanged(index, index);
                return true;
            }
            break;
        case COL_LOCKED:
            {
                info.set_freq_lock(value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_MODULATION:
            {
                Q_ASSERT(!value.toString().contains(";")); // may not contain a comma because tablemodel is saved as comma-separated file (csv).
                if(Modulations::IsModulationValid(value.toString()))
                {
                    info.modulation = value.toString();
                    info.set_demod(Modulations::GetEnumForModulationString(info.modulation));
                    emit dataChanged(index, index);
                }
            }
            break;
        case COL_FILTER_LOW:
            {
                info.set_filter_low(value.toInt());
                if (Modulations::IsFilterSymmetric(info.get_demod()))
                    info.set_filter_high(-value.toInt());
                info.filter_adjust();
                emit dataChanged(index.sibling(index.row(), COL_FILTER_LOW),
                                 index.sibling(index.row(), COL_FILTER_TW));
            }
            break;
        case COL_FILTER_HIGH:
            {
                info.set_filter_high(value.toInt());
                if(Modulations::IsFilterSymmetric(info.get_demod()))
                    info.set_filter_low(-value.toInt());
                info.filter_adjust();
                emit dataChanged(index.sibling(index.row(), COL_FILTER_LOW),
                                 index.sibling(index.row(), COL_FILTER_TW));
            }
            break;
        case COL_FILTER_TW:
            {
                info.set_filter_tw(value.toInt());
                info.filter_adjust();
                emit dataChanged(index.sibling(index.row(), COL_FILTER_LOW),
                                 index.sibling(index.row(), COL_FILTER_TW));
            }
            break;
        case COL_AGC_ON:
            {
                info.set_agc_on(value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_TARGET:
            {
                info.set_agc_target_level(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_MANUAL:
            {
                info.set_agc_manual_gain(value.toFloat());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_MAX:
            {
                info.set_agc_max_gain(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_ATTACK:
            {
                info.set_agc_attack(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_DECAY:
            {
                info.set_agc_decay(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_HANG:
            {
                info.set_agc_hang(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_PANNING:
            {
                info.set_agc_panning(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_AGC_PANNING_AUTO:
            {
                info.set_agc_panning_auto(value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_CW_OFFSET:
            {
                info.set_cw_offset(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_FM_MAXDEV:
            {
                info.set_fm_maxdev(value.toFloat());
                emit dataChanged(index, index);
            }
            break;
        case COL_FM_DEEMPH:
            {
                info.set_fm_deemph(value.toFloat() * 1e-6);
                emit dataChanged(index, index);
            }
            break;
        case COL_AM_DCR:
            {
                info.set_am_dcr(value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_AMSYNC_DCR:
            {
                info.set_amsync_dcr(value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_AMSYNC_PLL_BW:
            {
                info.set_amsync_pll_bw(value.toFloat());
                emit dataChanged(index, index);
            }
            break;
        case COL_NB1_ON:
            {
                info.set_nb_on(1, value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_NB1_THRESHOLD:
            {
                info.set_nb_threshold(1, value.toFloat());
                emit dataChanged(index, index);
            }
            break;
        case COL_NB2_ON:
            {
                info.set_nb_on(2, value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_NB2_THRESHOLD:
            {
                info.set_nb_threshold(2, value.toFloat());
                emit dataChanged(index, index);
            }
            break;
        case COL_REC_DIR:
            {
                info.set_audio_rec_dir(value.toString().toStdString());
                emit dataChanged(index, index);
            }
            break;
        case COL_REC_SQL_TRIGGERED:
            {
                info.set_audio_rec_sql_triggered(value.toBool());
                emit dataChanged(index, index);
            }
            break;
        case COL_REC_MIN_TIME:
            {
                info.set_audio_rec_min_time(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        case COL_REC_MAX_GAP:
            {
                info.set_audio_rec_max_gap(value.toInt());
                emit dataChanged(index, index);
            }
            break;
        }
        return true; // return true means success
    }
    return false;
}

Qt::ItemFlags BookmarksTableModel::flags ( const QModelIndex& index ) const
{
    Qt::ItemFlags flags = Qt::ItemFlags();

    switch(index.column())
    {
    case COL_TAGS:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        break;
    default:
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        break;
    }
    return flags;
}

void BookmarksTableModel::update()
{
    m_Bookmarks.clear();
    for(int iBookmark=0; iBookmark<Bookmarks::Get().size(); iBookmark++)
    {
        BookmarkInfo& info = Bookmarks::Get().getBookmark(iBookmark);

        for(int iTag=0; iTag<info.tags.size(); ++iTag)
        {
            TagInfo::sptr tag = info.tags[iTag];
            if(tag->active)
            {
                m_Bookmarks.append(iBookmark);
                break;
            }
        }
    }
    sort(m_sortCol, m_sortDir);
}

BookmarkInfo *BookmarksTableModel::getBookmarkAtRow(int row) const
{
    return & Bookmarks::Get().getBookmark(m_Bookmarks[row]);
}

int BookmarksTableModel::GetBookmarksIndexForRow(int iRow)
{
  return m_Bookmarks[iRow];
}

int BookmarksTableModel::GetRowForBookmarkIndex(int index)
{
    return m_Bookmarks.indexOf(index);
}

bool BookmarksTableModel::bmCompare(const int a, const int b, int column, int order)
{
        switch (column)
        {
        case COL_FREQUENCY://LongLong
            if (order)
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toLongLong() >=
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toLongLong();
            else
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toLongLong() <
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toLongLong();

        case COL_FILTER_LOW://int
        case COL_FILTER_HIGH:
        case COL_FILTER_TW:
        case COL_AGC_TARGET:
        case COL_AGC_MAX:
        case COL_AGC_ATTACK:
        case COL_AGC_DECAY:
        case COL_AGC_HANG:
        case COL_AGC_PANNING:
        case COL_CW_OFFSET:
        case COL_REC_MIN_TIME:
        case COL_REC_MAX_GAP:
            if (order)
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toInt() >=
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toInt();
            else
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toInt() <
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toInt();

        case COL_LOCKED://bool
        case COL_AGC_ON:
        case COL_AGC_PANNING_AUTO:
        case COL_AM_DCR:
        case COL_AMSYNC_DCR:
        case COL_NB1_ON:
        case COL_NB2_ON:
        case COL_REC_SQL_TRIGGERED:
            if (order)
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toBool() >=
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toBool();
            else
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toBool() <
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toBool();

        case COL_AGC_MANUAL://float
        case COL_FM_MAXDEV:
        case COL_FM_DEEMPH:
        case COL_AMSYNC_PLL_BW:
        case COL_NB1_THRESHOLD:
        case COL_NB2_THRESHOLD:
            if (order)
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toFloat() >=
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toFloat();
            else
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toFloat() <
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toFloat();

        case COL_NAME://string
        case COL_TAGS:
        case COL_MODULATION:
        case COL_REC_DIR:
        default:
            if (order)
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toString() >=
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toString();
            else
                return dataFromBookmark(Bookmarks::Get().getBookmark(a), column).toString() <
                    dataFromBookmark(Bookmarks::Get().getBookmark(b), column).toString();

        }
}

void BookmarksTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0)
        return;
    m_sortCol = column;
    m_sortDir = order;
    std::stable_sort(m_Bookmarks.begin(), m_Bookmarks.end(),
                     std::bind(bmCompare, std::placeholders::_1,
                               std::placeholders::_2, column, order));
    emit layoutChanged();
}
