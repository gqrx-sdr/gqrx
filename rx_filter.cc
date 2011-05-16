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
#include <rx_filter.h>


/*
 * Create a new instance of rx_filter and return
 * a boost shared_ptr. This is effectively the public constructor.
 */
rx_filter_sptr make_rx_filter(float sample_rate, float offset, float low, float high, float trans_width)
{
    return gnuradio::get_initial_sptr(new rx_filter(sample_rate, offset, low, high, trans_width));
}


static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */


rx_filter::rx_filter(float sample_rate, float offset, float low, float high, float trans_width)
    : gr_hier_block2 ("rx_filter",
                      gr_make_io_signature (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (gr_complex)))
{
    /* passthrough */
    connect(self(), 0, self(), 0);
}


rx_filter::~rx_filter ()
{

}


void rx_filter::set_offset(float offset)
{

}


void rx_filter::set_low(float low)
{

}


void rx_filter::set_high(float high)
{

}


void rx_filter::set_trans_width(float trans_width)
{

}


void rx_filter::set_param(float center, float low, float high, float trans_width)
{

}


void rx_filter::set_param(float center, float low, float high)
{

}


void rx_filter::set_param(float low, float high)
{

}

