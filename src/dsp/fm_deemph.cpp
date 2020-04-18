/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2020 Clayton Smith VE3IRR.
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
#include "dsp/fm_deemph.h"


/* Create a new instance of fm_deemph and return a boost shared_ptr. */
fm_deemph_sptr make_fm_deemph(float quad_rate, double tau)
{
    return gnuradio::get_initial_sptr(new fm_deemph(quad_rate, tau));
}

static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

fm_deemph::fm_deemph(float quad_rate, double tau)
    : gr::hier_block2 ("fm_deemph",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_quad_rate(quad_rate)
{
    /* de-emphasis */
    d_fftaps.resize(2);
    d_fbtaps.resize(2);
    calculate_iir_taps(tau);
    d_deemph = gr::filter::iir_filter_ffd::make(d_fftaps, d_fbtaps, false);

    connect(self(), 0, d_deemph, 0);
    connect(d_deemph, 0, self(), 0);
}

fm_deemph::~fm_deemph ()
{
}

/*! \brief Set FM de-emphasis time constant.
 *  \param tau The new time costant.
 */
void fm_deemph::set_tau(double tau)
{
    calculate_iir_taps(tau);
    d_deemph->set_taps(d_fftaps, d_fbtaps);
}

/*! \brief Calculate taps for FM de-emph IIR filter. */
void fm_deemph::calculate_iir_taps(double tau)
{
    if (tau > 1.0e-9)
    {
        // copied from fm_emph.py in gr-analog
        double  w_c;    // Digital corner frequency
        double  w_ca;   // Prewarped analog corner frequency
        double  k, z1, p1, b0;
        double  fs = d_quad_rate;

        w_c = 1.0 / tau;
        w_ca = 2.0 * fs * tan(w_c / (2.0 * fs));

        // Resulting digital pole, zero, and gain term from the bilinear
        // transformation of H(s) = w_ca / (s + w_ca) to
        // H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
        k = -w_ca / (2.0 * fs);
        z1 = -1.0;
        p1 = (1.0 + k) / (1.0 - k);
        b0 = -k / (1.0 - k);

        d_fftaps[0] = b0;
        d_fftaps[1] = -z1 * b0;
        d_fbtaps[0] = 1.0;
        d_fbtaps[1] = -p1;
    }
    else
    {
        d_fftaps[0] = 1.0;
        d_fftaps[1] = 0.0;
        d_fbtaps[0] = 0.0;
        d_fbtaps[1] = 0.0;
    }
}
