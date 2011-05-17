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
#include <gr_io_signature.h>
#include <gr_firdes.h>
#include <rx_filter.h>


/*
 * Create a new instance of rx_filter and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
rx_filter_sptr make_rx_filter(double sample_rate, double center, double low, double high, double trans_width)
{
    return gnuradio::get_initial_sptr(new rx_filter(sample_rate, center, low, high, trans_width));
}


static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */


rx_filter::rx_filter(double sample_rate, double center, double low, double high, double trans_width)
    : gr_hier_block2 ("rx_filter",
                      gr_make_io_signature (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
      d_sample_rate(sample_rate),
      d_center(center),
      d_low(low),
      d_high(high),
      d_trans_width(trans_width)
{
    /* generate taps */
    d_taps = gr_firdes::complex_band_pass(1.0, d_sample_rate, d_low, d_high, d_trans_width);

    /* create band pass filter */
    d_bpf = gr_make_freq_xlating_fir_filter_ccc(1, d_taps, d_center, d_sample_rate);

    /* connect filter */
    connect(self(), 0, d_bpf, 0);
    connect(d_bpf, 0, self(), 0);
}


rx_filter::~rx_filter ()
{

}


void rx_filter::set_offset(double center)
{
    d_center = center;
    d_bpf->set_center_freq(center);
}


void rx_filter::set_low(double low)
{

}


void rx_filter::set_high(double high)
{

}


void rx_filter::set_trans_width(double trans_width)
{

}


void rx_filter::set_param(double center, double low, double high, double trans_width)
{

}


void rx_filter::set_param(double center, double low, double high)
{

}


void rx_filter::set_param(double low, double high)
{

}

