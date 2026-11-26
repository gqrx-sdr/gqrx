/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Contributors.
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
#include <cmath>
#include "dsp/tx_mod_nfm.h"

/* FM sensitivity: sensitivity = 2π * max_deviation / audio_rate
 * This maps audio amplitude ±1.0 to frequency deviation ±max_dev Hz. */
static inline float fm_sensitivity(float max_dev, unsigned int audio_rate)
{
    return 2.0f * static_cast<float>(M_PI) * max_dev
           / static_cast<float>(audio_rate);
}

tx_mod_nfm_sptr make_tx_mod_nfm(unsigned int audio_rate,
                                 double       rf_rate,
                                 float        max_dev)
{
    return tx_mod_nfm::make(audio_rate, rf_rate, max_dev);
}

tx_mod_nfm_sptr tx_mod_nfm::make(unsigned int audio_rate,
                                  double       rf_rate,
                                  float        max_dev)
{
    return tx_mod_nfm_sptr(new tx_mod_nfm(audio_rate, rf_rate, max_dev));
}

tx_mod_nfm::tx_mod_nfm(unsigned int audio_rate,
                         double       rf_rate,
                         float        max_dev)
    : tx_modulator_base("tx_mod_nfm", audio_rate, rf_rate),
      d_max_dev(max_dev)
{
    /* frequency_modulator_fc: float in → complex out at same rate */
    d_fm = gr::analog::frequency_modulator_fc::make(
                fm_sensitivity(max_dev, audio_rate));

    /* Upsample from audio_rate to rf_rate using rational resampler */
    int interp = static_cast<int>(rf_rate);
    int decim  = static_cast<int>(audio_rate);
    int a = interp, b = decim;
    while (b) { int t = b; b = a % b; a = t; }
    interp /= a; decim /= a;

    d_resamp = gr::filter::rational_resampler_ccc::make(interp, decim);

    /* Chain: audio_in → FM modulator → resampler → IQ out */
    connect(self(),    0, d_fm,    0);
    connect(d_fm,      0, d_resamp, 0);
    connect(d_resamp,  0, self(),  0);
}

void tx_mod_nfm::set_max_dev(float dev_hz)
{
    d_max_dev = dev_hz;
    d_fm->set_sensitivity(fm_sensitivity(dev_hz, d_audio_rate));
}
