/* -*- c++ -*- */
/*
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
#include <gr_io_signature.h>
#include <gr_firdes.h>
#include "dsp/resampler_xx.h"


/*
 * Create a new instance of resampler_cc and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
resampler_cc_sptr make_resampler_cc(float rate)
{
    return gnuradio::get_initial_sptr(new resampler_cc(rate));
}

resampler_cc::resampler_cc(float rate)
    : gr_hier_block2 ("resampler_cc",
          gr_make_io_signature (1, 1, sizeof(gr_complex)),
          gr_make_io_signature (1, 1, sizeof(gr_complex)))
{
    /* I ceated this code based on:
       http://gnuradio.squarespace.com/blog/2010/12/6/new-interface-for-pfb_arb_resampler_ccf.html

       and blks2.pfb_arb_resampler.py
    */

    /* generate taps */
    double cutoff = 0.4;
    double trans_width = 0.2;
    unsigned int flt_size = 32;

    d_taps = gr_firdes::low_pass(flt_size, 2*flt_size, cutoff, trans_width);

    /* create the filter */
    d_filter = gr_make_pfb_arb_resampler_ccf(rate, d_taps, flt_size);

    /* connect filter */
    connect(self(), 0, d_filter, 0);
    connect(d_filter, 0, self(), 0);
}

resampler_cc::~resampler_cc()
{

}
