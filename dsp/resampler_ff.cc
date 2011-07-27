/* -*- c++ -*- */
/*
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
#include <cmath>
#include <gr_io_signature.h>
#include <gr_firdes.h>
#include <dsp/resampler_ff.h>


/*
 * Create a new instance of resampler_ff and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
resampler_ff_sptr make_resampler_ff(unsigned int input_rate, unsigned int output_rate)
{
    return gnuradio::get_initial_sptr(new resampler_ff(input_rate, output_rate));
}


static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */


resampler_ff::resampler_ff(unsigned int input_rate, unsigned int output_rate)
    : gr_hier_block2 ("resampler_ff",
                      gr_make_io_signature (MIN_IN, MAX_IN, sizeof (float)),
                      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (float)))
{

    /* calculate interpolation and decimation */
    d_interp = lcm(input_rate, output_rate) / input_rate;
    d_decim = lcm(input_rate, output_rate) / output_rate;

    /* generate taps */
    float fract_bw = 0.4;
    float trans_width = 0.5 - fract_bw;
    float mid_trans_band = 0.5 - trans_width/2.0;

    d_taps = gr_firdes::low_pass(d_interp,   // gain
                                 1.0,        // sampling freq
                                 mid_trans_band/d_interp,
                                 trans_width/d_interp,
                                 gr_firdes::WIN_KAISER,
                                 5.0     // beta
                                );

    /* create band pass filter */
    d_rrb = gr_make_rational_resampler_base_fff(d_interp, d_decim, d_taps);

    /* connect filter */
    connect(self(), 0, d_rrb, 0);
    connect(d_rrb, 0, self(), 0);
}


resampler_ff::~resampler_ff ()
{

}


/*! \brief Greatest common divisor. */
unsigned int resampler_ff::gcd(unsigned int a, unsigned int b)
{
    unsigned int c = a % b;

    while (c!= 0)
    {
        a = b;
        b = c;
        c = a % b;
    }

    return b;
}

/*! \brief Least common multiple. */
unsigned int resampler_ff::lcm(unsigned int a, unsigned int b)
{
    return (a*b) / gcd(a,b);
}
