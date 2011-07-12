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
#include <gr_agc2_cc.h>
#include <fcd/fcd_source_c.h>
#include <dsp/rx_filter.h>
#include <dsp/rx_meter.h>
#include <dsp/rx_demod_fm.h>
#include <dsp/rx_fft.h>


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


    /*! \brief Public contructor.
     *  \param input_device Input device specifier, e.g. hw:1 for FCD source.
     *  \param audio_device Audio output device specifier,
     *                      e.g. hw:0 when using ALSA or Portaudio.
     *
     * \todo Option to use UHD device instead of FCD.
     */
    receiver(const std::string input_device="", const std::string audio_device="");

    /*! \brief Public destructor. */
    ~receiver();

    /*! \brief Start the receiver. */
    void start();

    /*! \brief Stop the receiver. */
    void stop();

    /*! \brief Set RF frequency.
     *  \param freq_hz The desired frequency in Hz.
     *  \return RX_STATUS_ERROR if an error occurs, e.g. the frequency is out of range.
     *  \sa get_rf_freq()
     */
    status set_rf_freq(float freq_hz);

    /*! \brief Get RF frequency.
     *  \return The current RF frequency.
     *  \sa set_rf_freq()
     */
    float  get_rf_freq();

    /*! \brief Set RF gain.
     *  \param gain_db The desired gain in dB.
     *  \return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
     */
    status set_rf_gain(float gain_db);


    /*! \brief Set filter offset.
     *  \param offset_hz The desired filter offset in Hz.
     *  \return RX_STATUS_ERROR if the tuning offset is out of range.
     *
     * This method sets a new tuning offset for the receiver. The tuning offset is used
     * to tune within the passband, i.e. select a specific channel within the received
     * spectrum.
     *
     * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
     * logical limit.
     *
     * \sa get_filter_offset()
     */
    status set_filter_offset(double offset_hz);

    /*! \brief Get filterm offset.
     *  \return The current filter offset.
     *  \sa set_filter_offset()
     */
    double get_filter_offset();

    status set_filter(double low, double high, filter_shape shape);
    status set_filter_low(double freq_hz);
    status set_filter_high(double freq_hz);
    status set_filter_shape(filter_shape shape);

    status set_dc_corr(double dci, double dcq);
    status set_iq_corr(double gain, double phase);


    /*! \brief Get current signal power.
     *  \param dbfs Whether to use dbfs or absolute power.
     *  \return The current signal power.
     *
     * This method returns the current signal power detected by the receiver. The detector
     * is located after the band pass filter. The full scale is 1.0
     */
    float get_signal_pwr(bool dbfs);

    void get_fft_data(std::complex<float>* fftPoints, int &fftsize);

    status set_demod(demod rx_demod);

    /* FM parameeters */
    status set_fm_maxdev(float maxdev_hz);
    status set_fm_deemph(double tau);

    status set_af_gain(float gain_db);


private:
    float  d_bandwidth;        /*! Receiver bandwidth. */
    int    d_audio_rate;       /*! Audio output rate. */
    float  d_rf_freq;          /*! Current RF frequency. */
    double d_filter_offset;    /*! Current filter offset (tune within passband). */

    demod  d_demod;          /*! Current demodulator. */

    gr_top_block_sptr         tb;        /*! The GNU Radio top block. */
    fcd_source_c_sptr         fcd_src;   /*! Funcube Dongle source. */
    rx_fft_c_sptr             fft;       /*! Receiver FFT block. */
    rx_filter_sptr            filter;
    rx_meter_c_sptr           meter;      /*! Signal strength. */
    gr_complex_to_real_sptr   demod_ssb;  /*! SSB demodulator. */
    gr_agc2_cc_sptr           agc;        /*! AGC. */
    rx_demod_fm_sptr          demod_fm;   /*! FM demodulator. */
    gr_multiply_const_ff_sptr audio_gain; /*! Audio gain block. */
    audio_sink::sptr          audio_snk;  /*! Audio sink. */

protected:


};

#endif // RECEIVER_H
