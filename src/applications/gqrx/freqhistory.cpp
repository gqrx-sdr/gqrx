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

#include "applications/gqrx/freqhistory.h"

FreqHistory::FreqHistory() :
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
        entry = *cache->at(--cur_index);
        return true;
    }
    return false;
}

bool FreqHistory::forward(FreqHistoryEntry &entry)
{
    store_tmp_entry();
    if (cache->count() > cur_index + 1)
    {
        entry = *cache->at(++cur_index);
        return true;
    }
    return false;
}

bool FreqHistory::is_first()
{
    return cur_index <= 0;
}

bool FreqHistory::is_last()
{
    return cur_index < 0 || cur_index >= cache->count() - 1 || (tmp_entry && *cache->at(cur_index) != *tmp_entry);
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
    store_tmp_entry();
    tmp_entry_tstamp = QDateTime::currentDateTime().addMSecs(timeout_ms());
    tmp_entry = FHCacheEntry(new FreqHistoryEntry(entry));

#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << "Modified tmp_entry: " << tmp_entry->freq_hz;
#endif
}

int FreqHistory::cache_size()
{
    return FH_DEFAULT_SIZE;
}

void FreqHistory::cleanup()
{
    if (cache->empty())
        return;

    cache->resize(cur_index + 1);
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

    if (cur_index >= 0 && *cache->at(cur_index) == *entry)
    {
        // entry already available
        return;
    }

    if (cache->count() >= cache_size())
    {
        // max. history cache size reached, remove first (oldest)
        cache->removeFirst();
        cur_index -= 1; /* should stay >= 0, but it's checked with cache_size in if block */
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
        qDebug() << "Reset tmp_entry";
#endif
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
