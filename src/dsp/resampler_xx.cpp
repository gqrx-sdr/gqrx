/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#include <cstdio>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include "dsp/resampler_xx.h"


/* Create a new instance of resampler_cc and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
resampler_cc_sptr make_resampler_cc(float rate)
{
    return gnuradio::get_initial_sptr(new resampler_cc(rate));
}

resampler_cc::resampler_cc(float rate)
    : gr::hier_block2 ("resampler_cc",
          gr::io_signature::make (1, 1, sizeof(gr_complex)),
          gr::io_signature::make (1, 1, sizeof(gr_complex)))
{
    /* I ceated this code based on:
       http://gnuradio.squarespace.com/blog/2010/12/6/new-interface-for-pfb_arb_resampler_ccf.html

       and blks2.pfb_arb_resampler.py

       Note: In case of decimation, we limit the cutoff to the output bandwidth to avoid "phantom"
             signals when we have a frequency translation in front of the PFB resampler.
    */

    /* generate taps */
    double cutoff = rate > 1.0 ? 0.4 : 0.4*rate;
    double trans_width = rate > 1.0 ? 0.2 : 0.2*rate;
    unsigned int flt_size = 32;

    d_taps = gr::filter::firdes::low_pass(flt_size, flt_size, cutoff, trans_width);

    /* create the filter */
    d_filter = gr::filter::pfb_arb_resampler_ccf::make(rate, d_taps, flt_size);

    /* connect filter */
    connect(self(), 0, d_filter, 0);
    connect(d_filter, 0, self(), 0);
}

resampler_cc::~resampler_cc()
{

}

void resampler_cc::set_rate(float rate)
{
    /* generate taps */
    double cutoff = rate > 1.0 ? 0.4 : 0.4*rate;
    double trans_width = rate > 1.0 ? 0.2 : 0.2*rate;
    unsigned int flt_size = 32;
    d_taps = gr::filter::firdes::low_pass(flt_size, flt_size, cutoff, trans_width);

    /* FIXME: Should implement set_taps() in PFB */
    lock();
    disconnect(self(), 0, d_filter, 0);
    disconnect(d_filter, 0, self(), 0);
    d_filter.reset();
    d_filter = gr::filter::pfb_arb_resampler_ccf::make(rate, d_taps, flt_size);
    connect(self(), 0, d_filter, 0);
    connect(d_filter, 0, self(), 0);
    unlock();
}

/* Create a new instance of resampler_ff and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
resampler_ff_sptr make_resampler_ff(float rate)
{
    return gnuradio::get_initial_sptr(new resampler_ff(rate));
}

resampler_ff::resampler_ff(float rate)
    : gr::hier_block2 ("resampler_ff",
          gr::io_signature::make (1, 1, sizeof(float)),
          gr::io_signature::make (1, 1, sizeof(float)))
{
    /* I ceated this code based on:
       http://gnuradio.squarespace.com/blog/2010/12/6/new-interface-for-pfb_arb_resampler_ccf.html

       and blks2.pfb_arb_resampler.py

       Note: In case of decimation, we limit the cutoff to the output bandwidth to avoid "phantom"
             signals when we have a frequency translation in front of the PFB resampler.
    */

    /* generate taps */
    double cutoff = rate > 1.0 ? 0.4 : 0.4*rate;
    double trans_width = rate > 1.0 ? 0.2 : 0.2*rate;
    unsigned int flt_size = 32;

    d_taps = gr::filter::firdes::low_pass(flt_size, flt_size, cutoff, trans_width);

    /* create the filter */
    d_filter = gr::filter::pfb_arb_resampler_fff::make(rate, d_taps, flt_size);

    /* connect filter */
    connect(self(), 0, d_filter, 0);
    connect(d_filter, 0, self(), 0);
}

resampler_ff::~resampler_ff()
{

}

void resampler_ff::set_rate(float rate)
{
    /* generate taps */
    double cutoff = rate > 1.0 ? 0.4 : 0.4*rate;
    double trans_width = rate > 1.0 ? 0.2 : 0.2*rate;
    unsigned int flt_size = 32;
    d_taps = gr::filter::firdes::low_pass(flt_size, flt_size, cutoff, trans_width);

    /* FIXME: Should implement set_taps() in PFB */
    lock();
    disconnect(self(), 0, d_filter, 0);
    disconnect(d_filter, 0, self(), 0);
    d_filter.reset();
    d_filter = gr::filter::pfb_arb_resampler_fff::make(rate, d_taps, flt_size);
    connect(self(), 0, d_filter, 0);
    connect(d_filter, 0, self(), 0);
    unlock();
}
