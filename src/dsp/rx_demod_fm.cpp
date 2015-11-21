/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <dsp/rx_demod_fm.h>
#include <math.h>
#include <iostream>


/* Create a new instance of rx_demod_fm and return a boost shared_ptr. */
rx_demod_fm_sptr make_rx_demod_fm(float quad_rate, float audio_rate, float max_dev, double tau)
{
    return gnuradio::get_initial_sptr(new rx_demod_fm(quad_rate, audio_rate, max_dev, tau));
}

static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

rx_demod_fm::rx_demod_fm(float quad_rate, float audio_rate, float max_dev, double tau)
    : gr::hier_block2 ("rx_demod_fm",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_quad_rate(quad_rate),
    d_audio_rate(audio_rate),
    d_max_dev(max_dev),
    d_tau(tau)
{
    float gain;

    /* demodulator gain */
    gain = d_quad_rate / (2.0 * M_PI * d_max_dev);

    //std::cout << "G: " << gain << std::endl;

    /* demodulator */
    d_quad = gr::analog::quadrature_demod_cf::make(gain);

    /* de-emphasis */
    d_alpha = 1.0 - exp(-1.0/(d_quad_rate * d_tau));
    std::cout << " *** alpha: " << d_alpha << std::endl;
    d_deemph = gr::filter::single_pole_iir_filter_ff::make(d_alpha);

    /* connect block */
    connect(self(), 0, d_quad, 0);
    connect(d_quad, 0, d_deemph, 0);
    connect(d_deemph, 0, self(), 0);
}

rx_demod_fm::~rx_demod_fm ()
{
}

/*! \brief Set maximum FM deviation.
 *  \param max_dev The new mximum deviation in Hz
 *
 * The maximum deviation is related to the gain of the
 * quadrature demodulator by:
 *
 *   gain = quad_rate / (2 * PI * max_dev)
 */
void rx_demod_fm::set_max_dev(float max_dev)
{
    float gain;

    if ((max_dev < 500.0) || (max_dev > d_quad_rate/2.0)) {
        return;
    }

    d_max_dev = max_dev;

    gain = d_quad_rate / (2.0 * M_PI * max_dev);
    d_quad->set_gain(gain);
}

/*! \brief Set FM de-emphasis time constant.
 *  \param tau The new time costant.
 *
 * \bug Assumes that IIR filter has already been constructed so that we
 *      can use the set_taps() method.
 */
void rx_demod_fm::set_tau(double tau)
{
    if (fabs(tau - d_tau) < 1.0e-9) {
        return;
    }

    d_tau = tau;
    d_alpha = 1.0 - exp(-1.0/(d_quad_rate * d_tau));
    d_deemph->set_taps(d_alpha);
}
