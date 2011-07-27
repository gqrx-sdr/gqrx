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
#include <gr_audio_sink.h>
#include <gr_complex_to_xxx.h>
#include <gr_multiply_const_ff.h>
#include <gr_multiply_const_cc.h>
#include <gr_simple_squelch_cc.h>
#include <gr_agc2_cc.h>
#include <gr_wavfile_sink.h>
#include <fcd/fcd_source_c.h>
#include <dsp/rx_filter.h>
#include <dsp/rx_meter.h>
#include <dsp/rx_demod_fm.h>
#include <dsp/rx_demod_am.h>
#include <dsp/rx_fft.h>
#include <dsp/resampler_ff.h>


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
        STATUS_OK    = 0, /*! Operation was successful. */
        STATUS_ERROR = 1  /*! There was an error. */
    };

    /*! \brief Available demodulators. */
    enum demod {
        DEMOD_SSB  = 0,  /*! Single Side Band. */
        DEMOD_AM   = 1,  /*! Amplitude modulation. */
        DEMOD_FM   = 2,  /*! Frequency modulation. */
        DEMOD_NUM  = 3   /*! Included for convenience. */
    };

    /*! \brief Filter shape (convenience wrappers for "transition width"). */
    enum filter_shape {
        FILTER_SHAPE_SOFT = 0,   /*! Soft: Transition band is TBD of width. */
        FILTER_SHAPE_NORMAL = 1, /*! Normal: Transition band is TBD of width. */
        FILTER_SHAPE_SHARP = 2   /*! Sharp: Transition band is TBD of width. */
    };


    receiver(const std::string input_device="", const std::string audio_device="");
    ~receiver();

    void start();
    void stop();

    void set_input_device(const std::string device);
    void set_output_device(const std::string device);

    status set_rf_freq(float freq_hz);
    float  get_rf_freq();

    status set_rf_gain(float gain_db);


    status set_filter_offset(double offset_hz);
    double get_filter_offset();

    status set_filter(double low, double high, filter_shape shape);
    status set_filter_low(double freq_hz);
    status set_filter_high(double freq_hz);
    status set_filter_shape(filter_shape shape);

    status set_dc_corr(double dci, double dcq);
    status set_iq_corr(double gain, double phase);


    float get_signal_pwr(bool dbfs);

    void get_fft_data(std::complex<float>* fftPoints, int &fftsize);

    /* Squelch parameter */
    status set_sql_level(double level_db);
    status set_sql_alpha(double alpha);

    /* AGC and baseband gain */
    status set_bb_gain(float gain_db);

    status set_demod(demod rx_demod);

    /* FM parameters */
    status set_fm_maxdev(float maxdev_hz);
    status set_fm_deemph(double tau);

    /* AM parameters */
    status set_am_dcr(bool enabled);

    /* Audio parameters */
    status set_af_gain(float gain_db);
    status start_recording(const std::string filename);
    status stop_recording();

private:
    float  d_bandwidth;        /*! Receiver bandwidth. */
    int    d_audio_rate;       /*! Audio output rate. */
    float  d_rf_freq;          /*! Current RF frequency. */
    double d_filter_offset;    /*! Current filter offset (tune within passband). */
    bool   d_recording_wav;    /*! Whether we are recording WAV file. */

    demod  d_demod;          /*! Current demodulator. */

    gr_top_block_sptr         tb;        /*! The GNU Radio top block. */
    fcd_source_c_sptr         fcd_src;   /*! Funcube Dongle source. */
    rx_fft_c_sptr             fft;       /*! Receiver FFT block. */
    rx_filter_sptr            filter;
    rx_meter_c_sptr           meter;      /*! Signal strength. */
    gr_multiply_const_cc_sptr bb_gain;    /*! Baseband gain. Useful for AM-type modulations. */
    gr_agc2_cc_sptr           agc;        /*! AGC. */
    gr_simple_squelch_cc_sptr sql;        /*! Squelch. */
    gr_complex_to_real_sptr   demod_ssb;  /*! SSB demodulator. */
    rx_demod_fm_sptr          demod_fm;   /*! FM demodulator. */
    rx_demod_am_sptr          demod_am;   /*! AM demodulator. */
    resampler_ff_sptr         audio_rr;   /*! Audio resampler. */
    gr_multiply_const_ff_sptr audio_gain; /*! Audio gain block. */
    audio_sink::sptr          audio_snk;  /*! Audio sink. */

    gr_wavfile_sink_sptr      wav_sink;   /*! WAV file sink for recording. */

protected:


};

#endif // RECEIVER_H
