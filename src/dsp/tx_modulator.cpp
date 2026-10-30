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

#include "dsp/tx_modulator.h"
#include <cmath>
#include <algorithm>

tx_mod_am_sptr make_tx_mod_am(float mod_index)
{
    return gnuradio::get_initial_sptr(new tx_mod_am(mod_index));
}

tx_mod_am::tx_mod_am(float mod_index)
    : gr::sync_block("tx_mod_am",
                     gr::io_signature::make(1, 1, sizeof(float)),
                     gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_mod_index(mod_index)
{
}

void tx_mod_am::set_mod_index(float mod_index)
{
    d_mod_index = std::max(0.0f, std::min(1.0f, mod_index));
}

int tx_mod_am::work(int noutput_items,
                    gr_vector_const_void_star& input_items,
                    gr_vector_void_star& output_items)
{
    const float *in = static_cast<const float*>(input_items[0]);
    gr_complex *out = static_cast<gr_complex*>(output_items[0]);

    for (int i = 0; i < noutput_items; i++)
    {
        float clamped = std::max(-1.0f, std::min(1.0f, in[i]));
        float env = 0.5f * (1.0f + clamped * d_mod_index);
        out[i] = gr_complex(env, 0.0f);
    }

    return noutput_items;
}

tx_modulator_sptr make_tx_modulator(double audio_rate, tx_mod_type mode)
{
    return gnuradio::get_initial_sptr(new tx_modulator(audio_rate, mode));
}

tx_modulator::tx_modulator(double audio_rate, tx_mod_type mode)
    : gr::hier_block2("tx_modulator",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_audio_rate(audio_rate),
      d_mode(mode),
      d_mic_gain_db(0.0f),
      d_ctcss_freq(0.0f),
      d_max_dev(5000.0f)
{
    // Mic gain block
    d_mic_gain = gr::blocks::multiply_const_ff::make(1.0f);

    // CTCSS tone source and adder
    d_ctcss_source = gr::analog::sig_source_f::make(
        d_audio_rate, gr::analog::GR_COS_WAVE, 88.5f, 0.0f, 0.0f);
    d_ctcss_adder = gr::blocks::add_ff::make();

    // AM modulator and bandpass filter
    d_mod_am = make_tx_mod_am(0.9f);
    d_filter_am = make_rx_filter(d_audio_rate, -4500.0, 4500.0, 500.0);

    // NFM modulator and bandpass filter
    float sensitivity = 2.0f * (float)M_PI * d_max_dev / (float)d_audio_rate;
    d_mod_fm = gr::analog::frequency_modulator_fc::make(sensitivity);
    d_filter_fm = make_rx_filter(d_audio_rate, -6000.0, 6000.0, 1000.0);

    // USB modulator (Hilbert transform + bandpass filter)
    d_hilbert_usb = gr::filter::hilbert_fc::make(65);
    d_filter_usb = make_rx_filter(d_audio_rate, 300.0, 2800.0, 400.0);

    // LSB modulator (Hilbert transform + conjugate + bandpass filter)
    d_hilbert_lsb = gr::filter::hilbert_fc::make(65);
    d_conj_lsb = gr::blocks::conjugate_cc::make();
    d_filter_lsb = make_rx_filter(d_audio_rate, -2800.0, -300.0, 400.0);

    // Power / ALC meter
    d_meter = make_rx_meter_c(d_audio_rate);

    // Audio input stage
    connect(self(), 0, d_mic_gain, 0);
    connect(d_mic_gain, 0, d_ctcss_adder, 0);
    connect(d_ctcss_source, 0, d_ctcss_adder, 1);

    // Connect active mode
    connect_current_mode();
}

tx_modulator::~tx_modulator()
{
}

void tx_modulator::connect_current_mode()
{
    switch (d_mode)
    {
    case TX_MOD_AM:
        connect(d_ctcss_adder, 0, d_mod_am, 0);
        connect(d_mod_am, 0, d_filter_am, 0);
        connect(d_filter_am, 0, d_meter, 0);
        connect(d_filter_am, 0, self(), 0);
        break;

    case TX_MOD_USB:
        connect(d_ctcss_adder, 0, d_hilbert_usb, 0);
        connect(d_hilbert_usb, 0, d_filter_usb, 0);
        connect(d_filter_usb, 0, d_meter, 0);
        connect(d_filter_usb, 0, self(), 0);
        break;

    case TX_MOD_LSB:
        connect(d_ctcss_adder, 0, d_hilbert_lsb, 0);
        connect(d_hilbert_lsb, 0, d_conj_lsb, 0);
        connect(d_conj_lsb, 0, d_filter_lsb, 0);
        connect(d_filter_lsb, 0, d_meter, 0);
        connect(d_filter_lsb, 0, self(), 0);
        break;

    case TX_MOD_NFM:
    default:
        connect(d_ctcss_adder, 0, d_mod_fm, 0);
        connect(d_mod_fm, 0, d_filter_fm, 0);
        connect(d_filter_fm, 0, d_meter, 0);
        connect(d_filter_fm, 0, self(), 0);
        break;
    }
}

void tx_modulator::disconnect_current_mode()
{
    switch (d_mode)
    {
    case TX_MOD_AM:
        disconnect(d_ctcss_adder, 0, d_mod_am, 0);
        disconnect(d_mod_am, 0, d_filter_am, 0);
        disconnect(d_filter_am, 0, d_meter, 0);
        disconnect(d_filter_am, 0, self(), 0);
        break;

    case TX_MOD_USB:
        disconnect(d_ctcss_adder, 0, d_hilbert_usb, 0);
        disconnect(d_hilbert_usb, 0, d_filter_usb, 0);
        disconnect(d_filter_usb, 0, d_meter, 0);
        disconnect(d_filter_usb, 0, self(), 0);
        break;

    case TX_MOD_LSB:
        disconnect(d_ctcss_adder, 0, d_hilbert_lsb, 0);
        disconnect(d_hilbert_lsb, 0, d_conj_lsb, 0);
        disconnect(d_conj_lsb, 0, d_filter_lsb, 0);
        disconnect(d_filter_lsb, 0, d_meter, 0);
        disconnect(d_filter_lsb, 0, self(), 0);
        break;

    case TX_MOD_NFM:
    default:
        disconnect(d_ctcss_adder, 0, d_mod_fm, 0);
        disconnect(d_mod_fm, 0, d_filter_fm, 0);
        disconnect(d_filter_fm, 0, d_meter, 0);
        disconnect(d_filter_fm, 0, self(), 0);
        break;
    }
}

void tx_modulator::set_mode(tx_mod_type mode)
{
    if (mode == d_mode)
        return;

    disconnect_current_mode();
    d_mode = mode;
    connect_current_mode();
}

void tx_modulator::set_mic_gain(float gain_db)
{
    d_mic_gain_db = gain_db;
    float gain_linear = std::pow(10.0f, gain_db / 20.0f);
    d_mic_gain->set_k(gain_linear);
}

void tx_modulator::set_ctcss_freq(float freq_hz)
{
    d_ctcss_freq = freq_hz;
    if (freq_hz > 0.0f)
    {
        d_ctcss_source->set_frequency(freq_hz);
        // Sub-audible tone amplitude set to 15% of peak deviation in NFM
        d_ctcss_source->set_amplitude(0.15f);
    }
    else
    {
        d_ctcss_source->set_amplitude(0.0f);
    }
}

void tx_modulator::set_fm_maxdev(float maxdev_hz)
{
    d_max_dev = maxdev_hz;
    float sensitivity = 2.0f * (float)M_PI * d_max_dev / (float)d_audio_rate;
    d_mod_fm->set_sensitivity(sensitivity);
}

float tx_modulator::get_alc_level()
{
    return d_meter->get_level_db();
}
