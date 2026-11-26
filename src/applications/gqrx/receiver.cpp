/* -*- c++ -*- */
/* 
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 * https://gqrx.dk/
 * 
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
 * 
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License published by
 * the Free Software Foundation, either version 3, or (at your option)
 * any later version.
 * 
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <cmath>
#include <iostream>
#include <iomanip>
#include <QDebug>

#include <gnuradio/prefs.h>
#include <gnuradio/top_block.h>
#include <osmosdr/source.h>

#include "applications/gqrx/receiver.h"
#include "dsp/filter/fir_decim.h"
#include "receivers/nbrx.h"
#include "receivers/wfmrx.h"

#ifdef WITH_PULSEAUDIO
#include "portaudio/pulse_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#endif
#include "gnuradio/audio/sink.h"

#define DEFAULT_AUDIO_GAIN -6.0
#define TARGET_QUAD_RATE 1e6

/**
 * @brief Stop data sniffer.
 * @return STATUS_ERROR if the sniffer is not currently active.
 */
receiver::status receiver::stop_sniffer()
{
    if (!d_sniffer_active) {
        return STATUS_ERROR;
    }

    try {
        tb->lock();
        tb->disconnect(rx, 0, sniffer_rr, 0);
        
        // Temporary workaround for https://github.com/gnuradio/gnuradio/issues/5436
        tb->disconnect(ddc, 0, rx, 0);
        tb->connect(ddc, 0, rx, 0);
        // End temporary workaround
        
        tb->disconnect(sniffer_rr, 0, sniffer, 0);
        tb->unlock();
    } catch (...) {
        try {
            tb->unlock();
        } catch (...) {}
        d_sniffer_active = false;
        sniffer_rr.reset();
        return STATUS_ERROR;
    }

    d_sniffer_active = false;
    /* delete resampler */
    sniffer_rr.reset();
    return STATUS_OK;
}
