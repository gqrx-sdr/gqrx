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
#ifndef SNIFFER_F_H
#define SNIFFER_F_H

#include <gnuradio/sync_block.h>
#include <boost/thread/mutex.hpp>
#include <boost/circular_buffer.hpp>


class sniffer_f;

typedef boost::shared_ptr<sniffer_f> sniffer_f_sptr;


/*! \brief Return a shared_ptr to a new instance of sniffer_f.
 *  \param buffsize The size of the buffer
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the constructor is private. This function is the public
 * interface for creating new instances.
 *
 */
sniffer_f_sptr make_sniffer_f(int buffsize=48000);


/*! \brief Simple sink to allow accessing data in the flow graph.
 *  \ingroup DSP
 *
 * This block can be used by external objects to access the data stream in the
 * flow graph. For example, a sniffer can be connected to the output of the demodulator
 * and used by data decoders.
 *
 * The class uses a circular buffer for internal storage and if the received samples
 * exceed the buffer size, old samples will be overwritten. The collected samples
 * can be accessed via the get_samples() method.
 */
class sniffer_f : public gr::sync_block
{
    friend sniffer_f_sptr make_sniffer_f(int buffsize);

protected:
    sniffer_f(int buffsize);

public:
    ~sniffer_f();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    int  samples_available();
    void get_samples(float * buffer, unsigned int &num);

    void set_buffer_size(int newsize);
    int  buffer_size();

    void set_min_samples(unsigned int num) {d_minsamp = num;}
    int min_samples() {return d_minsamp;}

private:

    boost::mutex d_mutex;                   /*! Used to prevent concurrent access to buffer. */
    boost::circular_buffer<float> d_buffer; /*! buffer to accumulate samples. */
    unsigned int d_minsamp;                 /*! smallest number of samples we want to return. */

};


#endif /* SNIFFER_F_H */
