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
#pragma once

#include <gnuradio/filter/iir_filter_ffd.h>
#include <gnuradio/hier_block2.h>
#include <vector>

class fm_deemph;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<fm_deemph> fm_deemph_sptr;
#else
typedef std::shared_ptr<fm_deemph> fm_deemph_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of fm_deemph.
 *  \param quad_rate The input sample rate.
 *  \param tau De-emphasis time constant in seconds (75us in US, 50us in EUR, 0.0 disables).
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, fm_deemph's constructor is private.
 * make_rx_dmod_fm is the public interface for creating new instances.
 */
fm_deemph_sptr make_fm_deemph(float quad_rate, double tau=50.0e-6);

/*! \brief FM demodulator.
 *  \ingroup DSP
 *
 * This class implements the FM demodulator using the gr_quadrature_demod block.
 * It also provides de-emphasis with variable time constant (use 0.0 to disable).
 *
 */
class fm_deemph : public gr::hier_block2
{

public:
    fm_deemph(float quad_rate, double tau); // FIXME: should be private
    ~fm_deemph();

    void set_tau(double tau);

private:
    /* GR blocks */
    gr::filter::iir_filter_ffd::sptr        d_deemph;    /*! De-emphasis IIR filter. */

    /* other parameters */
    float       d_quad_rate;     /*! Quadrature rate. */

    /* De-emph IIR filter taps */
    std::vector<double> d_fftaps;  /*! Feed forward taps. */
    std::vector<double> d_fbtaps;  /*! Feed back taps. */

    void calculate_iir_taps(double tau);

};
