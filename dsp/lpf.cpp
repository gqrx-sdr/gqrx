/* -*- c++ -*- */
/*
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#include <cmath>
#include <gr_io_signature.h>
#include <gr_firdes.h>
#include "dsp/lpf.h"

static const int MIN_IN  = 1; /* Mininum number of input streams. */
static const int MAX_IN  = 1; /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */


/*
 * Create a new instance of lpf and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
lpf_ff_sptr make_lpf_ff(double sample_rate, double cutoff_freq,
                        double trans_width, double gain)
{
    return gnuradio::get_initial_sptr(new lpf_ff(sample_rate, cutoff_freq,
                                                 trans_width, gain));
}


lpf_ff::lpf_ff(double sample_rate, double cutoff_freq,
               double trans_width, double gain)
    : gr_hier_block2("lpf_ff",
                     gr_make_io_signature(MIN_IN,  MAX_IN,  sizeof (float)),
                     gr_make_io_signature(MIN_OUT, MAX_OUT, sizeof (float))),
    d_sample_rate(sample_rate),
    d_cutoff_freq(cutoff_freq),
    d_trans_width(trans_width),
    d_gain(gain)
{
    /* generate taps */
    d_taps = gr_firdes::low_pass(d_gain, d_sample_rate,
                                 d_cutoff_freq, d_trans_width);

    /* create low-pass filter (decimation=1) */
    lpf = gr_make_fir_filter_fff(1, d_taps);

    /* connect filter */
    connect(self(), 0, lpf, 0);
    connect(lpf, 0, self(), 0);
}


lpf_ff::~lpf_ff()
{

}


void lpf_ff::set_param(double cutoff_freq, double trans_width)
{
    d_cutoff_freq = cutoff_freq;
    d_trans_width = trans_width;

    /* generate new taps */
    d_taps = gr_firdes::low_pass(d_gain, d_sample_rate,
                                 d_cutoff_freq, d_trans_width);

    lpf->set_taps(d_taps);
}

