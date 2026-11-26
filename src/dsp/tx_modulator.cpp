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
#include "dsp/tx_modulator.h"

tx_modulator_base::tx_modulator_base(const std::string &name,
                                     unsigned int       audio_rate,
                                     double             rf_rate)
    : gr::hier_block2(name,
                      gr::io_signature::make(1, 1, sizeof(float)),        // in: float audio
                      gr::io_signature::make(1, 1, sizeof(gr_complex))),  // out: complex IQ
      d_audio_rate(audio_rate),
      d_rf_rate(rf_rate)
{
}

void tx_modulator_base::set_audio_rate(unsigned int rate)
{
    d_audio_rate = rate;
}

void tx_modulator_base::set_rf_rate(double rate)
{
    d_rf_rate = rate;
}

