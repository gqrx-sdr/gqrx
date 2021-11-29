/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#include <gnuradio/io_signature.h>
#include <dsp/rx_meter.h>
#include <iostream>


rx_meter_c_sptr make_rx_meter_c()
{
    return gnuradio::get_initial_sptr(new rx_meter_c());
}

rx_meter_c::rx_meter_c()
    : gr::sync_block ("rx_meter_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, 0, 0)),
      d_level(0.0),
      d_level_db(0.0),
      d_sum(0.0),
      d_num(0)
{

}

rx_meter_c::~rx_meter_c()
{
}


int rx_meter_c::work(int noutput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items)
{
    (void) output_items; // unused

    const gr_complex *in = (const gr_complex *) input_items[0];
    float pwr = 0.0;

    d_num += noutput_items;

    for (int i = 0; i < noutput_items; i++)
    {
        pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
        d_sum += pwr;
    }
    d_level = d_sum / (float)(d_num);

    d_level_db = (float) 10. * log10f(d_level + 1.0e-20);

    return noutput_items;
}


float rx_meter_c::get_level()
{
    float retval = d_level;
    reset_stats();

    return retval;
}

float rx_meter_c::get_level_db()
{
    float retval = d_level_db;
    reset_stats();

    return retval;
}

/*! \brief Reset statistics. */
void rx_meter_c::reset_stats()
{
    //d_level = 0.0;
    d_level_db = 0.0;
    d_sum = 0.0;
    d_num = 0;
}
