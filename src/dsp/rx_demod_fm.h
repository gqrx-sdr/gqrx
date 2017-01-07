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
#pragma once

#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/filter/iir_filter_ffd.h>
#include <gnuradio/hier_block2.h>
#include <vector>

class rx_demod_fm;
typedef boost::shared_ptr<rx_demod_fm> rx_demod_fm_sptr;

/*! \brief Return a shared_ptr to a new instance of rx_demod_fm.
 *  \param quad_rate The input sample rate.
 *  \param max_dev Maximum deviation in Hz
 *  \param tau De-emphasis time constant in seconds (75us in US, 50us in EUR, 0.0 disables).
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, rx_demod_fm's constructor is private.
 * make_rx_dmod_fm is the public interface for creating new instances.
 */
rx_demod_fm_sptr make_rx_demod_fm(float quad_rate, float max_dev=5000.0, double tau=50.0e-6);

/*! \brief FM demodulator.
 *  \ingroup DSP
 *
 * This class implements the FM demodulator using the gr_quadrature_demod block.
 * It also provides de-emphasis with variable time constant (use 0.0 to disable).
 *
 */
class rx_demod_fm : public gr::hier_block2
{

public:
    rx_demod_fm(float quad_rate, float max_dev, double tau); // FIXME: should be private
    ~rx_demod_fm();

    void set_max_dev(float max_dev);
    void set_tau(double tau);

private:
    /* GR blocks */
    gr::analog::quadrature_demod_cf::sptr   d_quad;      /*! The quadrature demodulator block. */
    gr::filter::iir_filter_ffd::sptr        d_deemph;    /*! De-emphasis IIR filter. */
    std::vector<float>            d_taps;      /*! Taps for the PFB resampler. */

    /* other parameters */
    float       d_quad_rate;     /*! Quadrature rate. */
    float       d_max_dev;       /*! Max deviation. */
    double      d_tau;           /*! De-emphasis time constant. */

    /* De-emph IIR filter taps */
    std::vector<double> d_fftaps;  /*! Feed forward taps. */
    std::vector<double> d_fbtaps;  /*! Feed back taps. */

    void calculate_iir_taps(double tau);

};
