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
#include <dsp/rx_meter.h>
#include <iostream>


rx_meter_c_sptr make_rx_meter_c (bool use_avg)
{
    return gnuradio::get_initial_sptr(new rx_meter_c (use_avg));
}



rx_meter_c::rx_meter_c(bool use_avg)
    : gr_sync_block ("rx_meter_c",
          gr_make_io_signature(1, 1, sizeof(gr_complex)),
          gr_make_io_signature(0, 0, 0)),
      d_use_avg(use_avg),
      d_level(0.0),
      d_level_db(0.0),
      d_fs(1.0)
{

}

rx_meter_c::~rx_meter_c()
{
}


#define ALPHA 0.4

int rx_meter_c::work (int noutput_items,
                      gr_vector_const_void_star &input_items,
                      gr_vector_void_star &output_items)
{
    const gr_complex *in = (const gr_complex *) input_items[0];
    float sum;
    float pwr = 0.0;
    int   i;

    //std::cout << "NUM: " << noutput_items << std::endl;

    sum = in[0].real()*in[0].real() + in[0].imag()*in[0].imag();

    /* find the maximum power in this set of samples */
    for (i = 1; i < noutput_items; i++) {
        /* calculate power as amplitude squared */
        pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
        sum = ALPHA*pwr + (1.0-ALPHA)*sum;
    }

    if (d_use_avg && (d_level > 0.0)) {
        d_level = (d_level + sum) / 2.0;
    }
    else {
        d_level = sum;
    }

    d_level_db = (float) 10. * log10(d_level / d_fs + 1.0e-20);

    return i;
}


float rx_meter_c::get_level()
{
    float retval = d_level;

    /* reset averaging */
    d_level = 0.0;
    d_level_db = 0.0;

    return retval;
}

float rx_meter_c::get_level_db()
{
    float retval = d_level_db;

    /* reset averaging */
    d_level = 0.0;
    d_level_db = 0.0;

    return retval;
}
