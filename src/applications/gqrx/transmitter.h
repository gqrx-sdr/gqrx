/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Authors
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
#include <gnuradio/blocks/null_source.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/multiply_const.h>
#include <osmosdr/sink.h>
#include "dsp/tx_modulator.h"
#include "dsp/resampler_xx.h"
#include "dsp/rx_meter.h"

/*! \brief Top-level transmitter class managing the TX DSP pipeline and SDR sink. */
class transmitter
{
public:
    enum status {
        STATUS_OK = 0,
        STATUS_ERROR = 1
    };

    transmitter(const std::string &device = "",
                const std::string &audio_device = "",
                double tx_rate = 2000000.0);
    ~transmitter();

    status      start();
    status      stop();
    bool        is_transmitting() const { return d_running; }

    void        set_tx_device(const std::string &device);
    std::string get_tx_device() const { return d_tx_devstr; }

    void        set_audio_device(const std::string &device);
    std::string get_audio_device() const { return d_audio_devstr; }

    status      set_tx_freq(double freq_hz);
    double      get_tx_freq() const { return d_tx_freq; }

    status      set_tx_rate(double rate);
    double      get_tx_rate() const { return d_tx_rate; }

    void        set_tx_mode(tx_mod_type mode);
    tx_mod_type get_tx_mode() const { return d_mode; }
    std::string get_tx_mode_str() const;

    void        set_mic_gain(float gain_db);
    float       get_mic_gain() const { return d_mic_gain_db; }

    void        set_tx_power(double power_db);
    double      get_tx_power() const { return d_tx_power_db; }

    void        set_ctcss_freq(float freq_hz);
    float       get_ctcss_freq() const { return d_ctcss_freq; }

    void        set_split_mode(bool enabled);
    bool        get_split_mode() const { return d_split_enabled; }

    void        set_split_offset(double offset_hz);
    double      get_split_offset() const { return d_split_offset; }

    float       get_alc_level();
    float       get_rf_power_level();
    float       get_swr() const { return d_swr; }

    std::vector<std::string> get_gain_names() const;
    void        set_gain(const std::string &name, double val);
    double      get_gain(const std::string &name) const;

    std::vector<std::string> get_antennas() const;
    void        set_antenna(const std::string &antenna);

    bool        is_sink_valid() const { return d_sink_valid; }
    std::string get_last_error() const { return d_last_error; }
    std::string get_status_details() const;

private:
    void        create_audio_source();
    void        create_tx_sink();
    void        build_graph();

private:
    bool        d_running;
    bool        d_sink_valid;
    std::string d_tx_devstr;
    std::string d_audio_devstr;
    double      d_audio_rate;
    double      d_tx_rate;
    double      d_tx_freq;
    double      d_tx_power_db;
    float       d_mic_gain_db;
    float       d_ctcss_freq;
    bool        d_split_enabled;
    double      d_split_offset;
    float       d_swr;
    tx_mod_type d_mode;
    std::string d_last_error;

    gr::top_block_sptr                  d_tb;
    gr::basic_block_sptr                d_audio_src;
    tx_modulator_sptr                   d_modulator;
    resampler_cc_sptr                   d_resampler;
    gr::blocks::multiply_const_cc::sptr d_rf_gain;
    rx_meter_c_sptr                     d_rf_meter;
    osmosdr::sink::sptr                 d_snk;
    gr::basic_block_sptr                d_sink_block;
};

#endif // TRANSMITTER_H
