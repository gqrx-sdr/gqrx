/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx contributors.
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
#include <gnuradio/filter/firdes.h>
#include <gnuradio/io_signature.h>
#include <QDebug>

#include "dsp/tx_mod_ssb.h"


/* Create a new instance of tx_mod_ssb and return a shared_ptr. */
tx_mod_ssb_sptr make_tx_mod_ssb(float audio_rate, bool upper)
{
    return gnuradio::get_initial_sptr(new tx_mod_ssb(audio_rate, upper));
}

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

/* Limits of the voice band-pass filter. */
static const float TX_AUDIO_LOW = 300.0f;    /* Lower audio cutoff in Hz */
static const float TX_AUDIO_HIGH = 3000.0f;  /* Upper audio cutoff in Hz */
static const float TX_AUDIO_TRANS = 400.0f;  /* Transition width in Hz */

/* Number of taps for the Hilbert transform. */
static const unsigned int TX_HILBERT_TAPS = 65;

tx_mod_ssb::tx_mod_ssb(float audio_rate, bool upper)
    : gr::hier_block2 ("tx_mod_ssb",
                       gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                       gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
    d_audio_rate(audio_rate),
    d_upper(upper)
{
    /* voice band-pass filter */
    std::vector<float> taps = gr::filter::firdes::band_pass(
        1.0, d_audio_rate, TX_AUDIO_LOW, TX_AUDIO_HIGH, TX_AUDIO_TRANS);
    d_bpf = gr::filter::fft_filter_fff::make(1, taps);

    /* analytic signal generator */
    d_hilbert = gr::filter::hilbert_fc::make(TX_HILBERT_TAPS);

    d_conj = gr::blocks::conjugate_cc::make();

    /* connect blocks */
    connect(self(), 0, d_bpf, 0);
    connect(d_bpf, 0, d_hilbert, 0);

    if (d_upper)
    {
        /* USB: transmit the analytic signal as is */
        connect(d_hilbert, 0, self(), 0);
    }
    else
    {
        /* LSB: transmit the complex conjugate of the analytic signal */
        connect(d_hilbert, 0, d_conj, 0);
        connect(d_conj, 0, self(), 0);
    }
}

tx_mod_ssb::~tx_mod_ssb ()
{
}

/*! \brief Set sideband.
 *  \param upper Whether to generate USB (true) or LSB (false).
 */
void tx_mod_ssb::set_upper(bool upper)
{
    if (upper == d_upper)
        return;

    lock();
    if (d_upper)
    {
        /* switching from USB to LSB */
        disconnect(d_hilbert, 0, self(), 0);
        connect(d_hilbert, 0, d_conj, 0);
        connect(d_conj, 0, self(), 0);
    }
    else
    {
        /* switching from LSB to USB */
        disconnect(d_hilbert, 0, d_conj, 0);
        disconnect(d_conj, 0, self(), 0);
        connect(d_hilbert, 0, self(), 0);
    }
    unlock();

    d_upper = upper;
}
