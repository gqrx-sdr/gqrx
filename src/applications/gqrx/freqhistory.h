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
#ifndef FREQHISTORY_H
#define FREQHISTORY_H

#include <QDateTime>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QVector>

#include "applications/gqrx/freqhistoryentry.h"

#define FH_DEFAULT_SIZE 20              /* the max. number of entries cached by default */
#define FH_DEFAULT_TIMEOUT_MS 2000      /* timeout in ms need to elapse for history */

typedef QSharedPointer<FreqHistoryEntry> FHCacheEntry;
typedef QVector<FHCacheEntry> FHCache;

/**
 * @brief FreqHistory caches frequency settings for fast jump back/forward in history
 *
 * The newest FH_DEFAULT_SIZE entries are cached in history.
 * Storing values in history happens after delay of FH_DEFAULT_TIMEOUT_MS ms.
 */
class FreqHistory
{
public:
    FreqHistory();
    /**
     * @brief Go back in history and retrieve the entry
     * @param entry
     * @return true if entry param set
     */
    bool back(FreqHistoryEntry &entry);

    /**
     * @brief Go forward after moving back in history and retrieve the newer entry
     * @param entry
     * @return true if entry param set
     */
    bool forward(FreqHistoryEntry &entry);

    /**
     * @brief currently on first position
     * @return
     */
    bool is_first();

    /**
     * @brief currently on last position
     * @return
     */
    bool is_last();

    /**
     * @brief Get current position in history
     * @return
     */
    int position() const;

    /**
     * @brief Get history size
     * @return
     */
    int size() const;

    /**
     * @brief Bring in sync any delayed history additions
     */
    void sync();

    /**
     * @brief Get timeout in ms when frequencies without change are added
     * @return
     */
    int timeout_ms() const;

    /**
     * @brief Make a new entry in history. This will become the current and also newest entry in history.
     * But not before time FH_DEFAULT_TIMEOUT_MS after this call has finished.
     * @param entry
     */
    void try_make_entry(const FreqHistoryEntry &entry);

#ifndef QT_NO_DEBUG_OUTPUT
    friend QDebug operator<<(QDebug dbg, const FreqHistory &history);
#endif

protected:
    int cache_size();

private:
    /**
     * @brief cleanup the cache up to one entry back in history
     */
    void cleanup();

    /**
     * @brief Handles adding entries
     * @param entry
     */
    void make_entry(const FHCacheEntry &entry);

    /**
     * @brief store tmp_entry permanent
     */
    void store_tmp_entry();

    /**
     * @brief storage for history caching
     */
    const QScopedPointer<FHCache> cache;

    /**
     * @brief temporary storage to try making new entry with handling timeout
     */
    FHCacheEntry tmp_entry;
    QDateTime tmp_entry_tstamp;

    int cur_index;
};


#endif // FREQHISTORY_H
