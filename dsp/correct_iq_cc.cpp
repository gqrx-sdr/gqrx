/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#include <gnuradio/gr_complex.h>
#include <iostream>
#include "dsp/correct_iq_cc.h"


dc_corr_cc_sptr make_dc_corr_cc(double sample_rate, double tau)
{
    return gnuradio::get_initial_sptr(new dc_corr_cc(sample_rate, tau));
}


/*! \brief Create DC correction object.
 *
 * Use make_dc_corr_cc() instead.
 */
dc_corr_cc::dc_corr_cc(double sample_rate, double tau)
    : gr::hier_block2 ("dc_corr_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    d_sr = sample_rate;
    d_tau = tau;
    d_alpha = 1.0 / (1.0 + d_tau * sample_rate);

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "IQ DCR alpha: " << d_alpha << std::endl;
#endif

    d_iir = gr::filter::single_pole_iir_filter_cc::make(d_alpha, 1);
    d_sub = gr::blocks::sub_cc::make(1);

    connect(self(), 0, d_iir, 0);
    connect(self(), 0, d_sub, 0);
    connect(d_iir, 0, d_sub, 1);
    connect(d_sub, 0, self(), 0);
}

dc_corr_cc::~dc_corr_cc()
{

}

/*! \brief Set new sample rate. */
void dc_corr_cc::set_sample_rate(double sample_rate)
{
    d_sr = sample_rate;
    d_alpha = 1.0 / (1.0 + d_tau * sample_rate);

    d_iir->set_taps(d_alpha);

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "IQ DCR samp_rate: " << sample_rate << std::endl;
    std::cout << "IQ DCR alpha: " << d_alpha << std::endl;
#endif
}

/*! \brief Set new time constant. */
void dc_corr_cc::set_tau(double tau)
{
    d_tau = tau;
    d_alpha = 1.0 / (1.0 + d_tau * d_sr);

    d_iir->set_taps(d_alpha);

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "IQ DCR alpha: " << d_alpha << std::endl;
#endif
}


/** I/Q swap **/
iq_swap_cc_sptr make_iq_swap_cc(bool enabled)
{
    return gnuradio::get_initial_sptr(new iq_swap_cc(enabled));
}

iq_swap_cc::iq_swap_cc(bool enabled)
    : gr::hier_block2 ("iq_swap_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    d_enabled = enabled;
    d_c2f = gr::blocks::complex_to_float::make();
    d_f2c = gr::blocks::float_to_complex::make();

    connect(self(), 0, d_c2f, 0);
    if (enabled)
    {
        connect(d_c2f, 0, d_f2c, 1);
        connect(d_c2f, 1, d_f2c, 0);
    }
    else
    {
        connect(d_c2f, 0, d_f2c, 0);
        connect(d_c2f, 1, d_f2c, 1);
    }
    connect(d_f2c, 0, self(), 0);
}

iq_swap_cc::~iq_swap_cc()
{

}

/*! \brief Enabled or disable I/Q swapping. */
void iq_swap_cc::set_enabled(bool enabled)
{
    if (enabled == d_enabled)
        return;

#ifndef QT_NO_DEBUG_OUTPUT
    std::cout << "IQ swap: " << enabled << std::endl;
#endif

    d_enabled = enabled;

    lock();
    if (d_enabled)
    {
        disconnect(d_c2f, 0, d_f2c, 0);
        disconnect(d_c2f, 1, d_f2c, 1);
        connect(d_c2f, 0, d_f2c, 1);
        connect(d_c2f, 1, d_f2c, 0);
    }
    else
    {
        disconnect(d_c2f, 0, d_f2c, 1);
        disconnect(d_c2f, 1, d_f2c, 0);
        connect(d_c2f, 0, d_f2c, 0);
        connect(d_c2f, 1, d_f2c, 1);
    }

/** DEBUG **/
/** disconnect_all() does not work here **/
/*
    disconnect_all();
    connect(self(), 0, d_c2f, 0);
    if (enabled)
    {
        connect(d_c2f, 0, d_f2c, 1);
        connect(d_c2f, 1, d_f2c, 0);
    }
    else
    {
        connect(d_c2f, 0, d_f2c, 0);
        connect(d_c2f, 1, d_f2c, 1);
    }
    connect(d_f2c, 0, self(), 0);
*/
    unlock();
}
