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

#include "applications/gqrx/freqhistoryentry.h"

FreqHistoryEntry::FreqHistoryEntry()
{
}

FreqHistoryEntry::FreqHistoryEntry(const FreqHistoryEntry &entry)
{
    assign(entry);
}

FreqHistoryEntry &FreqHistoryEntry::operator=(const FreqHistoryEntry &entry)
{
    assign(entry);
    return *this;
}

bool FreqHistoryEntry::operator==(const FreqHistoryEntry &entry)
{
    return (freq_hz == entry.freq_hz);
}

bool FreqHistoryEntry::operator!=(const FreqHistoryEntry &entry)
{
    return !(*this == entry);
}

void FreqHistoryEntry::assign(const FreqHistoryEntry &entry)
{
    demod = entry.demod;
    max_hz = entry.max_hz;
    min_hz = entry.min_hz;
    freq_hz = entry.freq_hz;
    squelch = entry.squelch;
    filter_bw = entry.filter_bw;
    filter_shape = entry.filter_shape;
    offset_freq_hz = entry.offset_freq_hz;
}

#ifndef QT_NO_DEBUG_OUTPUT
QDebug operator<<(QDebug dbg, const FreqHistoryEntry &entry)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "freq_hz: " << entry.freq_hz;
    return dbg;
}
#endif
