/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Contributors.
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

#include <string>
#include <vector>

/* Only include what transmitter.h actually uses in its class definition.
 * Modulator-specific headers (nbfm_tx, hilbert_fc, etc.) belong in
 * the individual tx_mod_*.h / tx_mod_*.cpp files only. */
#include <gnuradio/top_block.h>
#include <gnuradio/audio/source.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/null_sink.h>
#include <osmosdr/sink.h>

#include "dsp/tx_modulator.h"

/**
 * @brief Transmitter class — TX counterpart to receiver.
 * @ingroup DSP
 *
 * Owns the TX signal chain within the shared GNU Radio top block.
 * Completely isolated from the receiver signal path.
 * A failure here cannot affect the existing RX chain.
 */
class transmitter
{
public:

    /** Operation status codes — mirroring receiver::status */
    enum status {
        STATUS_OK    = 0,
        STATUS_ERROR = 1
    };

    /** Supported TX modulation modes */
    enum tx_modulation {
        TX_MOD_OFF  = 0,  /*!< No modulator active. */
        TX_MOD_NFM  = 1,  /*!< Narrowband FM. */
        TX_MOD_AM   = 2,  /*!< Amplitude Modulation (DSB-FC). */
        TX_MOD_USB  = 3,  /*!< Upper Single Sideband. */
        TX_MOD_LSB  = 4,  /*!< Lower Single Sideband. */
    };

    /** Duplex strategy — determined from device string at construction */
    enum tx_mode {
        TX_MODE_FULLDUP  = 0,  /*!< Full-duplex: sink always running, gate controls TX. */
        TX_MODE_HALFDUP  = 1,  /*!< Half-duplex (HackRF): stop/start required. */
        TX_MODE_DISABLED = 2,  /*!< TX not supported by this device. */
    };

    transmitter(gr::top_block_sptr         tb,
                const std::string         &output_device,
                unsigned int               audio_rate,
                double                     rf_rate);
    ~transmitter();

    /* Lifecycle */
    status  set_tx_enabled(bool enable);
    bool    is_tx_enabled() const { return d_tx_enabled; }

    /* PTT */
    status  set_ptt(bool on);
    bool    ptt_active()  const { return d_ptt_active; }

    /* Modulation */
    status  set_modulation(tx_modulation mod);
    tx_modulation get_modulation() const { return d_mod; }

    /* Tuning */
    status  set_rf_freq(double freq_hz);
    double  get_rf_freq() const { return d_rf_freq; }

    /* Gains */
    std::vector<std::string> get_tx_gain_names() const;
    status  get_tx_gain_range(const std::string &name,
                              double *start, double *stop, double *step) const;
    status  set_tx_gain(const std::string &name, double value);
    double  get_tx_gain(const std::string &name) const;

    /* Mic/audio */
    status  set_mic_gain(float gain_db);
    float   get_mic_gain() const { return d_mic_gain_db; }

    /* TX audio input device (mic) */
    status  set_audio_device(const std::string &device);

    /* TX frequency offset for split operation */
    status  set_tx_offset(double offset_hz);
    double  get_tx_offset() const { return d_tx_offset; }

    /* NFM deviation */
    status  set_nfm_maxdev(float dev_hz);

    /* Device capability query */
    tx_mode get_tx_mode() const { return d_tx_mode; }
    bool    is_tx_capable() const { return d_tx_mode != TX_MODE_DISABLED; }

    /* Static helper: infer TX mode from a device string */
    static tx_mode detect_tx_mode(const std::string &device_string);

private:
    void connect_tx_chain();
    void disconnect_tx_chain();
    void rebuild_modulator();

    /* Shared top block — owned by receiver, borrowed here */
    gr::top_block_sptr           d_tb;

    /* Device / rate */
    std::string                  d_output_devstr;
    unsigned int                 d_audio_rate;   /*!< Microphone sample rate (Hz). */
    double                       d_rf_rate;      /*!< SDR sample rate (Hz). */
    double                       d_rf_freq;      /*!< Current TX centre frequency (Hz). */
    double                       d_tx_offset;    /*!< Split frequency offset (Hz). */

    /* State */
    tx_mode                      d_tx_mode;
    tx_modulation                d_mod;
    bool                         d_tx_enabled;
    bool                         d_ptt_active;
    bool                         d_chain_connected;
    float                        d_mic_gain_db;

    /* GNU Radio blocks */
    osmosdr::sink::sptr              d_sink;        /*!< TX hardware sink. */
    gr::audio::source::sptr          d_audio_src;   /*!< Microphone source. */
    tx_modulator_base_sptr           d_modulator;   /*!< Active modulator chain. */

    /* PTT gate: gr_complex(1,0) = TX on, gr_complex(0,0) = mute */
    gr::blocks::multiply_const_cc::sptr  d_ptt_gate;

    /* Audio pre-gain */
    gr::blocks::multiply_const_ff::sptr  d_mic_gain;

    /* Null sink used to consume modulator output when TX disabled (keeps graph balanced) */
    gr::blocks::null_sink::sptr          d_null_sink;
};

#endif // TRANSMITTER_H

