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
#include <dsp/sniffer_f.h>


/* Return a shared_ptr to a new instance of sniffer_f */
sniffer_f_sptr make_sniffer_f(int buffsize)
{
    return gnuradio::get_initial_sptr(new sniffer_f(buffsize));
}


/*! \brief Create a sniffe_fr object.
 *  \param buffsize The internal buffer size.
 *
 * When choosing buffer size, the user of this class should take into account:
 *  - The input sample rate.
 *  - How ofter the data will be popped.
 */
sniffer_f::sniffer_f(int buffsize)
    : gr::sync_block ("rx_fft_c",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(0, 0, 0)),
      d_minsamp(1000)
{

    /* allocate circular buffer */
    d_buffer.set_capacity(buffsize);

}

sniffer_f::~sniffer_f()
{

}


/*! \brief Work method.
 *  \param mooutput_items
 *  \param input_items
 *  \param output_items
 *
 * This method does nothing except dumping the incoming samples into the
 * circular buffer.
 */
int sniffer_f::work(int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    int i;
    const float *in = (const float *)input_items[0];

    (void) output_items;

    boost::mutex::scoped_lock lock(d_mutex);

    /* dump new samples into the buffer */
    for (i = 0; i < noutput_items; i++) {
        d_buffer.push_back(in[i]);
    }

    return noutput_items;
}


/*! \brief Get number of samples avaialble for fetching.
 *  \return The number of samples in the buffer.
 *
 * This method can be used to read how many samples are currently
 * stored in the buffer.
 */
int  sniffer_f::samples_available()
{
    boost::mutex::scoped_lock lock(d_mutex);

    return d_buffer.size();
}

/*! \brief Fetch avaialble samples.
 *  \param out Pointer to allocated memory where the samples will be copied.
 *             Should be at least as big as buffer_size().
 *  \param num The number of sampels returned.
 */
void sniffer_f::get_samples(float * out, unsigned int &num)
{
    boost::mutex::scoped_lock lock(d_mutex);

    if (d_buffer.size() < d_minsamp) {
        /* not enough samples in buffer */
        num = 0;
        return;
    }

    num = d_buffer.size();
    float *buff = d_buffer.linearize();

    memcpy(out, buff, sizeof(float)*num);

    d_buffer.clear();
}


/*! \brief Resize internal buffer.
 *  \param newsize The new size of the buffer (number of samples, not bytes)
 */
void sniffer_f::set_buffer_size(int newsize)
{
    boost::mutex::scoped_lock lock(d_mutex);

    //d_buffer.clear();
    d_buffer.set_capacity(newsize);
}


/*! \brief Get current size of the internal buffer.
 *
 * This number equals the largest number of samples that can be returned by
 * get_samples().
 */
int  sniffer_f::buffer_size()
{
    boost::mutex::scoped_lock lock(d_mutex);

    return d_buffer.capacity();
}
