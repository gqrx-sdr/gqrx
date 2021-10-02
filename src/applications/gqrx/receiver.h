/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#ifndef RECEIVER_H
#define RECEIVER_H

#include <gnuradio/top_block.h>
//#include <gnuradio/blocks/file_sink.h>
#include <osmosdr/source.h>

#include <memory>
#include <string>
#include <vector>

#include <QDebug>

#include "dsp/correct_iq_cc.h"
#include "dsp/filter/fir_decim.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_fft.h"

#include "applications/gqrx/receiver_types.h"
#include "applications/gqrx/demodulator.h"

/**
 * @defgroup DSP Digital signal processing library based on GNU Radio
 */

/**
 * @brief Top-level receiver class.
 * @ingroup DSP
 *
 * This class encapsulates the GNU Radio flow graph for the receiver.
 * Front-ends should only control the receiver through the interface provided
 * by this class.
 */
class receiver
{

public:
    typedef std::shared_ptr<receiver> sptr;

    receiver(const std::string input_device="",
             const std::string audio_device="",
             unsigned int decimation=1);
    ~receiver();

    void            start();
    void            stop();
    void            set_input_device(const std::string device);

    /* Input device control */
    std::vector<std::string> get_antennas(void) const;
    void                     set_antenna(const std::string &antenna);

    double          set_input_rate(double rate);
    double          get_input_rate(void) const { return d_input_rate; }

    unsigned int    set_input_decim(unsigned int decim);
    unsigned int    get_input_decim(void) const { return d_decim; }

    double          get_quad_rate(void) const { return d_input_rate / (double)d_decim; }

    double          set_analog_bandwidth(double bw);
    double          get_analog_bandwidth(void) const;

    void            set_iq_swap(bool reversed);
    bool            get_iq_swap(void) const;

    void            set_dc_cancel(bool enable);
    bool            get_dc_cancel(void) const;

    void            set_iq_balance(bool enable);
    bool            get_iq_balance(void) const;

    rx_status       set_rf_freq(double freq_hz);
    double          get_rf_freq(void);
    rx_status       get_rf_range(double *start, double *stop, double *step);

    std::vector<std::string>  get_gain_names();
    rx_status                 get_gain_range(std::string &name, double *start, double *stop, double *step) const;
    rx_status                 set_auto_gain(bool automatic);
    rx_status                 set_gain(std::string name, double value);
    double                    get_gain(std::string name) const;
    rx_status                 set_freq_corr(double ppm);

    /* Receiver data for visualisation */
    void        set_iq_fft_size(int newsize);
    void        set_iq_fft_window(int window_type);
    void        get_iq_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize);

    /* I/Q recording and playback */
//    rx_status      start_iq_recording(const std::string filename);
//    rx_status      stop_iq_recording();
//    rx_status      seek_iq_file(long pos);

    /* Flowgraph configuration */
    void        begin_reconfigure();
    void        complete_reconfigure();

    /* Demodulators control */
    demodulator::sptr   add_demodulator();
    void                remove_demodulator(size_t idx);

    // Proxy due to flowgraph reconfiguration
    rx_status   set_demod(const size_t idx, rx_demod demod, bool force = false);

    /* Audio system handling */
    void        set_output_device(const std::string device);

private:
    void        connect_all(); /*!< Return the block to use as subreciever source */

private:
    bool          d_running;          /*!< Whether receiver is running or not. */
    double        d_input_rate;       /*!< Input sample rate. */
    double        d_decim_rate;       /*!< Rate after decimation (input_rate / decim) */
    double        d_quad_rate;        /*!< Quadrature rate (after down-conversion) */
    double        d_audio_rate;       /*!< Audio output rate. */
    unsigned int  d_decim;            /*!< input decimation. */
    unsigned int  d_ddc_decim;        /*!< Down-conversion decimation. */
    double        d_rf_freq;          /*!< Current RF frequency. */

    bool          d_iq_rev;           /*!< Whether I/Q is reversed or not. */
    bool          d_dc_cancel;        /*!< Enable automatic DC removal. */
    bool          d_iq_balance;       /*!< Enable automatic IQ balance. */

    std::string   input_devstr;       /*!< Current input device string. */
    std::string   output_devstr;      /*!< Current output device string. */

    gr::top_block_sptr        tb;          /*!< The GNU Radio top block. */

    osmosdr::source::sptr     src;         /*!< Real time I/Q source. */
    fir_decim_cc_sptr         input_decim; /*!< Input decimator. */
    gr::basic_block_sptr      subrxsrc;    /*!< Input to sub receivers */

    dc_corr_cc_sptr           dc_corr;     /*!< DC corrector block. */
    iq_swap_cc_sptr           iq_swap;     /*!< I/Q swapping block. */

    rx_fft_c_sptr             iq_fft;      /*!< Baseband FFT block. */

//    bool               d_recording_iq;     /*!< Whether we are recording I/Q file. */
//    gr::blocks::file_sink::sptr iq_sink;    /*!< I/Q file sink. */

    //! Get a path to a file containing random bytes
    static std::string get_random_file(void);

    /* Demodulating receiving the same RF input */
    std::vector<demodulator::sptr> demods;
};

#endif // RECEIVER_H
