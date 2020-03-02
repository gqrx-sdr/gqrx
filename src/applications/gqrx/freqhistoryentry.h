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
    bool operator==(const FreqHistoryEntry &entry) const;
    bool operator!=(const FreqHistoryEntry &entry) const;
    bool equals(const FreqHistoryEntry &entry, bool only_freq) const;

    enum class Varname {
        demod, max_hz, min_hz, freq_hz, squelch, filter,
        filter_bw, filter_shape, offset_freq_hz
    };

    template<typename T>
    void set(const Varname varname, T value)
    {
        switch (varname) {
        case Varname::demod:
            demod = static_cast<DockRxOpt::rxopt_mode_idx>(value);
            break;
        case Varname::max_hz:
            max_hz = value;
            break;
        case Varname::min_hz:
            min_hz = value;
            break;
        case Varname::freq_hz:
            freq_hz = value;
            break;
        case Varname::squelch:
            squelch = value;
            break;
        case Varname::filter:
            filter = value;
            break;
        case Varname::filter_bw:
            filter_bw = value;
            break;
        case Varname::filter_shape:
            filter_shape = value;
            break;
        case Varname::offset_freq_hz:
            offset_freq_hz = value;
            break;
        default:
            throw "FreqHistoryEntry::set(...) varname not implemented";
            break;
        }
    }

    DockRxOpt::rxopt_mode_idx demod;
    int filter;
    qint64 filter_bw;
    int filter_shape;
    qint64 freq_hz;
    int min_hz, max_hz;
    qint64 offset_freq_hz;
    double squelch;

protected:
    void assign(const FreqHistoryEntry &entry);
};

#ifndef QT_NO_DEBUG_OUTPUT
QDebug operator<<(QDebug dbg, const FreqHistoryEntry &entry);
#endif

#endif // FREQHISTORYENTRY_H
