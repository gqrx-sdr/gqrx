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
#include <dsp/sniffer_f.h>


/* Return a shared_ptr to a new instance of sniffer_f */
sniffer_f_sptr make_sniffer_f(int buffsize)
{
    return gnuradio::get_initial_sptr(new sniffer_f(buffsize));
}


/*! \brief Create a sniffer_f object.
 *  \param buffsize The internal buffer size.
 *
 * When choosing buffer size, the user of this class should take into account:
 *  - The input sample rate.
 *  - How often the data will be popped.
 */
sniffer_f::sniffer_f(int buffsize)
    : gr::sync_block ("sniffer_f",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(0, 0, 0)),
      d_minsamp(1000)
{

    /* allocate circular buffer */
#if GNURADIO_VERSION < 0x031000
    d_writer = gr::make_buffer(buffsize, sizeof(float));
#else
    d_writer = gr::make_buffer(buffsize, sizeof(float), 1, 1);
#endif
    d_reader = gr::buffer_add_reader(d_writer, 0);

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
    const float *in = (const float *)input_items[0];

    (void) output_items;

    std::lock_guard<std::mutex> lock(d_mutex);

    /* dump new samples into the buffer */
    int items_to_copy = std::min(noutput_items, (int)d_writer->bufsize());
    if (items_to_copy < noutput_items)
        in += (noutput_items - items_to_copy);

    if (d_writer->space_available() < items_to_copy)
        d_reader->update_read_pointer(items_to_copy - d_writer->space_available());
    memcpy(d_writer->write_pointer(), in, sizeof(float) * items_to_copy);
    d_writer->update_write_pointer(items_to_copy);

    return noutput_items;
}


/*! \brief Get number of samples available for fetching.
 *  \return The number of samples in the buffer.
 *
 * This method can be used to read how many samples are currently
 * stored in the buffer.
 */
int  sniffer_f::samples_available()
{
    std::lock_guard<std::mutex> lock(d_mutex);

    return d_reader->items_available();
}

/*! \brief Fetch available samples.
 *  \param out Pointer to allocated memory where the samples will be copied.
 *             Should be at least as big as buffer_size().
 *  \param num The number of samples returned.
 */
void sniffer_f::get_samples(float * out, unsigned int &num)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    if ((unsigned int)d_reader->items_available() < d_minsamp) {
        /* not enough samples in buffer */
        num = 0;
        return;
    }

    num = d_reader->items_available();
    memcpy(out, d_reader->read_pointer(), sizeof(float)*num);
    d_reader->update_read_pointer(num);
}


/*! \brief Resize internal buffer.
 *  \param newsize The new size of the buffer (number of samples, not bytes)
 */
void sniffer_f::set_buffer_size(int newsize)
{
    std::lock_guard<std::mutex> lock(d_mutex);

#if GNURADIO_VERSION < 0x031000
    d_writer = gr::make_buffer(newsize, sizeof(float));
#else
    d_writer = gr::make_buffer(newsize, sizeof(float), 1, 1);
#endif
    d_reader = gr::buffer_add_reader(d_writer, 0);
}


/*! \brief Get current size of the internal buffer.
 *
 * This number equals the largest number of samples that can be returned by
 * get_samples().
 */
int  sniffer_f::buffer_size()
{
    std::lock_guard<std::mutex> lock(d_mutex);

    return d_writer->bufsize();
}
