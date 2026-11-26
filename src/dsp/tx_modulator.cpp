/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <algorithm>
#include <cmath>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/io_signature.h>
#include "dsp/tx_modulator.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

tx_modulator_sptr make_tx_modulator(double quad_rate, double max_dev)
{
    return gnuradio::get_initial_sptr(new tx_modulator(quad_rate, max_dev));
}

tx_modulator::tx_modulator(double quad_rate, double max_dev)
    : gr::hier_block2("tx_modulator",
                      gr::io_signature::make(1, 1, sizeof(float)),
                      gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_quad_rate(quad_rate > 0.0 ? quad_rate : 48000.0),
      d_max_dev(max_dev > 0.0 ? max_dev : 5000.0),
      d_mic_gain_db(0.0),
      d_rf_power(1.0),
      d_ctcss_freq(0.0),
      d_mode(TX_MODE_NFM),
      d_ptt_enabled(false)
{
    d_mic_gain = gr::blocks::multiply_const_ff::make(1.0f);
    d_ctcss_src = gr::analog::sig_source_f::make(d_quad_rate, gr::analog::GR_COS_WAVE, 88.5, 0.0f);
    d_audio_sum = gr::blocks::add_ff::make();

    float sensitivity = static_cast<float>((2.0 * M_PI * d_max_dev) / d_quad_rate);
    d_fm_mod = gr::analog::frequency_modulator_fc::make(sensitivity);
    d_am_carrier = gr::blocks::add_const_ff::make(1.0f);
    d_f2c = gr::blocks::float_to_complex::make();

    std::vector<gr_complex> usb_taps = gr::filter::firdes::complex_band_pass(
        1.0, d_quad_rate, 300.0, 2700.0, 200.0);
    d_ssb_filter = gr::filter::fir_filter_ccc::make(1, usb_taps);

    d_ptt_gate = gr::blocks::multiply_const_cc::make(gr_complex(0.0f, 0.0f));

    rebuild_chain();
}

tx_modulator::~tx_modulator() = default;

bool tx_modulator::is_tx_capable_device(const std::string &device_args)
{
    std::string lower = device_args;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return (lower.find("uhd") != std::string::npos ||
            lower.find("usrp") != std::string::npos ||
            lower.find("hackrf") != std::string::npos ||
            lower.find("plutosdr") != std::string::npos ||
            lower.find("limesdr") != std::string::npos ||
            lower.find("lime") != std::string::npos ||
            lower.find("bladerf") != std::string::npos);
}

void tx_modulator::set_mode(tx_mode_t mode)
{
    if (d_mode == mode)
        return;
    lock();
    d_mode = mode;
    rebuild_chain();
    unlock();
}

void tx_modulator::set_ptt(bool enabled)
{
    d_ptt_enabled = enabled;
    float scale = (d_ptt_enabled && d_mode != TX_MODE_OFF)
                      ? static_cast<float>(d_rf_power)
                      : 0.0f;
    d_ptt_gate->set_k(gr_complex(scale, 0.0f));
}

void tx_modulator::set_max_dev(double max_dev)
{
    if (max_dev <= 0.0)
        return;
    d_max_dev = max_dev;
    float sensitivity = static_cast<float>((2.0 * M_PI * d_max_dev) / d_quad_rate);
    d_fm_mod->set_sensitivity(sensitivity);
}

void tx_modulator::set_mic_gain_db(double gain_db)
{
    d_mic_gain_db = std::max(-30.0, std::min(30.0, gain_db));
    float linear = std::pow(10.0f, static_cast<float>(d_mic_gain_db) / 20.0f);
    d_mic_gain->set_k(linear);
}

void tx_modulator::set_rf_power(double power_norm)
{
    d_rf_power = std::max(0.0, std::min(1.0, power_norm));
    set_ptt(d_ptt_enabled);
}

void tx_modulator::set_ctcss_freq(double freq_hz)
{
    d_ctcss_freq = (freq_hz >= 67.0 && freq_hz <= 254.1) ? freq_hz : 0.0;
    if (d_ctcss_freq > 0.0) {
        d_ctcss_src->set_frequency(d_ctcss_freq);
        d_ctcss_src->set_amplitude(0.15);
    } else {
        d_ctcss_src->set_amplitude(0.0);
    }
}

void tx_modulator::rebuild_chain()
{
    disconnect_all();

    connect(self(), 0, d_mic_gain, 0);
    connect(d_mic_gain, 0, d_audio_sum, 0);
    connect(d_ctcss_src, 0, d_audio_sum, 1);

    if (d_mode == TX_MODE_AM) {
        connect(d_audio_sum, 0, d_am_carrier, 0);
        connect(d_am_carrier, 0, d_f2c, 0);
        connect(d_f2c, 0, d_ptt_gate, 0);
    } else if (d_mode == TX_MODE_USB || d_mode == TX_MODE_LSB) {
        double lo = (d_mode == TX_MODE_USB) ? 300.0 : -2700.0;
        double hi = (d_mode == TX_MODE_USB) ? 2700.0 : -300.0;
        std::vector<gr_complex> taps = gr::filter::firdes::complex_band_pass(
            1.0, d_quad_rate, lo, hi, 200.0);
        d_ssb_filter->set_taps(taps);
        connect(d_audio_sum, 0, d_f2c, 0);
        connect(d_f2c, 0, d_ssb_filter, 0);
        connect(d_ssb_filter, 0, d_ptt_gate, 0);
    } else {
        connect(d_audio_sum, 0, d_fm_mod, 0);
        connect(d_fm_mod, 0, d_ptt_gate, 0);
    }

    connect(d_ptt_gate, 0, self(), 0);
    set_ptt(d_ptt_enabled);
}
