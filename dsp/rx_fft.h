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
#ifndef RX_FFT_H
#define RX_FFT_H

#include <gr_sync_block.h>
#include <gri_fft.h>
#include <gr_firdes.h>       /* contains enum win_type */
#include <gr_complex.h>


class rx_fft_c;

typedef boost::shared_ptr<rx_fft_c> rx_fft_c_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_fft_c.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gr_firdes.h)
 *  \param use_avg Whether to use averaging.
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_c constructor is private.
 * make_rx_fft_c is the public interface for creating new instances.
 */
rx_fft_c_sptr make_rx_fft_c(int fftsize, int wintype, bool use_avg);


/*! \brief Block for computing complex FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the received spectrum.
 */
class rx_fft_c : public gr_sync_block
{
    friend rx_fft_c_sptr make_rx_fft_c(int fftsize, int wintype, bool use_avg);

protected:
    rx_fft_c(int fftsize=2048, int wintype=gr_firdes::WIN_HAMMING, bool use_avg=false);

public:
    ~rx_fft_c();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);


    gr_complex *get_fft_data();

    void set_window_type(int wintype);
    int  get_window_type();

    void set_fft_size(int fftsize);
    int  get_fft_size();

private:
    int  d_fftsize;   /*! Current FFT size. */
    int  d_wintype;   /*! Current window type. */
    bool d_use_avg;   /*! Whether to store the average or jsut the latest value. */

    gri_fft_complex    *d_fft;    /*! FFT object. */
    std::vector<float>  d_window; /*! FFT window taps. */

    /* buffer to collect samples for FFT */
    int         d_rbuf_idx;
    gr_complex *d_rbuf;

    void do_fft(const gr_complex *data_in, int size);

};


#endif /* RX_FFT_H */
