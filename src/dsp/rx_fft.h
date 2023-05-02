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
#ifndef RX_FFT_H
#define RX_FFT_H

#include <mutex>
#include <gnuradio/sync_block.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/filter/firdes.h>       /* contains enum win_type */
#include <gnuradio/gr_complex.h>
#include <gnuradio/buffer.h>
#if GNURADIO_VERSION >= 0x031000
#include <gnuradio/buffer_reader.h>
#endif
#include <chrono>


#define MAX_FFT_SIZE (1024 * 1024 * 4)
#define AUDIO_BUFFER_SIZE 65536

class rx_fft_c;
class rx_fft_f;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_fft_c> rx_fft_c_sptr;
typedef boost::shared_ptr<rx_fft_f> rx_fft_f_sptr;
#else
typedef std::shared_ptr<rx_fft_c> rx_fft_c_sptr;
typedef std::shared_ptr<rx_fft_f> rx_fft_f_sptr;
#endif


/*! \brief Return a shared_ptr to a new instance of rx_fft_c.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_c constructor is private.
 * make_rx_fft_c is the public interface for creating new instances.
 */
rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize=4096,
                            double quad_rate=0,
                            int wintype=gr::fft::window::WIN_HAMMING,
                            bool normalize_energy = false);


/*! \brief Block for computing complex FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the received spectrum.
 *
 * The samples are collected in a circular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data via get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * of course that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_c
 */
class rx_fft_c : public gr::sync_block
{
    friend rx_fft_c_sptr make_rx_fft_c(unsigned int fftsize, double quad_rate,
                                       int wintype, bool normalize_energy);

protected:
    rx_fft_c(unsigned int fftsize=4096,
             double quad_rate=0,
             int wintype=gr::fft::window::WIN_HAMMING,
             bool normalize_energy = false);

public:
    ~rx_fft_c();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void get_fft_data(float* fftPoints);
    void get_sample_data(std::complex<float>* data);

    void set_window_type(int wintype, bool normalize_energy);
    int  get_window_type() const { return d_wintype; }

    void set_fft_size(unsigned int fftsize);
    void set_quad_rate(double quad_rate);
    unsigned int fft_size() const {return d_fftsize;}

private:
    unsigned int d_fftsize;   /*! Current FFT size. */
    double       d_quadrate;
    int          d_wintype;   /*! Current window type. */
    bool         d_normalize_energy;

    std::mutex   d_in_mutex;   /*! Used to lock input buffer. */

#if GNURADIO_VERSION < 0x030900
    gr::fft::fft_complex    *d_fft;    /*! FFT object. */
#else
    gr::fft::fft_complex_fwd *d_fft;   /*! FFT object. */
#endif
    std::vector<float>  d_window; /*! FFT window taps. */

    gr::buffer_sptr d_writer;
    gr::buffer_reader_sptr d_reader;
    std::chrono::time_point<std::chrono::steady_clock> d_lasttime;

    void apply_window(unsigned int size);
    void update_window();
};


/*! \brief Return a shared_ptr to a new instance of rx_fft_f.
 *  \param fftsize The FFT size
 *  \param winttype The window type (see gnuradio/filter/firdes.h)
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, the rx_fft_f constructor is private.
 * make_rx_fft_f is the public interface for creating new instances.
 */
rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize=1024,
                            double audio_rate=48000,
                            int wintype=gr::fft::window::WIN_HAMMING,
                            bool normalize_energy = false);


/*! \brief Block for computing real FFT.
 *  \ingroup DSP
 *
 * This block is used to compute the FFT of the audio spectrum or anything
 * else where real FFT is useful.
 *
 * The samples are collected in a circular buffer with size FFT_SIZE.
 * When the GUI asks for a new set of FFT data using get_fft_data() an FFT
 * will be performed on the data stored in the circular buffer - assuming
 * that the buffer contains at least fftsize samples.
 *
 * \note Uses code from qtgui_sink_f
 */
class rx_fft_f : public gr::sync_block
{
    friend rx_fft_f_sptr make_rx_fft_f(unsigned int fftsize, double audio_rate,
                                       int wintype, bool normalize_energy);

protected:
    rx_fft_f(unsigned int fftsize=1024,
             double audio_rate=48000,
             int wintype=gr::fft::window::WIN_HAMMING,
             bool normalize_energy = false);

public:
    ~rx_fft_f();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items);

    void get_fft_data(float* fftPoints);

    void set_window_type(int wintype, bool normalize_energy);
    int  get_window_type() const { return d_wintype; }

    void set_fft_size(unsigned int fftsize);
    unsigned int fft_size() const {return d_fftsize;}

private:
    unsigned int d_fftsize;   /*! Current FFT size. */
    double       d_audiorate;
    int          d_wintype;   /*! Current window type. */
    bool         d_normalize_energy;

    std::mutex   d_in_mutex;   /*! Used to lock input buffer. */

#if GNURADIO_VERSION < 0x030900
    gr::fft::fft_complex    *d_fft;    /*! FFT object. */
#else
    gr::fft::fft_complex_fwd *d_fft;   /*! FFT object. */
#endif
    std::vector<float>  d_window; /*! FFT window taps. */

    gr::buffer_sptr d_writer;
    gr::buffer_reader_sptr d_reader;
    std::chrono::time_point<std::chrono::steady_clock> d_lasttime;

    void apply_window(unsigned int size);
    void update_window();
};


#endif /* RX_FFT_H */
