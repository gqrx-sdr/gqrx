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
#include <iostream>
#include <math.h>
#include <QDebug>

#include "dsp/tx_mod_nfm.h"


/* Create a new instance of tx_mod_nfm and return a shared_ptr. */
tx_mod_nfm_sptr make_tx_mod_nfm(float audio_rate, float max_dev)
{
    return gnuradio::get_initial_sptr(new tx_mod_nfm(audio_rate, max_dev));
}

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

/* Limits of the voice band-pass filter. */
static const float TX_AUDIO_LOW = 300.0f;    /* Lower audio cutoff in Hz */
static const float TX_AUDIO_HIGH = 3000.0f;  /* Upper audio cutoff in Hz */
static const float TX_AUDIO_TRANS = 400.0f;  /* Transition width in Hz */

/* Default CTCSS tone deviation as a fraction of the maximum deviation. */
static const float TX_CTCSS_DEV_FRAC = 0.15f;

/* Valid range for CTCSS tones (EIA standard tones are 67.0 - 250.3 Hz). */
static const float TX_CTCSS_MIN = 60.0f;
static const float TX_CTCSS_MAX = 260.0f;

tx_mod_nfm::tx_mod_nfm(float audio_rate, float max_dev)
    : gr::hier_block2 ("tx_mod_nfm",
                       gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                       gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
    d_audio_rate(audio_rate),
    d_max_dev(max_dev)
{
    /* voice band-pass filter */
    std::vector<float> taps = gr::filter::firdes::band_pass(
        1.0, d_audio_rate, TX_AUDIO_LOW, TX_AUDIO_HIGH, TX_AUDIO_TRANS);
    d_bpf = gr::filter::fft_filter_fff::make(1, taps);

    /* limiter: prevents overdriving the modulator */
    d_limit = gr::analog::rail_ff::make(-1.0, 1.0);

    /* CTCSS tone generator; disabled until set_ctcss() is called */
    d_ctcss = gr::analog::sig_source_f::make(d_audio_rate,
                                             gr::analog::GR_SIN_WAVE,
                                             100.0,
                                             0.0);
    d_adder = gr::blocks::add_ff::make();

    /* modulator gain is the inverse of the quadrature demod gain */
    float sens = 2 * (float)M_PI * d_max_dev / d_audio_rate;

    qDebug() << "NFM mod sens:" << sens;

    d_mod = gr::analog::frequency_modulator_fc::make(sens);

    /* connect blocks */
    connect(self(), 0, d_bpf, 0);
    connect(d_bpf, 0, d_limit, 0);
    connect(d_limit, 0, d_adder, 0);
    connect(d_ctcss, 0, d_adder, 1);
    connect(d_adder, 0, d_mod, 0);
    connect(d_mod, 0, self(), 0);
}

tx_mod_nfm::~tx_mod_nfm ()
{
}

/*! \brief Set maximum FM deviation.
 *  \param max_dev The new maximum deviation in Hz.
 *
 * The sensitivity of the frequency modulator is related to the
 * maximum deviation by:
 *
 *   sens = 2 * PI * max_dev / rate
 */
void tx_mod_nfm::set_max_dev(float max_dev)
{
    float sens;

    if ((max_dev < 500.0f) || (max_dev > d_audio_rate/2.0f))
    {
        return;
    }

    d_max_dev = max_dev;

    sens = 2 * (float)M_PI * max_dev / d_audio_rate;
    d_mod->set_sensitivity(sens);
}

/*! \brief Set CTCSS tone frequency.
 *  \param freq The tone frequency in Hz; 0 disables the tone.
 */
void tx_mod_nfm::set_ctcss(float freq)
{
    if ((freq > 0.0f) && (freq >= TX_CTCSS_MIN) && (freq <= TX_CTCSS_MAX))
    {
        /* tone deviation is a fraction of the maximum deviation */
        d_ctcss->set_frequency(freq);
        d_ctcss->set_amplitude(TX_CTCSS_DEV_FRAC);
    }
    else
    {
        d_ctcss->set_amplitude(0.0);
    }
}
