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
class FreqHistory : public QObject
{
    Q_OBJECT

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
    bool is_first() const;

    /**
     * @brief currently on last position
     * @return
     */
    bool is_last() const;

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
     * @brief Get timeout in ms when frequencies without change are added/updated
     * @return
     */
    int timeout_ms() const;

#ifndef QT_NO_DEBUG_OUTPUT
    friend QDebug operator<<(QDebug dbg, const FreqHistory &history);
#endif

public slots:
    /**
     * @brief Make a new entry in history. This will become the current and also newest entry in history.
     * But not before time FH_DEFAULT_TIMEOUT_MS after this call has finished.
     * @param entry
     */
    void try_make_entry(const FreqHistoryEntry &entry);

    /**
     * @brief Modifies offset frequency on current history requirement freq_hz
     * @param freq_hz
     * @param offset_hz
     */
    void set_offset_freq(qint64 freq_hz, qint64 offset_hz);

    /**
     * @brief Modifies demod on current history requirement freq_hz
     * @param freq_hz
     * @param demod
     */
    void set_demod(qint64 freq_hz, DockRxOpt::rxopt_mode_idx demod);

    /**
     * @brief Modifies squelch db level on current history requirement freq_hz
     * @param freq_hz
     * @param db_level
     */
    void set_squelch(qint64 freq_hz, double db_level);

    /**
     * @brief Modifies filter index on current history requirement freq_hz
     * @param freq_hz
     * @param filter
     */
    void set_filter(qint64 freq_hz, int filter);

    /**
     * @brief Modifies filter frequency min and max on current history requirement freq_hz
     * @param freq_hz
     * @param min_hz
     * @param max_hz
     */
    void set_filter_freq(qint64 freq_hz, int min_hz, int max_hz);

    /**
     * @brief Modifies filter bandwidth on current history requirement freq_hz
     * @param freq_hz
     * @param bandwidth
     */
    void set_filter_bw(qint64 freq_hz, int bandwidth);

    /**
     * @brief Modifies filter shape on current history requirement freq_hz
     * @param freq_hz
     * @param shape
     */
    void set_filter_shape(qint64 freq_hz, int shape);

protected:
    /**
     * @brief The maximum size of history cache
     * @return
     */
    int cache_size() const;

private:
    /**
     * @brief cleanup the cache up to one entry back in history
     */
    inline void cleanup();

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
     * @brief emits signales related to history size changes
     */
    inline void emit_history_size_changed();

    template<typename T>
    inline void set_member(qint64 freq_hz, const FreqHistoryEntry::Varname varname, T value);

    template<typename T1, typename T2>
    inline void set_member(qint64 freq_hz, const FreqHistoryEntry::Varname varname1,
                           T1 value1, const FreqHistoryEntry::Varname varname2, T2 value2);

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

signals:
    /**
     * @brief history_updated emits after history cache modifications
     * @param index
     * @param history_size
     * @param entry
     */
    void history_updated(const int index, const int history_size, const FreqHistoryEntry &entry);

    /**
     * @brief history_size_changed emits when history size changes
     * @param index
     * @param history_size
     */
    void history_size_changed(const int index, const int history_size);

    /**
     * @brief history_first emits when first entry of history reached
     * @param is_first is true when first index, else false
     */
    void history_first(bool is_first);

    /**
     * @brief history_last emits when last entry of history reached
     * @param is_last is true when last index, else false
     */
    void history_last(bool is_last);

};


#endif // FREQHISTORY_H
