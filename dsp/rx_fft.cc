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
#include <gr_firdes.h>
#include <gr_complex.h>
#include <gri_fft.h>
#include <dsp/rx_fft.h>


rx_fft_c_sptr make_rx_fft_c (int fftsize, int wintype, bool use_avg)
{
    return gnuradio::get_initial_sptr(new rx_fft_c (fftsize, wintype, use_avg));
}


/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr_firdes::win_type).
 *  \param use_avg Whether to use averaging.
 *
 * \bug Implement use_avg.
 */
rx_fft_c::rx_fft_c(int fftsize, int wintype, bool use_avg)
    : gr_sync_block ("rx_fft_c",
          gr_make_io_signature(1, 1, sizeof(gr_complex)),
          gr_make_io_signature(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(-1),
      d_use_avg(use_avg)
{

    /* create FFT object */
    d_fft = new gri_fft_complex (d_fftsize, true);

    /* create FFT buffer */
    d_rbuf_idx = 0;
    d_rbuf = new gr_complex[d_fftsize];

    /* create window */
    set_window_type(wintype);
}

rx_fft_c::~rx_fft_c()
{
    delete [] d_rbuf;
    delete d_fft;
}


/*! \brief Execute receiver FFT.
 *  \param mooutput_items
 *  \param input_items
 *  \param output_items
 *
 * Collect samples into the FFT buffer d_rbuf and execute FFT when we have
 * enough samples (d_fftsize). It may take several calls to work() before we have enough
 * samples depending on the sample rate.
 */
int rx_fft_c::work (int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    int i,j = 0;
    const gr_complex *in = (const gr_complex*)input_items[0];


    for (i = 0; i < noutput_items; i += d_fftsize) {

        unsigned int datasize = noutput_items - i;
        unsigned int resid = d_fftsize - d_rbuf_idx;

        if (datasize >= resid)
        /* we have enough samples to do an FFT */
        {
            /* Fill up residual buffer with d_fftsize number of items */
            memcpy(d_rbuf+d_rbuf_idx, &in[j], sizeof(gr_complex)*resid);
            d_rbuf_idx = 0;

            j += resid;
            do_fft(d_rbuf, d_fftsize);

        }
        else
        /* copy what we received into the residual buffer for next execution */
        {
            memcpy(d_rbuf+d_rbuf_idx, &in[j], sizeof(gr_complex)*datasize);
            d_rbuf_idx += datasize;
            j += datasize;
        }
    }

    consume_each(j);
    return j;
}



/*! \brief Get FFT data.
 *  \param fftPoint Buffer to copy FFT data
 *  \param fftSize Current FFt size (output).
 */
void rx_fft_c::get_fft_data(std::complex<float>* fftPoints, int &fftSize)
{
    boost::mutex::scoped_lock lock(d_mutex);

    memcpy(fftPoints, d_fft->get_outbuf(), sizeof(gr_complex)*d_fftsize);
    fftSize = d_fftsize;
}

/*! \brief Compute FFT on the avaialble input data. */
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
    boost::mutex::scoped_lock lock(d_mutex);
    d_fft->execute();
}



/*! \brief Set new FFT size. */
void rx_fft_c::set_fft_size(int fftsize)
{
    if (fftsize != d_fftsize) {

        d_fftsize = fftsize;
        d_rbuf_idx = 0;

        /* clear & resize residual buffer */
        delete [] d_rbuf;
        d_rbuf = new gr_complex[d_fftsize];

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
