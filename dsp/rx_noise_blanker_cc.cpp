/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
 * Copyright 2004-2008 by Frank Brickle, AB2KT and Bob McGwier, N4HY
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
#include <math.h>
#include <gr_io_signature.h>
#include <gr_complex.h>
#include "dsp/rx_noise_blanker_cc.h"

rx_nb_cc_sptr make_rx_nb_cc(double sample_rate, float threshold)
{
    return gnuradio::get_initial_sptr(new rx_nb_cc(sample_rate, threshold));
}


/*! \brief Create noise blanker object.
 *
 * Use make_rx_nb_cc() instead.
 */
rx_nb_cc::rx_nb_cc(double sample_rate, float threshold)
    : gr_sync_block ("rx_agc_cc",
          gr_make_io_signature(1, 1, sizeof(gr_complex)),
          gr_make_io_signature(1, 1, sizeof(gr_complex))),
      d_sample_rate(sample_rate),
      d_threshold(threshold),
      d_nb1_on(false),
      d_nb2_on(false)
{

}

rx_nb_cc::~rx_nb_cc()
{

}

/*! \brief Receiver noise blanker work method.
 *  \param mooutput_items
 *  \param input_items
 *  \param output_items
 */
int rx_nb_cc::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    gr_complex *out = (gr_complex *) output_items[0];
    int i;

    // lock mutex
    boost::mutex::scoped_lock lock(d_mutex);

    if (d_nb1_on)
    {
        // run noise blanker 1 processing
    }
    if (d_nb2_on)
    {
        // run noise blanker 1 processing
    }


    for (i = 0; i < noutput_items; i++) {
        out[i].imag() = in[i].imag();
        out[i].real() = in[i].real();
    }

    return noutput_items;
}
