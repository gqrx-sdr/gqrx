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
#ifndef RX_METER_H
#define RX_METER_H

#include <gnuradio/sync_block.h>
#include <gnuradio/buffer.h>
#if GNURADIO_VERSION >= 0x031000
#include <gnuradio/buffer_reader.h>
#endif
#include <chrono>
#include <mutex>


class rx_meter_c;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_meter_c> rx_meter_c_sptr;
#else
typedef std::shared_ptr<rx_meter_c> rx_meter_c_sptr;
#endif


/*! \brief Return a shared_ptr to a new instance of rx_meter_c.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_meter_c constructor is private.
 * make_rxfilter is the public interface for creating new instances.
 */
rx_meter_c_sptr make_rx_meter_c(double quad_rate);


/*! \brief Block for measuring signal strength (complex input).
 *  \ingroup DSP
 *
 * This block can be used to measure the received signal strength.
 * The get_level_db() method returns the average signal power
 * over a 100ms period.
 */
class rx_meter_c : public gr::sync_block
{
    friend rx_meter_c_sptr make_rx_meter_c(double quad_rate);

protected:
    rx_meter_c(double quad_rate);

public:
    ~rx_meter_c();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    /*! \brief Get the current signal level in dBFS. */
    float get_level_db();

private:
    double d_quadrate;
    unsigned int d_avgsize; /*! Number of samples to average. */

    gr::buffer_sptr d_writer; /*! buffer to accumulate samples. */
    gr::buffer_reader_sptr d_reader;
    std::chrono::time_point<std::chrono::steady_clock> d_lasttime;

    std::mutex   d_mutex;  /*! Used to lock FFT output buffer. */
};


#endif /* RX_METER_H */
