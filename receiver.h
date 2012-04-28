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
#ifndef RECEIVER_H
#define RECEIVER_H

#include <gr_top_block.h>
//#include <gr_audio_sink.h>
#include <gr_complex_to_xxx.h>
#include <gr_multiply_const_ff.h>
#include <gr_simple_squelch_cc.h>
#include <gr_file_sink.h>
#include <gr_file_source.h>
#include <gr_throttle.h>
#include <gr_wavfile_sink.h>
#include <gr_wavfile_source.h>
#include <gr_null_sink.h>
#include "dsp/correct_iq_cc.h"
#include "dsp/rx_source_fcd.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_agc_xx.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/rx_demod_am.h"
#include "dsp/rx_fft.h"
#include "dsp/resampler_ff.h"
#include "dsp/sniffer_f.h"

#include <pulseaudio/pa_sink.h>
#include <pulseaudio/pa_source.h>


/*! \defgroup DSP Digital signal processing library based on GNU Radio */


/*! \brief Top-level receiver class.
 *  \ingroup DSP
 *
 * This class encapsulates the GNU Radio flow graph for the receiver.
 * Front-ends should only control the receiver through the interface provided
 * by this class.
 *
 */
class receiver
{

public:

    /*! \brief Flag used to indicate success or failure of an operation, usually a set_something().     */
    enum status {
        STATUS_OK    = 0, /*!< Operation was successful. */
        STATUS_ERROR = 1  /*!< There was an error. */
    };

    /*! \brief Available demodulators. */
    enum demod {
        DEMOD_NONE = 0,  /*!< No demod. Raw I/Q to audio. */
        DEMOD_AM   = 1,  /*!< Amplitude modulation. */
        DEMOD_FM   = 2,  /*!< Frequency modulation. */
        DEMOD_SSB  = 3,  /*!< Single Side Band. */
        DEMOD_NUM  = 4   /*!< Included for convenience. */
    };

    /*! \brief Filter shape (convenience wrappers for "transition width"). */
    enum filter_shape {
        FILTER_SHAPE_SOFT = 0,   /*!< Soft: Transition band is TBD of width. */
        FILTER_SHAPE_NORMAL = 1, /*!< Normal: Transition band is TBD of width. */
        FILTER_SHAPE_SHARP = 2   /*!< Sharp: Transition band is TBD of width. */
    };


    receiver(const std::string input_device="", const std::string audio_device="");
    ~receiver();

    void start();
    void stop();

    void set_input_device(const std::string device);
    void set_output_device(const std::string device);

    status set_rf_freq(double freq_hz);
    double get_rf_freq();

    status set_rf_gain(float gain_db);


    status set_filter_offset(double offset_hz);
    double get_filter_offset();

    status set_filter(double low, double high, filter_shape shape);
    status set_filter_low(double freq_hz);
    status set_filter_high(double freq_hz);
    status set_filter_shape(filter_shape shape);

    status set_freq_corr(int ppm);
    status set_dc_corr(double dci, double dcq);
    status set_iq_corr(double gain, double phase);


    float get_signal_pwr(bool dbfs);

    void get_fft_data(std::complex<float>* fftPoints, int &fftsize);

    /* Noise blanker */
    status set_nb_on(int nbid, bool on);
    status set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    status set_sql_level(double level_db);
    status set_sql_alpha(double alpha);

    /* AGC */
    status set_agc_on(bool agc_on);
    status set_agc_hang(bool use_hang);
    status set_agc_threshold(int threshold);
    status set_agc_slope(int slope);
    status set_agc_decay(int decay_ms);
    status set_agc_manual_gain(int gain);

    status set_demod(demod rx_demod);

    /* FM parameters */
    status set_fm_maxdev(float maxdev_hz);
    status set_fm_deemph(double tau);

    /* AM parameters */
    status set_am_dcr(bool enabled);

    /* Audio parameters */
    status set_af_gain(float gain_db);
    status start_audio_recording(const std::string filename);
    status stop_audio_recording();
    status start_audio_playback(const std::string filename);
    status stop_audio_playback();

    /* I/Q recording and playback */
    status start_iq_recording(const std::string filename);
    status stop_iq_recording();
    status start_iq_playback(const std::string filename, float samprate);
    status stop_iq_playback();

    /* sample sniffer */
    status start_sniffer(unsigned int samplrate, int buffsize);
    status stop_sniffer();
    void   get_sniffer_data(float * outbuff, int &num);

private:
    bool   d_running;          /*!< Whether receiver is running or not. */
    float  d_bandwidth;        /*!< Receiver bandwidth. */
    int    d_audio_rate;       /*!< Audio output rate. */
    double d_rf_freq;          /*!< Current RF frequency. */
    double d_filter_offset;    /*!< Current filter offset (tune within passband). */
    bool   d_recording_iq;     /*!< Whether we are recording I/Q data. */
    bool   d_recording_wav;    /*!< Whether we are recording WAV file. */
    bool   d_sniffer_active;   /*!< Only one data decoder allowed. */

    demod  d_demod;          /*!< Current demodulator. */

    gr_top_block_sptr         tb;        /*!< The GNU Radio top block. */

    rx_source_base_sptr       src;       /*!< Real time I/Q source. */

    dc_corr_cc_sptr           dc_corr;   /*!< DC corrector block. */

    rx_fft_c_sptr             fft;        /*!< Receiver FFT block. */
    rx_nb_cc_sptr             nb;         /*!< Noise blanker. */
    rx_filter_sptr            filter;     /*!< Bandpass filter. */
    rx_meter_c_sptr           meter;      /*!< Signal strength. */
    rx_agc_cc_sptr            agc;        /*!< Receiver AGC. */
    gr_simple_squelch_cc_sptr sql;        /*!< Squelch. */
    gr_complex_to_real_sptr   demod_ssb;  /*!< SSB demodulator. */
    rx_demod_fm_sptr          demod_fm;   /*!< FM demodulator. */
    rx_demod_am_sptr          demod_am;   /*!< AM demodulator. */
    resampler_ff_sptr         audio_rr;   /*!< Audio resampler. */
    gr_multiply_const_ff_sptr audio_gain; /*!< Audio gain block. */

    gr_file_sink_sptr         iq_sink;    /*!< I/Q file sink. */
    gr_file_source_sptr       iq_src;     /*!< I/Q file source. */
    gr_throttle::sptr         iq_throttle; /*!< Throttle for I/Q playback (in case we don't use audio sink) */
    gr_wavfile_sink_sptr      wav_sink;   /*!< WAV file sink for recording. */
    gr_wavfile_source_sptr    wav_src;    /*!< WAV file source for playback. */
    gr_null_sink_sptr         audio_null_sink; /*!< Audio null sink used during playback. */
    sniffer_f_sptr            sniffer;    /*!< Sample sniffer for data decoders. */
    resampler_ff_sptr         sniffer_rr; /*!< Sniffer resampler. */

    pa_sink_sptr              audio_snk;  /*!< Audio sink. */

protected:


};

#endif // RECEIVER_H
