/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#include <gr_firdes.h>
#include <gr_complex.h>
#include <gri_fft.h>
#include "dsp/rx_fft.h"


rx_fft_c_sptr make_rx_fft_c (int fftsize, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_c (fftsize, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr_firdes::win_type).
 *
 */
rx_fft_c::rx_fft_c(int fftsize, int wintype)
    : gr_sync_block ("rx_fft_c",
          gr_make_io_signature(1, 1, sizeof(gr_complex)),
          gr_make_io_signature(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(-1)
{

    /* create FFT object */
    d_fft = new gri_fft_complex(d_fftsize, true);

    /* allocate circular buffer */
    d_cbuf.set_capacity(d_fftsize);

    /* create FFT window */
    set_window_type(wintype);
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
    int i,j = 0;
    const gr_complex *in = (const gr_complex*)input_items[0];

    /* just throw new samples into the buffer */
    boost::mutex::scoped_lock lock(d_mutex);
    for (i = 0; i < noutput_items; i++) {
        d_cbuf.push_back(in[i]);
    }

    return noutput_items;

}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_c::get_fft_data(std::complex<float>* fftPoints, int &fftSize)
{
    boost::mutex::scoped_lock lock(d_mutex);

    if (d_cbuf.size() < d_fftsize) {
        // not enough samples in the buffer
        fftSize = 0;

        return;
    }

    /* perform FFT */
    do_fft(d_cbuf.linearize(), d_cbuf.size());  // FIXME: array_one() and two() may be faster
    d_cbuf.clear();

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex)*d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has alrady locked it.
 */
void rx_fft_c::do_fft(const gr_complex *data_in, int size)
{
    /* apply window, if any */
    if (d_window.size()) {
        gr_complex *dst = d_fft->get_inbuf();
        int i;
        for (i = 0; i < size; i++)
            dst[i] = data_in[i] * d_window[i];
    }
    else {
        memcpy(d_fft->get_inbuf(), data_in, sizeof(gr_complex)*size);
    }

    /* compute FFT */
    d_fft->execute();
}

/*! \brief Set new FFT size. */
void rx_fft_c::set_fft_size(int fftsize)
{
    if (fftsize != d_fftsize) {
        boost::mutex::scoped_lock lock(d_mutex);

        d_fftsize = fftsize;

        /* clear and resize circular buffer */
        d_cbuf.clear();
        d_cbuf.set_capacity(d_fftsize);

        /* reset window */
        d_wintype = -1;
        set_window_type(d_wintype);

        /* reset FFT object (also reset FFTW plan) */
        delete d_fft;
        d_fft = new gri_fft_complex (d_fftsize, true);
    }

}

/*! \brief Get currently used FFT size. */
int rx_fft_c::get_fft_size()
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_c::set_window_type(int wintype)
{
    if (wintype == d_wintype) {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr_firdes::WIN_HAMMING) || (d_wintype > gr_firdes::WIN_BLACKMAN_hARRIS)) {
        d_wintype = gr_firdes::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr_firdes::window((gr_firdes::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_c::get_window_type()
{
    return d_wintype;
}


/**   rx_fft_f     **/

rx_fft_f_sptr make_rx_fft_f(int fftsize, int wintype)
{
    return gnuradio::get_initial_sptr(new rx_fft_f (fftsize, wintype));
}

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr_firdes::win_type).
 *
 */
rx_fft_f::rx_fft_f(int fftsize, int wintype)
    : gr_sync_block ("rx_fft_f",
          gr_make_io_signature(1, 1, sizeof(float)),
          gr_make_io_signature(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(-1)
{

    /* create FFT object */
    d_fft = new gri_fft_complex(d_fftsize, true);

    /* allocate circular buffer */
    d_cbuf.set_capacity(d_fftsize);

    /* create FFT window */
    set_window_type(wintype);
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
    int i,j = 0;
    const float *in = (const float*)input_items[0];

    /* just throw new samples into the buffer */
    boost::mutex::scoped_lock lock(d_mutex);
    for (i = 0; i < noutput_items; i++) {
        d_cbuf.push_back(in[i]);
    }

    return noutput_items;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
void rx_fft_f::get_fft_data(std::complex<float>* fftPoints, int &fftSize)
{
    boost::mutex::scoped_lock lock(d_mutex);

    if (d_cbuf.size() < d_fftsize) {
        // not enough samples in the buffer
        fftSize = 0;

        return;
    }

    /* perform FFT */
    do_fft(d_cbuf.linearize(), d_cbuf.size());  // FIXME: array_one() and two() may be faster
    d_cbuf.clear();

    /* get FFT data */
    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex)*d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the available input data.
 *  \param data_in The data to compute FFT on.
 *  \param size The size of data_in.
 *
 * Note that this function does not lock the mutex since the caller, get_fft_data()
 * has alrady locked it.
 */
void rx_fft_f::do_fft(const float *data_in, int size)
{
    gr_complex *dst = d_fft->get_inbuf();
    int i;

    /* apply window, and convert to complex */
    if (d_window.size()) {
        for (i = 0; i < size; i++)
            dst[i] = data_in[i] * d_window[i];
    }
    else {
        for (i = 0; i < size; i++)
            dst[i] = data_in[i];
    }

    /* compute FFT */
    d_fft->execute();
}


/*! \brief Set new FFT size. */
void rx_fft_f::set_fft_size(int fftsize)
{
    if (fftsize != d_fftsize) {
        boost::mutex::scoped_lock lock(d_mutex);

        d_fftsize = fftsize;

        /* clear and resize circular buffer */
        d_cbuf.clear();
        d_cbuf.set_capacity(d_fftsize);

        /* reset window */
        d_wintype = -1;
        set_window_type(d_wintype);

        /* reset FFT object (also reset FFTW plan) */
        delete d_fft;
        d_fft = new gri_fft_complex(d_fftsize, true);
    }
}

/*! \brief Get currently used FFT size. */
int rx_fft_f::get_fft_size()
{
    return d_fftsize;
}

/*! \brief Set new window type. */
void rx_fft_f::set_window_type(int wintype)
{
    if (wintype == d_wintype) {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr_firdes::WIN_HAMMING) || (d_wintype > gr_firdes::WIN_BLACKMAN_hARRIS)) {
        d_wintype = gr_firdes::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr_firdes::window((gr_firdes::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
int rx_fft_f::get_window_type()
{
    return d_wintype;
}
