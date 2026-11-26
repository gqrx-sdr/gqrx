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
#include "dsp/tx_mod_ssb.h"

tx_mod_ssb_sptr make_tx_mod_ssb(unsigned int audio_rate,
                                 double       rf_rate,
                                 bool         is_usb)
{
    return tx_mod_ssb::make(audio_rate, rf_rate, is_usb);
}

tx_mod_ssb_sptr tx_mod_ssb::make(unsigned int audio_rate,
                                   double       rf_rate,
                                   bool         is_usb)
{
    return tx_mod_ssb_sptr(new tx_mod_ssb(audio_rate, rf_rate, is_usb));
}

tx_mod_ssb::tx_mod_ssb(unsigned int audio_rate,
                         double       rf_rate,
                         bool         is_usb)
    : tx_modulator_base("tx_mod_ssb", audio_rate, rf_rate),
      d_is_usb(is_usb)
{
    /* 65 taps — standard for narrowband audio (must be odd) */
    d_hilbert = gr::filter::hilbert_fc::make(65);

    /* Resampler: audio_rate → rf_rate */
    int interp = static_cast<int>(rf_rate);
    int decim  = static_cast<int>(audio_rate);
    int a = interp, b = decim;
    while (b) { int t = b; b = a % b; a = t; }
    interp /= a; decim /= a;
    d_resamp = gr::filter::rational_resampler_ccc::make(interp, decim);

    if (is_usb) {
        /* USB: analytic signal from hilbert_fc is already upper sideband */
        connect(self(),      0, d_hilbert, 0);
        connect(d_hilbert,   0, d_resamp,  0);
        connect(d_resamp,    0, self(),    0);
    } else {
        /* LSB: conjugate the analytic signal to mirror around 0 Hz.
         * Multiplying by gr_complex(1, -1) conjugates every sample. */
        d_conj = gr::blocks::multiply_const_cc::make(gr_complex(1.0f, -1.0f));
        connect(self(),      0, d_hilbert, 0);
        connect(d_hilbert,   0, d_conj,    0);
        connect(d_conj,      0, d_resamp,  0);
        connect(d_resamp,    0, self(),    0);
    }
}

