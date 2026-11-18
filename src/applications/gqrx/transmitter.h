/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx contributors.
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
#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/pfb_arb_resampler_ccf.h>
#include <gnuradio/top_block.h>
#include <osmosdr/sink.h>
#include <string>

#include "dsp/tx_mod_am.h"
#include "dsp/tx_mod_nfm.h"
#include "dsp/tx_mod_ssb.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_source.h"
#else
#include <gnuradio/audio/source.h>
#endif

/**
 * @defgroup TX Transmitting
 */

/**
 * @brief Top-level transmitter class.
 * @ingroup TX
 *
 * This class encapsulates the GNU Radio flow graph for the transmitter:
 *
 *   audio -> mic gain -> modulator -> resampler -> RF power -> SDR sink
 *
 * The flowgraph is started and stopped on each PTT event; on half-duplex
 * devices the application must stop the receiver before starting the
 * transmitter.
 */
class transmitter
{

public:

    /** Flag used to indicate success or failure of an operation */
    enum status {
        STATUS_OK    = 0, /*!< Operation was successful. */
        STATUS_ERROR = 1  /*!< There was an error. */
    };

    /** Available modulators. */
    enum tx_mod {
        TX_MOD_OFF = 0,  /*!< No modulator. */
        TX_MOD_NFM = 1,  /*!< Narrow band FM. */
        TX_MOD_AM  = 2,  /*!< Amplitude modulation (DSB). */
        TX_MOD_USB = 3,  /*!< Single sideband, upper sideband. */
        TX_MOD_LSB = 4   /*!< Single sideband, lower sideband. */
    };

    static const unsigned int TX_AUDIO_RATE = 48000;

    transmitter(const std::string input_device="",
                const std::string audio_device="");
    ~transmitter();

    void        start();
    void        stop();

    void        set_tx_device(const std::string device);
    void        set_audio_device(const std::string device);

    bool        is_running() const { return d_running; }

    status      set_mod(tx_mod mod);
    tx_mod      get_mod(void) const { return d_mod; }

    double      set_rf_freq(double freq_hz);
    double      get_rf_freq(void);
    status      get_rf_range(double *start, double *stop, double *step);

    status      set_rf_power(double value);
    double      get_rf_power(void) const { return d_rf_power; }
    status      get_gain_range(double *start, double *stop, double *step);

    status      set_mic_gain(float gain_db);
    float       get_mic_gain(void) const { return d_mic_gain; }
    void        set_muted(bool mute);

    /* FM parameters */
    status      set_max_dev(float max_dev);
    status      set_ctcss(float freq);

    /* AM parameters */
    status      set_mod_index(float mod_index);

    /**
     * @brief Check whether the given input device string refers to a
     *        device with transmit capability.
     *
     * Drivers known to have no TX capability are rejected immediately;
     * for soapy= devices the capability is decided from the soapy
     * driver name, since probing with a temporary osmosdr::sink is
     * unreliable (see sink_impl.cc device string fallback).
     */
    static bool device_supports_tx(const std::string &device);

private:
    void        connect_mod(tx_mod mod);

private:
    bool        d_running;      /*!< Whether transmitter is running or not. */
    double      d_rf_freq;      /*!< Current RF frequency. */
    double      d_rf_power;     /*!< RF power, relative to the device range. */
    float       d_mic_gain;     /*!< Mic gain in dB. */
    bool        d_muted;        /*!< Whether the audio path is muted. */
    tx_mod      d_mod;          /*!< Current modulator. */

    std::string tx_devstr;      /*!< Current TX device string. */
    std::string audio_devstr;   /*!< Current audio input device string. */

    gr::top_block_sptr             tb;        /*!< The GNU Radio top block. */
    osmosdr::sink::sptr            snk;       /*!< Real time I/Q sink. */

    tx_mod_nfm_sptr                mod_nfm;   /*!< NFM modulator. */
    tx_mod_am_sptr                 mod_am;    /*!< AM modulator. */
    tx_mod_ssb_sptr                mod_ssb;   /*!< SSB modulator. */

    gr::blocks::multiply_const_ff::sptr    mic_gain;   /*!< Mic gain block. */
    gr::blocks::multiply_const_cc::sptr    rf_power;   /*!< RF power block. */
    gr::filter::pfb_arb_resampler_ccf::sptr resampler; /*!< Audio rate to TX rate. */

#ifdef WITH_PULSEAUDIO
    pa_source_sptr                 audio_src; /*!< Pulse audio source. */
#else
    gr::audio::source::sptr        audio_src; /*!< gr audio source. */
#endif
};

#endif // TRANSMITTER_H
