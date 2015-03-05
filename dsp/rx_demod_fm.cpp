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
    d_fftaps.resize(2);
    d_fbtaps.resize(2);
    calculate_iir_taps(d_tau);
    d_deemph = gr::filter::iir_filter_ffd::make(d_fftaps, d_fbtaps);

    /* connect block */
    connect(self(), 0, d_quad, 0);
    if (d_tau > 1.0e-9) {
        connect(d_quad, 0, d_deemph, 0);
        connect(d_deemph, 0, self(), 0);
    }
    else {
        connect(d_quad, 0, self(), 0);
    }

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
        /* no change */
        return;
    }

    if (tau > 1.0e-9) {
        calculate_iir_taps(tau);
        d_deemph->set_taps(d_fftaps, d_fbtaps);

        /* check to see if we need to rewire flow graph */
        if (d_tau <= 1.0e-9) {
            /* need to put deemph into the flowgraph */
            lock();
            disconnect(d_quad, 0, self(), 0);
            connect(d_quad, 0, d_deemph, 0);
            connect(d_deemph, 0, self(), 0);
            unlock();
        }

        d_tau = tau;
    }
    else {
        //std::cout << "TAU is 0: " << tau << std::endl;
        /* diable de-emph if conencted */
        if (d_tau > 1.0e-9) {
            //std::cout << "  Disable deemph" << std::endl;
            lock();
            disconnect(d_quad, 0, d_deemph, 0);
            disconnect(d_deemph, 0, self(), 0);
            connect(d_quad, 0, self(), 0);
            unlock();
        }

        d_tau = 0.0;
    }

}


/*! \brief Calculate taps for FM de-emph IIR filter. */
void rx_demod_fm::calculate_iir_taps(double tau)
{
    /* copied from fm_emph.py in gnuradio-core */
    double w_p, w_pp;

    w_p = 1.0/tau;
    w_pp = tan(w_p / (d_quad_rate * 2.0)); /* prewarped analog freq */

    d_fftaps[0] = w_pp/(1 + w_pp);
    d_fftaps[1] = d_fftaps[0];

    d_fbtaps[0] = 1.0;
    d_fbtaps[1] = (w_pp - 1)/(w_pp + 1);
}
