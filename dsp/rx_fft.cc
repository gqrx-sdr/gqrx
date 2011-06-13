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
#include <math.h>
#include <gr_io_signature.h>
#include <dsp/rx_fft.h>


rx_fft_c_sptr make_rx_fft_c (int fftsize, int wintype, bool use_avg)
{
    return gnuradio::get_initial_sptr(new rx_fft_c (fftsize, wintype, use_avg));
}



rx_fft_c::rx_fft_c(int fftsize, int wintype, bool use_avg)
    : gr_sync_block ("rx_fft_c",
          gr_make_io_signature(1, 1, sizeof(gr_complex)),
          gr_make_io_signature(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(wintype),
      d_use_avg(use_avg)
{

}

rx_fft_c::~rx_fft_c()
{
}

int rx_fft_c::work (int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    float max = 0.0;
    float pwr = 0.0;
    int   i;

    /* find the maximum power in this set of samples */
    for (i = 0; i < noutput_items; i++) {
        /* do something cool */
    }

    return i;
}

