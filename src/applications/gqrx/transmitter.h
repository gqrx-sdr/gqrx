/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024-2026 Gqrx Contributors.
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
#include <memory>

#include <gnuradio/top_block.h>
#include <gnuradio/basic_block.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/null_source.h>
#include <gnuradio/audio/source.h>
#include <osmosdr/sink.h>

#include "dsp/tx_modulator.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_source.h"
#endif

/*! \brief Top-level transmitter class.
 *  \ingroup DSP
 *
 * This class encapsulates the GNU Radio flowgraph for the transmitter.
 * Coordinates audio input, modulation, resampling, and RF sink transmission.
 */
class transmitter
{
public:
    enum status {
        STATUS_OK    = 0,
        STATUS_ERROR = 1
    };

    transmitter(const std::string &tx_device = "",
                const std::string &audio_device = "",
                double sample_rate = 2000000.0);
    virtual ~transmitter();

    status start();
    status stop();
    bool is_transmitting() const;

    status set_ptt(bool active);
    bool get_ptt() const;

    status set_tx_device(const std::string &device);
    std::string get_tx_device() const { return d_tx_devstr; }

    status set_audio_device(const std::string &device);
    std::string get_audio_device() const { return d_audio_devstr; }

    status set_tx_freq(double freq_hz);
    double get_tx_freq() const { return d_tx_freq; }

    status set_tx_mode(tx_mod_type mode);
    tx_mod_type get_tx_mode() const;
    std::string get_tx_mode_str() const;

    status set_tx_power(double power_db_or_pct);
    double get_tx_power() const { return d_tx_power; }

    status set_mic_gain(float gain_db);
    float get_mic_gain() const;

    status set_ctcss_freq(float freq_hz);
    float get_ctcss_freq() const;

    status set_split_mode(bool enabled);
    bool get_split_mode() const { return d_split_enabled; }

    status set_split_offset(double offset_hz);
    double get_split_offset() const { return d_split_offset; }

    status set_tx_rate(double rate);
    double get_tx_rate() const { return d_tx_rate; }

    float get_power_level_db();
    std::string get_status_details() const;

private:
    void create_audio_source();
    void create_tx_sink();
    void build_flowgraph();
    void teardown_flowgraph();

    gr::top_block_sptr d_tb;
    gr::basic_block_sptr d_audio_src;
    tx_modulator_sptr d_modulator;
    osmosdr::sink::sptr d_snk;
    gr::basic_block_sptr d_sink_block;

    std::string d_tx_devstr;
    std::string d_audio_devstr;
    double d_audio_rate;
    double d_tx_rate;
    double d_tx_freq;
    double d_split_offset;
    double d_tx_power;
    bool d_split_enabled;
    bool d_is_transmitting;
    bool d_tb_running;
};

#endif // TRANSMITTER_H
