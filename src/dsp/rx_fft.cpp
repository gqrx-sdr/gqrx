/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include <gnuradio/filter/firdes.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/fft/fft.h>
#include "dsp/rx_fft.h"
#include <algorithm>


rx_fft_c_sptr make_rx_fft_c (unsigned int fftsize, double quad_rate, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_c (fftsize, quad_rate, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::fft::window::win_type).
 *
 */
rx_fft_c::rx_fft_c(unsigned int fftsize, double quad_rate, int wintype)
    : gr::sync_block ("rx_fft_c",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_quadrate(quad_rate),
      d_wintype(-1)
{

    /* create FFT object */
#if GNURADIO_VERSION < 0x030900
    d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif

    /* allocate circular buffer */
    d_writer = gr::make_buffer(d_fftsize + d_quadrate, sizeof(gr_complex));
    d_reader = gr::buffer_add_reader(d_writer, 0);

    /* create FFT window */
    set_window_type(wintype);

    d_lasttime = std::chrono::steady_clock::now();
}

rx_fft_c::~rx_fft_c()
{
    delete d_fft;
}

/*! \brief Receiver FFT work method.
 *  \param noutput_items
 *  \param input_items
 *  \param output_items
 *
 * This method does nothing except throwing the incoming samples into the
 * circular buffer.
 * FFT is only executed when the GUI asks for new FFT data via get_fft_data().
 */
int rx_fft_c::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    (void) output_items;

    /* just throw new samples into the buffer */
    std::lock_guard<std::mutex> lock(d_mutex);
    if (d_writer->space_available() < noutput_items)
        d_reader->update_read_pointer(noutput_items - d_writer->space_available());
    memcpy(d_writer->write_pointer(), input_items[0], sizeof(gr_complex) * noutput_items);
    d_writer->update_write_pointer(noutput_items);
    return noutput_items;

}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_c::get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    if (d_reader->items_available() < d_fftsize)
    {
        // not enough samples in the buffer
        fftSize = 0;

        return;
    }

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    d_lasttime = now;

    /* perform FFT */
    d_reader->update_read_pointer(std::min((int)(diff.count() * d_quadrate * 1.001), d_reader->items_available() - d_fftsize));
    do_fft(d_fftsize);

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex) * d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has already locked it.
 */
void rx_fft_c::do_fft(unsigned int size)
{
    /* apply window, if any */
    gr_complex * p = (gr_complex *)d_reader->read_pointer();
    if (d_window.size())
    {
        gr_complex *dst = d_fft->get_inbuf();
        for (unsigned int i = 0; i < size; i++)
            dst[i] = p[i] * d_window[i];
    }
    else
    {
        memcpy(d_fft->get_inbuf(), p, sizeof(gr_complex) * size);
    }

    /* compute FFT */
    d_fft->execute();
}

/*! \brief Update circular buffer and FFT object. */
void rx_fft_c::set_params()
{
    std::lock_guard<std::mutex> lock(d_mutex);
    /* clear and resize circular buffer */
    d_writer = gr::make_buffer(d_fftsize + d_quadrate, sizeof(gr_complex));
    d_reader = gr::buffer_add_reader(d_writer, 0);

    /* reset window */
    int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
    d_wintype = -1;
    set_window_type(wintype);

    /* reset FFT object (also reset FFTW plan) */
    delete d_fft;
#if GNURADIO_VERSION < 0x030900
    d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif
}

/*! \brief Set new FFT size. */
void rx_fft_c::set_fft_size(int fftsize)
{
    if (fftsize != d_fftsize)
    {
        d_fftsize = fftsize;
        set_params();
    }

}

/*! \brief Set new quadrature rate. */
void rx_fft_c::set_quad_rate(double quad_rate)
{
    if (quad_rate != d_quadrate) {
        d_quadrate = quad_rate;
        set_params();
    }
}

/*! \brief Get currently used FFT size. */
unsigned int rx_fft_c::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_c::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::fft::window::WIN_HAMMING) || (d_wintype > gr::fft::window::WIN_FLATTOP))
    {
        d_wintype = gr::fft::window::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::fft::window::build((gr::fft::window::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_c::get_window_type() const
{
    return d_wintype;
}


/**   rx_fft_f     **/

rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, double audio_rate, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_f (fftsize, audio_rate, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::fft::window::win_type).
 *
 */
rx_fft_f::rx_fft_f(unsigned int fftsize, double audio_rate, int wintype)
    : gr::sync_block ("rx_fft_f",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_audiorate(audio_rate),
      d_wintype(-1)
{

    /* create FFT object */
#if GNURADIO_VERSION < 0x030900
    d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
    d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif

    /* allocate circular buffer */
    d_writer = gr::make_buffer(d_fftsize + d_audiorate, sizeof(float));
    d_reader = gr::buffer_add_reader(d_writer, 0);

    /* create FFT window */
    set_window_type(wintype);

    d_lasttime = std::chrono::steady_clock::now();
}

rx_fft_f::~rx_fft_f()
{
    delete d_fft;
}

/*! \brief Audio FFT work method.
 *  \param noutput_items
 *  \param input_items
 *  \param output_items
 *
 * This method does nothing except throwing the incoming samples into the
 * circular buffer.
 * FFT is only executed when the GUI asks for new FFT data via get_fft_data().
 */
int rx_fft_f::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    (void) output_items;

    /* just throw new samples into the buffer */
    std::lock_guard<std::mutex> lock(d_mutex);
    if(d_writer->space_available() < noutput_items)
        d_reader->update_read_pointer(noutput_items - d_writer->space_available());
    memcpy(d_writer->write_pointer(), input_items[0], sizeof(gr_complex) * noutput_items);
    d_writer->update_write_pointer(noutput_items);
    return noutput_items;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_f::get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    if (d_reader->items_available() < d_fftsize)
    {
        // not enough samples in the buffer
        fftSize = 0;

        return;
    }

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    d_lasttime = now;

    /* perform FFT */
    d_reader->update_read_pointer(std::min((unsigned int)(diff.count() * d_audiorate * 1.001), (unsigned int)d_reader->items_available() - d_fftsize));
    do_fft(d_fftsize);

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex) * d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has already locked it.
 */
void rx_fft_f::do_fft(unsigned int size)
{
    gr_complex *dst = d_fft->get_inbuf();
    float * p = (float *)d_reader->read_pointer();
    /* apply window, and convert to complex */
    if (d_window.size())
    {
        for (unsigned int i = 0; i < size; i++)
            dst[i] = p[i] * d_window[i];
    }
    else
    {
        for (unsigned int i = 0; i < size; i++)
            dst[i] = p[i];
    }

    /* compute FFT */
    d_fft->execute();
}


/*! \brief Set new FFT size. */
void rx_fft_f::set_fft_size(int fftsize)
{
    if (fftsize != d_fftsize)
    {
        std::lock_guard<std::mutex> lock(d_mutex);

        d_fftsize = fftsize;

        /* clear and resize circular buffer */
        d_writer = gr::make_buffer(d_fftsize + d_audiorate,sizeof(float));
        d_reader = gr::buffer_add_reader(d_writer, 0);

        /* reset window */
        int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
        d_wintype = -1;
        set_window_type(wintype);

        /* reset FFT object (also reset FFTW plan) */
        delete d_fft;
#if GNURADIO_VERSION < 0x030900
        d_fft = new gr::fft::fft_complex(d_fftsize, true);
#else
        d_fft = new gr::fft::fft_complex_fwd(d_fftsize);
#endif
    }
}

/*! \brief Get currently used FFT size. */
unsigned int rx_fft_f::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_f::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::fft::window::WIN_HAMMING) || (d_wintype > gr::fft::window::WIN_FLATTOP))
    {
        d_wintype = gr::fft::window::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::fft::window::build((gr::fft::window::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_f::get_window_type() const
{
    return d_wintype;
}
