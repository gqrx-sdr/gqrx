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
#include <fcd/fcd_source_c.h>


/*! \defgroup DSP Digital signal processing library based on GNU Radio */


typedef enum {
    RX_STATUS_OK    = 0,
    RX_STATUS_ERROR = 1
} rx_status_t;


typedef enum {
    RX_DEMOD_NONE = 0,
    RX_DEMOD_SSB  = 1,
    RX_DEMOD_AM   = 2,
    RX_DEMOD_AMS  = 3,
    RX_DEMOD_FMN  = 4,
    RX_DEMOD_APT  = 5,
    RX_DEMOD_FMW  = 6,
    RX_DEMOD_B1K  = 7
} rx_demod_t;


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
     */
    rx_status_t set_rf_freq(float freq_hz);

    /*! \brief Set RF gain.
     *  \param gain_db The desired gain in dB.
     *  \return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
     */
    rx_status_t set_rf_gain(float gain_db);


    /*! \brief Set filter offset.
     *  \param offset_hz The desired filter offset in Hz.
     *  \return RX_STATUS_ERROR if the tuning offset is out of range.
     *
     * This method sets a new tuning offset fir the receiver. The tuning offset is used
     * to tune within the passband, i.e. select a specific channel within the received
     * spectrum.
     *
     * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
     * logical limit.
     */
    rx_status_t set_filter_offset(double offset_hz);

    rx_status_t set_filter_low(double freq_hz);
    rx_status_t set_filter_high(double freq_hz);

    rx_status_t set_demod(rx_demod_t demod);

    rx_status_t set_af_gain(float gain_db);


private:
    float d_bandwidth;             /*! Receiver bandwidth. */
    int d_audio_rate;              /*! Audio output rate. */

    gr_top_block_sptr tb;        /*! The GNU Radio top block. */
    fcd_source_c_sptr fcd_src;   /*! Funcube Dongle source. */

    audio_sink::sptr audio_snk; /*! Audio sink. */

protected:


};

#endif // RECEIVER_H
