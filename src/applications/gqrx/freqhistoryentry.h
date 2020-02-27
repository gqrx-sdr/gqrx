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
#ifndef FREQHISTORYENTRY_H
#define FREQHISTORYENTRY_H

#ifndef QT_NO_DEBUG_OUTPUT
#include <QtDebug>
#endif
#include <QtGlobal>

#include "qtgui/dockrxopt.h"

class FreqHistoryEntry
{
public:
    FreqHistoryEntry();
    FreqHistoryEntry(const FreqHistoryEntry &entry);
    FreqHistoryEntry &operator=(const FreqHistoryEntry &entry);
    bool operator==(const FreqHistoryEntry &entry);
    bool operator!=(const FreqHistoryEntry &entry);

    DockRxOpt::rxopt_mode_idx demod;
    qint64 filter_bw;
    int filter_shape;
    qint64 freq_hz;
    qint64 min_hz, max_hz;
    qint64 offset_freq_hz;
    double squelch;

protected:
    void assign(const FreqHistoryEntry &entry);
};

#ifndef QT_NO_DEBUG_OUTPUT
QDebug operator<<(QDebug dbg, const FreqHistoryEntry &entry);
#endif

#endif // FREQHISTORYENTRY_H
