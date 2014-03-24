/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
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
#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include "dsp/rx_noise_blanker_cc.h"

rx_nb_cc_sptr make_rx_nb_cc(double sample_rate, float thld1, float thld2)
{
    return gnuradio::get_initial_sptr(new rx_nb_cc(sample_rate, thld1, thld2));
}


/*! \brief Create noise blanker object.
 *
 * Use make_rx_nb_cc() instead.
 */
rx_nb_cc::rx_nb_cc(double sample_rate, float thld1, float thld2)
    : gr::sync_block ("rx_nb_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_nb1_on(false),
      d_nb2_on(false),
      d_sample_rate(sample_rate),
      d_thld_nb1(thld1),
      d_thld_nb2(thld2),
      d_avgmag_nb1(1.0),
      d_avgmag_nb2(1.0),
      d_delidx(2),
      d_sigidx(0),
      d_hangtime(0)
{
    memset(d_delay, 0, 8 * sizeof(gr_complex));
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

    boost::mutex::scoped_lock lock(d_mutex);

    // copy data into output buffer then perform the processing on that buffer
    for (i = 0; i < noutput_items; i++)
    {
        out[i] = in[i];
    }

    if (d_nb1_on)
    {
        process_nb1(out, noutput_items);
    }
    if (d_nb2_on)
    {
        process_nb2(out, noutput_items);
    }

    return noutput_items;
}

/*! \brief Perform noise blanker 1 processing.
 *  \param buf The data buffer holding gr_complex samples.
 *  \param num The number of samples in the buffer.
 *
 * Noise blanker 1 is the first noise blanker in the processing chain.
 * It is intended to reduce the effect of impulse type noise.
 *
 * FIXME: Needs different constants for higher sample rates?
 */
void rx_nb_cc::process_nb1(gr_complex *buf, int num)
{
    float cmag;
    gr_complex zero(0.0, 0.0);

    for (int i = 0; i < num; i++)
    {
        cmag = abs(buf[i]);
        d_delay[d_sigidx] = buf[i];
        d_avgmag_nb1 = 0.999*d_avgmag_nb1 + 0.001*cmag;

        if ((d_hangtime == 0) && (cmag > (d_thld_nb1*d_avgmag_nb1)))
            d_hangtime = 7;

        if (d_hangtime > 0)
        {
            buf[i] = zero;
            d_hangtime--;
        }
        else
        {
            buf[i] = d_delay[d_delidx];
        }

        d_sigidx = (d_sigidx + 7) & 7;
        d_delidx = (d_delidx + 7) & 7;
    }
}

/*! \brief Perform noise blanker 2 processing.
 *  \param buf The data buffer holding gr_complex samples.
 *  \param num The number of samples in the buffer.
 *
 * Noise blanker 2 is the second noise blanker in the processing chain.
 * It is intended to reduce non-pulse type noise (i.e. longer time constants).
 *
 * FIXME: Needs different constants for higher sample rates?
 */
void rx_nb_cc::process_nb2(gr_complex *buf, int num)
{
    float cmag;
    gr_complex c1(0.75);
    gr_complex c2(0.25);

    for (int i = 0; i < num; i++)
    {
        cmag = abs(buf[i]);
        d_avgsig = c1*d_avgsig + c2*buf[i];
        d_avgmag_nb2 = 0.999*d_avgmag_nb2 + 0.001*cmag;

        if (cmag > d_thld_nb2*d_avgmag_nb2)
            buf[i] = d_avgsig;
    }
}

void rx_nb_cc::set_threshold1(float threshold)
{
    if ((threshold >= 1.0) && (threshold <= 20.0))
        d_thld_nb1 = threshold;
}

void rx_nb_cc::set_threshold2(float threshold)
{
    if ((threshold >= 0.0) && (threshold <= 15.0))
        d_thld_nb2 = threshold;
}
