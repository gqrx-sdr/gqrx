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
#include "dsp/tx_mod_am.h"

tx_mod_am_sptr make_tx_mod_am(unsigned int audio_rate,
                               double       rf_rate,
                               float        mod_index)
{
    return tx_mod_am::make(audio_rate, rf_rate, mod_index);
}

tx_mod_am_sptr tx_mod_am::make(unsigned int audio_rate,
                                double       rf_rate,
                                float        mod_index)
{
    return tx_mod_am_sptr(new tx_mod_am(audio_rate, rf_rate, mod_index));
}

tx_mod_am::tx_mod_am(unsigned int audio_rate,
                       double       rf_rate,
                       float        mod_index)
    : tx_modulator_base("tx_mod_am", audio_rate, rf_rate),
      d_mod_index(mod_index)
{
    /* Scale audio to ±mod_index then add 1.0 carrier */
    d_level   = gr::blocks::multiply_const_ff::make(mod_index);
    d_carrier = gr::blocks::add_const_ff::make(1.0f);
    d_f2c     = gr::blocks::float_to_complex::make(1);

    /* Resampler: audio_rate → rf_rate */
    int interp = static_cast<int>(rf_rate);
    int decim  = static_cast<int>(audio_rate);
    int a = interp, b = decim;
    while (b) { int t = b; b = a % b; a = t; }
    interp /= a; decim /= a;
    d_resamp = gr::filter::rational_resampler_ccc::make(interp, decim);

    connect(self(),     0, d_level,   0);
    connect(d_level,    0, d_carrier, 0);
    connect(d_carrier,  0, d_f2c,     0);
    connect(d_f2c,      0, d_resamp,  0);
    connect(d_resamp,   0, self(),    0);
}

void tx_mod_am::set_mod_index(float index)
{
    d_mod_index = index;
    d_level->set_k(index);
}

