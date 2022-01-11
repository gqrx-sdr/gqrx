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
#include <volk/volk.h>
#include <gnuradio/io_signature.h>
#include <dsp/rx_meter.h>
#include <iostream>


rx_meter_c_sptr make_rx_meter_c(double quad_rate)
{
    return gnuradio::get_initial_sptr(new rx_meter_c(quad_rate));
}

rx_meter_c::rx_meter_c(double quad_rate)
    : gr::sync_block ("rx_meter_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, 0, 0)),
      d_quadrate(quad_rate),
      d_avgsize(quad_rate * 0.100)
{
    /* allocate circular buffer */
#if GNURADIO_VERSION < 0x031000
    d_writer = gr::make_buffer(d_avgsize + d_quadrate, sizeof(gr_complex));
#else
    d_writer = gr::make_buffer(d_avgsize + d_quadrate, sizeof(gr_complex), 1, 1);
#endif
    d_reader = gr::buffer_add_reader(d_writer, 0);

    d_lasttime = std::chrono::steady_clock::now();
}

rx_meter_c::~rx_meter_c()
{
}


int rx_meter_c::work(int noutput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    const gr_complex *in = (const gr_complex *) input_items[0];
    (void) output_items; // unused

    int items_to_copy = std::min(noutput_items, (int)d_writer->bufsize());
    if (items_to_copy < noutput_items)
        in += (noutput_items - items_to_copy);

    if (d_writer->space_available() < items_to_copy)
        d_reader->update_read_pointer(items_to_copy - d_writer->space_available());
    memcpy(d_writer->write_pointer(), in, sizeof(gr_complex) * items_to_copy);
    d_writer->update_write_pointer(items_to_copy);

    return noutput_items;
}


float rx_meter_c::get_level_db()
{
    std::lock_guard<std::mutex> lock(d_mutex);

    if ((unsigned int)d_reader->items_available() < d_avgsize)
        return 0;

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    d_lasttime = now;

    d_reader->update_read_pointer(std::min((unsigned int)(diff.count() * d_quadrate * 1.001), (unsigned int)d_reader->items_available() - d_avgsize));
    float sum = 0;
    volk_32f_x2_dot_prod_32f(&sum, (float *)d_reader->read_pointer(), (float *)d_reader->read_pointer(), d_avgsize * 2);
    float power = sum / (float)(d_avgsize);
    return (float) 10. * log10f(power + 1.0e-20);
}
