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
#include "dsp/tx_modulator.h"
#include <gnuradio/io_signature.h>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ----------------------------------------------------------------------
 * tx_mod_am Implementation
 * ---------------------------------------------------------------------- */

tx_mod_am_sptr make_tx_mod_am(float mod_index)
{
    return gnuradio::get_initial_sptr(new tx_mod_am(mod_index));
}

tx_mod_am::tx_mod_am(float mod_index)
    : gr::sync_block("tx_mod_am",
                     gr::io_signature::make(1, 1, sizeof(float)),
                     gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_mod_index(std::max(0.0f, std::min(mod_index, 1.0f)))
{
}

void tx_mod_am::set_mod_index(float mod_index)
{
    d_mod_index = std::max(0.0f, std::min(mod_index, 1.0f));
}

int tx_mod_am::work(int noutput_items,
                    gr_vector_const_void_star& input_items,
                    gr_vector_void_star& output_items)
{
    const auto *in = static_cast<const float*>(input_items[0]);
    auto *out = static_cast<gr_complex*>(output_items[0]);

    for (int i = 0; i < noutput_items; ++i) {
        float sample = std::max(-1.0f, std::min(in[i], 1.0f));
        float env = 0.5f * (1.0f + d_mod_index * sample);
        out[i] = gr_complex(env, 0.0f);
    }
    return noutput_items;
}

/* ----------------------------------------------------------------------
 * tx_modulator Hierarchical Block Implementation
 * ---------------------------------------------------------------------- */

tx_modulator_sptr make_tx_modulator(double audio_rate, tx_mod_type mode, double rf_rate)
{
    return gnuradio::get_initial_sptr(new tx_modulator(audio_rate, mode, rf_rate));
}

tx_modulator::tx_modulator(double audio_rate, tx_mod_type mode, double rf_rate)
    : gr::hier_block2("tx_modulator",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_audio_rate(audio_rate > 0.0 ? audio_rate : 48000.0),
      d_rf_rate(rf_rate > 0.0 ? rf_rate : 2000000.0),
      d_mode(mode),
      d_mic_gain_db(0.0f),
      d_rf_power(1.0f),
      d_fm_maxdev(5000.0f),
      d_ctcss_freq(0.0f),
      d_ptt(false)
{
    // 1. Audio Conditioning & CTCSS
    d_mic_gain_block = gr::blocks::multiply_const_ff::make(1.0f);
    d_ctcss_source = gr::analog::sig_source_f::make(d_audio_rate, gr::analog::GR_COS_WAVE, 0.0, 0.0, 0.0);
    d_ctcss_adder = gr::blocks::add_blk<float>::make(1);

    connect(self(), 0, d_mic_gain_block, 0);
    connect(d_mic_gain_block, 0, d_ctcss_adder, 0);
    connect(d_ctcss_source, 0, d_ctcss_adder, 1);

    // 2. Modulators
    // AM Modulator (envelope limited)
    d_am_mod = make_tx_mod_am(0.9f);
    connect(d_mic_gain_block, 0, d_am_mod, 0);

    // NFM Modulator
    float sensitivity = static_cast<float>((2.0 * M_PI * d_fm_maxdev) / d_audio_rate);
    d_nfm_mod = gr::analog::frequency_modulator_fc::make(sensitivity);
    connect(d_ctcss_adder, 0, d_nfm_mod, 0);

    // SSB Modulator (Hilbert Transform filter)
    d_ssb_hilbert = gr::filter::hilbert_fc::make(65);
    d_ssb_conjugate = gr::blocks::conjugate_cc::make();
    connect(d_mic_gain_block, 0, d_ssb_hilbert, 0);
    connect(d_ssb_hilbert, 0, d_ssb_conjugate, 0);

    // CW Modulator (Cosine source at baseband)
    d_cw_source = gr::analog::sig_source_c::make(d_audio_rate, gr::analog::GR_COS_WAVE, 0.0, 1.0, 0.0);

    // 3. Resampler from Audio Rate to RF Rate
    float resample_ratio = static_cast<float>(d_rf_rate / d_audio_rate);
    d_resampler = make_resampler_cc(resample_ratio);

    // 4. RF Output Scaling & Continuous PTT Gate
    d_rf_power_block = gr::blocks::multiply_const_cc::make(gr_complex(d_rf_power, 0.0f));
    d_ptt_gate = gr::blocks::multiply_const_cc::make(gr_complex(0.0f, 0.0f)); // default idle/muted
    d_meter = make_rx_meter_c(static_cast<float>(d_rf_rate));

    // Connect common post-modulator pipeline
    connect(d_resampler, 0, d_rf_power_block, 0);
    connect(d_rf_power_block, 0, d_ptt_gate, 0);
    connect(d_ptt_gate, 0, d_meter, 0);
    connect(d_ptt_gate, 0, self(), 0);

    // Connect active mode modulator to resampler
    connect_mode(d_mode);
}

void tx_modulator::connect_mode(tx_mod_type mode)
{
    switch (mode) {
    case TX_MOD_AM:
        connect(d_am_mod, 0, d_resampler, 0);
        break;
    case TX_MOD_NFM:
        connect(d_nfm_mod, 0, d_resampler, 0);
        break;
    case TX_MOD_USB:
        connect(d_ssb_hilbert, 0, d_resampler, 0);
        break;
    case TX_MOD_LSB:
        connect(d_ssb_conjugate, 0, d_resampler, 0);
        break;
    case TX_MOD_CW:
        connect(d_cw_source, 0, d_resampler, 0);
        break;
    case TX_MOD_OFF:
    default:
        break;
    }
}

void tx_modulator::disconnect_mode(tx_mod_type mode)
{
    switch (mode) {
    case TX_MOD_AM:
        disconnect(d_am_mod, 0, d_resampler, 0);
        break;
    case TX_MOD_NFM:
        disconnect(d_nfm_mod, 0, d_resampler, 0);
        break;
    case TX_MOD_USB:
        disconnect(d_ssb_hilbert, 0, d_resampler, 0);
        break;
    case TX_MOD_LSB:
        disconnect(d_ssb_conjugate, 0, d_resampler, 0);
        break;
    case TX_MOD_CW:
        disconnect(d_cw_source, 0, d_resampler, 0);
        break;
    case TX_MOD_OFF:
    default:
        break;
    }
}

void tx_modulator::set_mode(tx_mod_type mode)
{
    if (d_mode == mode)
        return;

    disconnect_mode(d_mode);
    d_mode = mode;
    connect_mode(d_mode);
}

void tx_modulator::set_mic_gain(float gain_db)
{
    d_mic_gain_db = gain_db;
    float k = std::pow(10.0f, gain_db / 20.0f);
    d_mic_gain_block->set_k(k);
}

void tx_modulator::set_rf_power(float power_fraction)
{
    d_rf_power = std::max(0.0f, std::min(power_fraction, 1.0f));
    d_rf_power_block->set_k(gr_complex(d_rf_power, 0.0f));
}

void tx_modulator::set_fm_maxdev(float maxdev_hz)
{
    d_fm_maxdev = std::max(500.0f, maxdev_hz);
    float sensitivity = static_cast<float>((2.0 * M_PI * d_fm_maxdev) / d_audio_rate);
    d_nfm_mod->set_sensitivity(sensitivity);
}

void tx_modulator::set_ctcss_freq(float freq_hz)
{
    d_ctcss_freq = freq_hz;
    if (d_ctcss_freq > 0.0f) {
        d_ctcss_source->set_frequency(d_ctcss_freq);
        d_ctcss_source->set_amplitude(0.15f);
    } else {
        d_ctcss_source->set_amplitude(0.0f);
    }
}

void tx_modulator::set_rf_rate(double rf_rate)
{
    if (rf_rate <= 0.0 || rf_rate == d_rf_rate)
        return;

    d_rf_rate = rf_rate;
    if (d_resampler) {
        d_resampler->set_rate(static_cast<float>(d_rf_rate / d_audio_rate));
    }
}

void tx_modulator::set_ptt(bool active)
{
    d_ptt = active;
    if (d_ptt_gate) {
        // Zero-sample gating: 1.0f when transmitting, 0.0f when idle
        d_ptt_gate->set_k(gr_complex(active ? 1.0f : 0.0f, 0.0f));
    }
}

float tx_modulator::get_power_level_db()
{
    return d_meter ? d_meter->get_level_db() : -100.0f;
}
