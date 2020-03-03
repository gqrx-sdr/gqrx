/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#ifndef RX_FFT_H
#define RX_FFT_H

#include <gnuradio/sync_block.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/filter/firdes.h>       /* contains enum win_type */
#include <gnuradio/gr_complex.h>
#include <gnuradio/io_signature.h>
#include <boost/thread/mutex.hpp>
#include <boost/circular_buffer.hpp>
#include <algorithm>
#include <chrono>
#include <math.h>

#define MAX_FFT_SIZE 1048576

/*! \brief Block for computing complex FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the received spectrum.
 *
 * The samples are collected in a cicular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data via get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * of course that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_c
 */
template<class streamsizeof>
class rx_fft : public gr::sync_block
{
//    friend rx_fft_c_sptr make_rx_fft_c(const std::string &name, unsigned int fftsize, double quad_rate, int wintype);

//protected:
public:
    rx_fft(const std::string &name, unsigned int fftsize=4096, double rate=0, int wintype=gr::filter::firdes::WIN_HAMMING);

public:
    ~rx_fft();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize);

    void set_window_type(int wintype);
    int  get_window_type() const;

    void set_fft_size(unsigned int fftsize);
    void set_rate(double quad_rate);
    unsigned int get_fft_size() const;

private:
    unsigned int d_fftsize;   /*! Current FFT size. */
    int          d_wintype;   /*! Current window type. */
    double       d_rate;

    boost::mutex d_mutex;  /*! Used to lock FFT output buffer. */

    gr::fft::fft_complex    *d_fft;    /*! FFT object. */
    std::vector<float>  d_window; /*! FFT window taps. */

    boost::circular_buffer<gr_complex> d_cbuf; /*! buffer to accumulate samples. */
    std::chrono::time_point<std::chrono::steady_clock> d_lasttime;

    void do_fft(unsigned int size);
    void set_params();

};

typedef boost::shared_ptr<rx_fft<gr_complex>> rx_fft_c_sptr;


/*! \brief Return a shared_ptr to a new instance of rx_fft_c.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_c constructor is private.
 * make_rx_fft_c is the public interface for creating new instances.
 */
rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize=4096, double quad_rate=0, int wintype=gr::filter::firdes::WIN_HAMMING);
typedef boost::shared_ptr<rx_fft<float>> rx_fft_f_sptr;
/*! \brief Return a shared_ptr to a new instance of rx_fft_f.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_f constructor is private.
 * make_rx_fft_f is the public interface for creating new instances.
 */
rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize=1024, double audio_rate=48000, int wintype=gr::filter::firdes::WIN_HAMMING);

/*! \brief Create receiver FFT object.
 *  \param fftsize The FFT size.
 *  \param wintype The window type (see gr::filter::firdes::win_type).
 *
 */
template <typename streamsizeof>
rx_fft<streamsizeof>::rx_fft(const std::string &name, unsigned int fftsize, double rate, int wintype)
    : gr::sync_block (name,
          gr::io_signature::make(1, 1, sizeof(streamsizeof)),
          gr::io_signature::make(0, 0, 0)),
      d_fftsize(fftsize),
      d_wintype(-1),
      d_rate(rate)
{

    /* create FFT object */
    d_fft = new gr::fft::fft_complex(d_fftsize, true);

    /* allocate circular buffer */
    d_cbuf.set_capacity(d_fftsize + d_rate);

    /* create FFT window */
    set_window_type(wintype);

    d_lasttime = std::chrono::steady_clock::now();
}

template <typename streamsizeof>
rx_fft<streamsizeof>::~rx_fft()
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
template <typename streamsizeof>
int rx_fft<streamsizeof>::work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items)
{
    int i;
    const gr_complex *in = (const gr_complex*)input_items[0];
    (void) output_items;

    /* just throw new samples into the buffer */
    boost::mutex::scoped_lock lock(d_mutex);
    d_cbuf.resize( d_cbuf.size() + input_items.size() );
    for (i = 0; i < noutput_items; i++)
    {
        d_cbuf.push_back(in[i]);
    }

    return noutput_items;
}

/*! \brief Get FFT data.
 *  \param fftPoints Buffer to copy FFT data
 *  \param fftSize Current FFT size (output).
 */
template <typename streamsizeof>
void rx_fft<streamsizeof>::get_fft_data(std::complex<float>* fftPoints, unsigned int &fftSize)
{
    boost::mutex::scoped_lock lock(d_mutex);

    if (d_cbuf.size() < d_fftsize)
    {
        // not enough samples in the buffer
        fftSize = 0;

        return;
    }

    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - d_lasttime;
    d_lasttime = now;

    /* perform FFT */
    d_cbuf.erase_begin(std::min((unsigned int)(diff.count() * d_rate * 1.001), (unsigned int)d_cbuf.size() - d_fftsize));
    do_fft(d_fftsize);
    //d_cbuf.clear();

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
template <typename streamsizeof>
void
rx_fft<streamsizeof>::do_fft(unsigned int size)
{
    /* apply window, if any */
    if (d_window.size())
    {
        gr_complex *dst = d_fft->get_inbuf();
        for (unsigned int i = 0; i < size; i++)
            dst[i] = d_cbuf[i] * d_window[i];
    }
    else
    {
        memcpy(d_fft->get_inbuf(), d_cbuf.linearize(), sizeof(gr_complex)*size);
    }

    /* compute FFT */
    d_fft->execute();
}

/*! \brief Update circular buffer and FFT object. */
template <typename streamsizeof>
void
rx_fft<streamsizeof>::set_params()
{
    boost::mutex::scoped_lock lock(d_mutex);

    /* clear and resize circular buffer */
    d_cbuf.clear();
    d_cbuf.set_capacity(d_fftsize + d_rate);

    /* reset window */
    int wintype = d_wintype; // FIXME: would be nicer with a window_reset()
    d_wintype = -1;
    set_window_type(wintype);

    /* reset FFT object (also reset FFTW plan) */
    delete d_fft;
    d_fft = new gr::fft::fft_complex (d_fftsize, true);
}

/*! \brief Set new FFT size. */
template <typename streamsizeof>
void rx_fft<streamsizeof>::set_fft_size(unsigned int fftsize)
{
    if (fftsize != d_fftsize)
    {
        d_fftsize = fftsize;
        set_params();
    }
}

/*! \brief Set new quadrature rate. */
template <typename streamsizeof>
void rx_fft<streamsizeof>::set_rate(double rate)
{
    if (rate != d_rate) {
        d_rate = rate;
        set_params();
    }
}

/*! \brief Get currently used FFT size. */
template <typename streamsizeof>
unsigned int rx_fft<streamsizeof>::get_fft_size() const
{
    return d_fftsize;
}

/*! \brief Set new window type. */
template <typename streamsizeof>
void rx_fft<streamsizeof>::set_window_type(int wintype)
{
    if (wintype == d_wintype)
    {
        /* nothing to do */
        return;
    }

    d_wintype = wintype;

    if ((d_wintype < gr::filter::firdes::WIN_HAMMING) || (d_wintype > gr::filter::firdes::WIN_FLATTOP))
    {
        d_wintype = gr::filter::firdes::WIN_HAMMING;
    }

    d_window.clear();
    d_window = gr::filter::firdes::window((gr::filter::firdes::win_type)d_wintype, d_fftsize, 6.76);
}

/*! \brief Get currently used window type. */
template <typename streamsizeof>
int rx_fft<streamsizeof>::get_window_type() const
{
    return d_wintype;
}

#endif /* RX_FFT_H */
