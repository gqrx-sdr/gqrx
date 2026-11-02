/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx Contributors.
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
#include "dsp/tx_chain.h"
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

tx_chain_sptr make_tx_chain(float audio_rate, float tx_rate)
{
    return gnuradio::get_initial_sptr(new tx_chain(audio_rate, tx_rate));
}

tx_chain::tx_chain(float audio_rate, float tx_rate)
    : gr::hier_block2("tx_chain",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_audio_rate(audio_rate),
      d_tx_rate(tx_rate),
      d_mode(TX_MODE_NFM),
      d_mic_gain_db(0.0f),
      d_rf_power(0.5f),
      d_fm_max_dev(5000.0f),
      d_ctcss_freq(0.0f)
{
    // Audio conditioning & CTCSS
    d_mic_gain = gr::blocks::multiply_const_ff::make(1.0f);
    d_ctcss_source = gr::analog::sig_source_f::make(d_audio_rate, gr::analog::GR_COS_WAVE, 0.0, 0.0, 0.0);
    d_ctcss_adder = gr::blocks::add_blk<float>::make(1);

    connect(self(), 0, d_mic_gain, 0);
    connect(d_mic_gain, 0, d_ctcss_adder, 0);
    connect(d_ctcss_source, 0, d_ctcss_adder, 1);

    // AM modulator
    d_am_add_const = gr::blocks::add_const_ff::make(1.0f);
    d_am_f2c = gr::blocks::float_to_complex::make(1);
    connect(d_mic_gain, 0, d_am_add_const, 0);
    connect(d_am_add_const, 0, d_am_f2c, 0);

    // NFM modulator
    float sensitivity = (2.0f * (float)M_PI * d_fm_max_dev) / d_audio_rate;
    d_fm_mod = gr::analog::frequency_modulator_fc::make(sensitivity);
    connect(d_ctcss_adder, 0, d_fm_mod, 0);

    // SSB modulator
    d_ssb_hilbert = gr::filter::hilbert_fc::make(65);
    d_ssb_conjugate = gr::blocks::conjugate_cc::make();
    connect(d_mic_gain, 0, d_ssb_hilbert, 0);
    connect(d_ssb_hilbert, 0, d_ssb_conjugate, 0);

    // CW modulator
    d_cw_source = gr::analog::sig_source_c::make(d_audio_rate, gr::analog::GR_COS_WAVE, 0.0, 1.0, 0.0);

    // Resampler to hardware TX rate
    d_resampler = make_resampler_cc(d_tx_rate / d_audio_rate);

    // Power gain scaling
    d_rf_gain = gr::blocks::multiply_const_cc::make(gr_complex(d_rf_power, 0.0f));

    // Meter for measuring TX power
    d_tx_meter = make_rx_meter_c(d_tx_rate);

    // Connect common output path
    connect(d_resampler, 0, d_rf_gain, 0);
    connect(d_rf_gain, 0, d_tx_meter, 0);
    connect(d_rf_gain, 0, self(), 0);

    // Connect default modulator (NFM)
    connect_modulator(d_mode);
}

tx_chain::~tx_chain()
{
}

void tx_chain::connect_modulator(tx_mode mode)
{
    switch (mode) {
    case TX_MODE_AM:
        connect(d_am_f2c, 0, d_resampler, 0);
        break;
    case TX_MODE_NFM:
        connect(d_fm_mod, 0, d_resampler, 0);
        break;
    case TX_MODE_USB:
        connect(d_ssb_hilbert, 0, d_resampler, 0);
        break;
    case TX_MODE_LSB:
        connect(d_ssb_conjugate, 0, d_resampler, 0);
        break;
    case TX_MODE_CW:
        connect(d_cw_source, 0, d_resampler, 0);
        break;
    case TX_MODE_OFF:
    default:
        break;
    }
}

void tx_chain::disconnect_modulator(tx_mode mode)
{
    switch (mode) {
    case TX_MODE_AM:
        disconnect(d_am_f2c, 0, d_resampler, 0);
        break;
    case TX_MODE_NFM:
        disconnect(d_fm_mod, 0, d_resampler, 0);
        break;
    case TX_MODE_USB:
        disconnect(d_ssb_hilbert, 0, d_resampler, 0);
        break;
    case TX_MODE_LSB:
        disconnect(d_ssb_conjugate, 0, d_resampler, 0);
        break;
    case TX_MODE_CW:
        disconnect(d_cw_source, 0, d_resampler, 0);
        break;
    case TX_MODE_OFF:
    default:
        break;
    }
}

void tx_chain::set_mode(tx_mode mode)
{
    if (d_mode == mode)
        return;

    disconnect_modulator(d_mode);
    d_mode = mode;
    connect_modulator(d_mode);
}

void tx_chain::set_mic_gain(float gain_db)
{
    d_mic_gain_db = gain_db;
    float k = std::pow(10.0f, gain_db / 20.0f);
    d_mic_gain->set_k(k);
}

void tx_chain::set_rf_power(float power_fraction)
{
    d_rf_power = power_fraction;
    d_rf_gain->set_k(gr_complex(power_fraction, 0.0f));
}

void tx_chain::set_fm_max_dev(float max_dev)
{
    d_fm_max_dev = max_dev;
    float sensitivity = (2.0f * (float)M_PI * d_fm_max_dev) / d_audio_rate;
    d_fm_mod->set_sensitivity(sensitivity);
}

void tx_chain::set_ctcss_freq(float freq_hz)
{
    d_ctcss_freq = freq_hz;
    if (d_ctcss_freq > 0.0f) {
        d_ctcss_source->set_frequency(d_ctcss_freq);
        d_ctcss_source->set_amplitude(0.15f);
    } else {
        d_ctcss_source->set_amplitude(0.0f);
    }
}

void tx_chain::set_tx_rate(float tx_rate)
{
    d_tx_rate = tx_rate;
    if (d_resampler) {
        d_resampler->set_rate(d_tx_rate / d_audio_rate);
    }
}

float tx_chain::get_power_level_db()
{
    return d_tx_meter ? d_tx_meter->get_level_db() : -100.0f;
}
