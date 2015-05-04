/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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


rx_meter_c_sptr make_rx_meter_c (int detector)
{
    return gnuradio::get_initial_sptr(new rx_meter_c (detector));
}

rx_meter_c::rx_meter_c(int detector)
    : gr::sync_block ("rx_meter_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, 0, 0)),
      d_detector(detector),
      d_level(0.0),
      d_level_db(0.0),
      d_sum(0.0),
      d_sumsq(0.0),
      d_num(0)
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
    (void) output_items; // unused

    const gr_complex *in = (const gr_complex *) input_items[0];
    float pwr = 0.0;
    int   i = 0;

    if (d_num == 0)
    {
        // first sample after a reset
        d_level = in[0].real()*in[0].real() + in[0].imag()*in[0].imag();
        d_sum = d_level;
        d_sumsq = d_level*d_level;
        i = 1;
    }

    d_num += noutput_items;

    // processing depends on detector type
    switch (d_detector)
    {
    case DETECTOR_TYPE_SAMPLE:
        // just take the first sample
        d_level = in[0].real()*in[0].real() + in[0].imag()*in[0].imag();
        break;

    case DETECTOR_TYPE_MIN:
        // minimum peak
        while (i < noutput_items)
        {
            pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
            if (pwr < d_level)
                d_level = pwr;
            i++;
        }
        break;

    case DETECTOR_TYPE_MAX:
        // maximum peak
        while (i < noutput_items)
        {
            pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
            if (pwr > d_level)
                d_level = pwr;
            i++;
        }
        break;

    case DETECTOR_TYPE_AVG:
        // mean value
        while (i < noutput_items)
        {
            pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
            d_sum += pwr;
            i++;
        }
        d_level = d_sum / (float)(d_num);
        break;

    case DETECTOR_TYPE_RMS:
        // root mean square
        while (i < noutput_items)
        {
            pwr = in[i].real()*in[i].real() + in[i].imag()*in[i].imag();
            d_sumsq += pwr*pwr;
            i++;
        }
        d_level = sqrt(d_sumsq / (float)(d_num));
        break;

    default:
        std::cout << "Invalid detector type: " << d_detector << std::endl;
        std::cout << "Fallback to DETECTOR_TYPE_RMS." << std::endl;
        d_detector = DETECTOR_TYPE_RMS;
        break;
    }

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


void rx_meter_c::set_detector_type(int detector)
{
    if (d_detector == detector)
        return;

    d_detector = detector;
    reset_stats();
}

/*! \brief Reset statistics. */
void rx_meter_c::reset_stats()
{
    //d_level = 0.0;
    d_level_db = 0.0;
    d_sum = 0.0;
    d_sumsq = 0.0;
    d_num = 0;
}
