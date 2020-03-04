/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2020 Markus Kolb
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

#include <QTimer>

#include "applications/gqrx/freqhistory.h"

FreqHistory::FreqHistory() : QObject(),
    cache(new FHCache()),
    tmp_entry_tstamp(QDateTime::currentDateTime().addMSecs(std::numeric_limits<qint64>::max())),
    cur_index(-1)
{
}

bool FreqHistory::back(FreqHistoryEntry &entry)
{
    store_tmp_entry();
    if (cur_index > 0)
    {
        if (tmp_entry.isNull() || cache->at(cur_index)->equals(*tmp_entry, true))
            entry = *cache->at(--cur_index);
        else
            entry = *cache->at(cur_index);
        emit history_first(is_first());
        emit history_last(is_last());
        return true;
    }
    emit history_first(is_first());
    emit history_last(is_last());
    return false;
}

bool FreqHistory::forward(FreqHistoryEntry &entry)
{
    store_tmp_entry();
    if (cache->count() > cur_index + 1)
    {
        entry = *cache->at(++cur_index);
        emit history_first(is_first());
        emit history_last(is_last());
        return true;
    }
    emit history_first(is_first());
    emit history_last(is_last());
    return false;
}

bool FreqHistory::get_entry(int position, FreqHistoryEntry &entry)
{
    if (position >= 0)
    {
        entry = *cache->at(position);
        return true;
    }
    return false;
}

bool FreqHistory::is_first() const
{
    return cur_index <= 0;
}

bool FreqHistory::is_last() const
{
    return cur_index < 0 || cur_index >= cache->count() - 1;
}

int FreqHistory::position() const
{
    return cur_index;
}

int FreqHistory::size() const
{
    return cache->count();
}

void FreqHistory::sync()
{
    store_tmp_entry();
}

int FreqHistory::timeout_ms() const
{
    return FH_DEFAULT_TIMEOUT_MS;
}

void FreqHistory::try_make_entry(const FreqHistoryEntry &entry)
{
    if (tmp_entry && tmp_entry->freq_hz == entry.freq_hz)
    {
        // update available tmp_entry before storing
        if (*tmp_entry != entry)
            *tmp_entry = entry;

#ifndef QT_NO_DEBUG_OUTPUT
        qDebug() << "Modified tmp_entry: " << tmp_entry->freq_hz;
#endif

        store_tmp_entry();
    }
    else
    {
        store_tmp_entry();
        tmp_entry_tstamp = QDateTime::currentDateTime().addMSecs(timeout_ms());
        tmp_entry = FHCacheEntry(new FreqHistoryEntry(entry));

#ifndef QT_NO_DEBUG_OUTPUT
        qDebug() << "New tmp_entry: " << tmp_entry->freq_hz;
#endif
    }

    // try to sync after the timeout (+ 20 to be sure after tmp_entry_tstamp)
    QTimer::singleShot(timeout_ms() + 20, this, [=]() {
        store_tmp_entry();
    });
}

void FreqHistory::set_offset_freq(qint64 freq_hz, qint64 offset_hz)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::offset_freq_hz, offset_hz);
}

void FreqHistory::set_demod(qint64 freq_hz, DockRxOpt::rxopt_mode_idx demod)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::demod, demod);
}

void FreqHistory::set_squelch(qint64 freq_hz, double db_level)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::squelch, db_level);
}

void FreqHistory::set_filter(qint64 freq_hz, int filter)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::filter, filter);
}

void FreqHistory::set_filter_freq(qint64 freq_hz, int min_hz, int max_hz)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::min_hz, min_hz, FreqHistoryEntry::Varname::max_hz, max_hz);
}

void FreqHistory::set_filter_bw(qint64 freq_hz, int bandwidth)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::filter_bw, bandwidth);
}

void FreqHistory::set_filter_shape(qint64 freq_hz, int shape)
{
    set_member(freq_hz, FreqHistoryEntry::Varname::filter_shape, shape);
}

int FreqHistory::cache_size() const
{
    return FH_DEFAULT_SIZE;
}

inline void FreqHistory::cleanup()
{
    if (cache->empty())
        return;

    cache->resize(cur_index + 1);
    emit_history_size_changed();
}

void FreqHistory::make_entry(const FHCacheEntry &entry)
{
#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << ">>> History [" << cache->count() << "] before:";
    for (auto i = 0; i < cache->count(); i++)
    {
        qDebug() << " " << (i < 10 ? " " : "") << i << " " << *cache->at(i);
    }
    qDebug() << "<<<";
#endif

    if (cur_index >= 0) {
        if (*cache->at(cur_index) == *entry)
        {
            // entry already available
            return;
        }
        else if (cache->at(cur_index)->equals(*entry, true))
        {
            // values for frequency changed
            cache->replace(cur_index, entry);
#ifndef QT_NO_DEBUG_OUTPUT
            qDebug() << ">>> History [" << cache->count() << "] modified:";
            for (auto i = 0; i < cache->count(); i++)
            {
                qDebug() << " " << (i < 10 ? " " : "") << i << " " << *cache->at(i);
            }
            qDebug() << "<<<";
#endif
            emit history_updated(cur_index, cache->count(), *entry);
            return;
        }
    }

    if (cache->count() >= cache_size())
    {
        // max. history cache size reached, remove first (oldest)
        cache->removeFirst();
        cur_index -= 1; /* should stay >= 0, but it's checked with cache_size in if block */
        emit_history_size_changed();
    }

    cur_index++;

    if (cache->count() > cur_index)
    {
        // in the middle of history cache
        cache->replace(cur_index, entry);
        cleanup();
    }
    else
    {
        cache->append(entry);
        emit_history_size_changed();
    }

#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << ">>> History [" << cache->count() << "] afterwards:";
    for (auto i = 0; i < cache->count(); i++)
    {
        qDebug() << " " << (i < 10 ? " " : "") << i << " " << *cache->at(i);
    }
    qDebug() << "<<<";
#endif
}

void FreqHistory::store_tmp_entry()
{
    if (tmp_entry && tmp_entry_tstamp.toMSecsSinceEpoch() <= QDateTime::currentMSecsSinceEpoch())
    {
        make_entry(tmp_entry);
        // set far in future
        tmp_entry_tstamp = QDateTime::currentDateTime().addMSecs(std::numeric_limits<qint64>::max());
        tmp_entry.reset();

#ifndef QT_NO_DEBUG_OUTPUT
        Q_ASSERT(tmp_entry.isNull());
        qDebug() << "Reset tmp_entry";
#endif
    }
}

inline void FreqHistory::emit_history_size_changed()
{
    emit history_size_changed(cur_index, cache->count());
    emit history_last(is_last());
    emit history_first(is_first());
}

template<typename T>
inline void FreqHistory::set_member(qint64 freq_hz, const FreqHistoryEntry::Varname varname, T value)
{
    if (tmp_entry && tmp_entry->freq_hz == freq_hz)
    {
        tmp_entry->set<T>(varname, value);
    }
    else if (cur_index >= 0 && cache->at(cur_index)->freq_hz == freq_hz)
    {
        FreqHistoryEntry entry(*cache->at(cur_index));
        entry.set<T>(varname, value);
        try_make_entry(entry);
    }
}

template<typename T1, typename T2>
inline void FreqHistory::set_member(qint64 freq_hz, const FreqHistoryEntry::Varname varname1, T1 value1,
                             const FreqHistoryEntry::Varname varname2, T2 value2)
{
    if (tmp_entry && tmp_entry->freq_hz == freq_hz)
    {
        tmp_entry->set<T1>(varname1, value1);
        tmp_entry->set<T2>(varname2, value2);
    }
    else if (cur_index >= 0 && cache->at(cur_index)->freq_hz == freq_hz)
    {
        FreqHistoryEntry entry(*cache->at(cur_index));
        entry.set<T1>(varname1, value1);
        entry.set<T2>(varname2, value2);
        try_make_entry(entry);
    }
}

#ifndef QT_NO_DEBUG_OUTPUT
QDebug operator<<(QDebug dbg, const FreqHistory &history)
{
    QDebugStateSaver saver(dbg);
    for (auto it = history.cache->cbegin(); it; it++)
    {
        dbg.nospace() << *it;
    }
    return dbg;
}
#endif
