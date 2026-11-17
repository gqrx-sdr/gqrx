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

#include "dsp/tx_mod_am.h"


/* Create a new instance of tx_mod_am and return a shared_ptr. */
tx_mod_am_sptr make_tx_mod_am(float audio_rate, float mod_index)
{
    return gnuradio::get_initial_sptr(new tx_mod_am(audio_rate, mod_index));
}

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

/* Limits of the voice band-pass filter. */
static const float TX_AUDIO_LOW = 300.0f;    /* Lower audio cutoff in Hz */
static const float TX_AUDIO_HIGH = 3000.0f;  /* Upper audio cutoff in Hz */
static const float TX_AUDIO_TRANS = 400.0f;  /* Transition width in Hz */

tx_mod_am::tx_mod_am(float audio_rate, float mod_index)
    : gr::hier_block2 ("tx_mod_am",
                       gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                       gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
    d_audio_rate(audio_rate),
    d_mod_index(mod_index)
{
    /* voice band-pass filter */
    std::vector<float> taps = gr::filter::firdes::band_pass(
        1.0, d_audio_rate, TX_AUDIO_LOW, TX_AUDIO_HIGH, TX_AUDIO_TRANS);
    d_bpf = gr::filter::fft_filter_fff::make(1, taps);

    /* limiter: keeps the envelope within modulation limits */
    d_limit = gr::analog::rail_ff::make(-1.0, 1.0);

    /* modulation */
    d_scale = gr::blocks::multiply_const_ff::make(d_mod_index);
    d_carrier = gr::blocks::add_const_ff::make(1.0f);
    d_f2c = gr::blocks::float_to_complex::make(1);

    /* connect blocks */
    connect(self(), 0, d_bpf, 0);
    connect(d_bpf, 0, d_limit, 0);
    connect(d_limit, 0, d_scale, 0);
    connect(d_scale, 0, d_carrier, 0);
    connect(d_carrier, 0, d_f2c, 0);
    connect(d_f2c, 0, self(), 0);
}

tx_mod_am::~tx_mod_am ()
{
}

/*! \brief Set modulation index.
 *  \param mod_index The new modulation index; 1.0 is 100% modulation.
 *
 * Values above 1.0 would cause overmodulation and are clamped.
 */
void tx_mod_am::set_mod_index(float mod_index)
{
    if (mod_index > 1.0f)
        mod_index = 1.0f;
    else if (mod_index < 0.0f)
        mod_index = 0.0f;

    d_mod_index = mod_index;
    d_scale->set_k(d_mod_index);
}
