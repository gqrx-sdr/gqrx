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
#include "applications/gqrx/transmitter.h"
#include <QDebug>
#include <cmath>
#include <iomanip>
#include <sstream>

transmitter::transmitter(const std::string &tx_device,
                         const std::string &audio_device,
                         double sample_rate)
    : d_tx_devstr(tx_device),
      d_audio_devstr(audio_device),
      d_audio_rate(48000.0),
      d_tx_rate(sample_rate > 0.0 ? sample_rate : 2000000.0),
      d_tx_freq(145500000.0),
      d_split_offset(0.0),
      d_tx_power(10.0),
      d_split_enabled(false),
      d_is_transmitting(false),
      d_tb_running(false)
{
    d_tb = gr::make_top_block("gqrx_tx");
    build_flowgraph();
}

transmitter::~transmitter()
{
    stop();
    teardown_flowgraph();
}

void transmitter::create_audio_source()
{
#ifdef WITH_PULSEAUDIO
    try {
        d_audio_src = make_pa_source(d_audio_devstr, static_cast<int>(d_audio_rate), 1, "Gqrx", "TX Audio Input");
        if (d_audio_src) {
            qDebug() << "Created PulseAudio source for transmitter:" << d_audio_devstr.c_str();
            return;
        }
    }
    catch (const std::exception &e) {
        qWarning() << "PulseAudio source failed:" << e.what() << "- attempting gr::audio::source fallback";
    }
#endif

    try {
        d_audio_src = gr::audio::source::make(static_cast<int>(d_audio_rate), d_audio_devstr, true);
        if (d_audio_src) {
            qDebug() << "Created gr::audio::source for transmitter:" << d_audio_devstr.c_str();
            return;
        }
    }
    catch (const std::exception &e) {
        qWarning() << "gr::audio::source failed:" << e.what() << "- using null audio source fallback";
    }

    d_audio_src = gr::blocks::null_source::make(sizeof(float));
}

void transmitter::create_tx_sink()
{
    d_snk.reset();
    d_sink_block.reset();

    if (!d_tx_devstr.empty() && d_tx_devstr.find("file=/dev/null") == std::string::npos) {
        try {
            d_snk = osmosdr::sink::make(d_tx_devstr);
            if (d_snk) {
                d_snk->set_sample_rate(d_tx_rate);
                double freq = d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq;
                d_snk->set_center_freq(freq);
                d_sink_block = d_snk;
                qDebug() << "Created osmosdr::sink for device:" << d_tx_devstr.c_str();
                return;
            }
        }
        catch (const std::exception &e) {
            qWarning() << "Failed to create osmosdr::sink for" << d_tx_devstr.c_str() << ":" << e.what();
        }
    }

    // Fallback to null sink
    d_sink_block = gr::blocks::null_sink::make(sizeof(gr_complex));
}

void transmitter::build_flowgraph()
{
    create_audio_source();
    d_modulator = make_tx_modulator(d_audio_rate, TX_MOD_NFM, d_tx_rate);
    create_tx_sink();

    if (d_tb && d_audio_src && d_modulator && d_sink_block) {
        try {
            d_tb->connect(d_audio_src, 0, d_modulator, 0);
            d_tb->connect(d_modulator, 0, d_sink_block, 0);
        }
        catch (const std::exception &e) {
            qWarning() << "Error connecting transmitter flowgraph:" << e.what();
        }
    }
}

void transmitter::teardown_flowgraph()
{
    if (d_tb) {
        try {
            if (d_tb_running) {
                d_tb->stop();
                d_tb->wait();
                d_tb_running = false;
            }
            d_tb->disconnect_all();
        }
        catch (const std::exception &e) {
            qWarning() << "Error tearing down transmitter flowgraph:" << e.what();
        }
    }
}

transmitter::status transmitter::start()
{
    if (d_is_transmitting)
        return STATUS_OK;

    if (!d_tb_running && d_tb) {
        try {
            d_tb->start();
            d_tb_running = true;
        }
        catch (const std::exception &e) {
            qWarning() << "Error starting transmitter top block:" << e.what();
            return STATUS_ERROR;
        }
    }

    set_ptt(true);
    return STATUS_OK;
}

transmitter::status transmitter::stop()
{
    set_ptt(false);
    return STATUS_OK;
}

bool transmitter::is_transmitting() const
{
    return d_is_transmitting;
}

transmitter::status transmitter::set_ptt(bool active)
{
    d_is_transmitting = active;
    if (d_modulator) {
        d_modulator->set_ptt(active);
    }
    return STATUS_OK;
}

bool transmitter::get_ptt() const
{
    return d_is_transmitting;
}

transmitter::status transmitter::set_tx_device(const std::string &device)
{
    if (d_tx_devstr == device)
        return STATUS_OK;

    bool was_running = d_is_transmitting;
    stop();
    teardown_flowgraph();

    d_tx_devstr = device;
    build_flowgraph();

    if (was_running)
        start();

    return STATUS_OK;
}

transmitter::status transmitter::set_audio_device(const std::string &device)
{
    if (d_audio_devstr == device)
        return STATUS_OK;

    bool was_running = d_is_transmitting;
    stop();
    teardown_flowgraph();

    d_audio_devstr = device;
    build_flowgraph();

    if (was_running)
        start();

    return STATUS_OK;
}

transmitter::status transmitter::set_tx_freq(double freq_hz)
{
    if (freq_hz <= 0.0)
        return STATUS_ERROR;

    d_tx_freq = freq_hz;
    if (d_snk) {
        try {
            double effective_freq = d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq;
            d_snk->set_center_freq(effective_freq);
        }
        catch (const std::exception &e) {
            qWarning() << "Error setting TX center frequency:" << e.what();
            return STATUS_ERROR;
        }
    }
    return STATUS_OK;
}

transmitter::status transmitter::set_tx_mode(tx_mod_type mode)
{
    if (d_modulator) {
        d_modulator->set_mode(mode);
    }
    return STATUS_OK;
}

tx_mod_type transmitter::get_tx_mode() const
{
    return d_modulator ? d_modulator->mode() : TX_MOD_OFF;
}

std::string transmitter::get_tx_mode_str() const
{
    switch (get_tx_mode()) {
    case TX_MOD_AM:  return "AM";
    case TX_MOD_NFM: return "NFM";
    case TX_MOD_USB: return "USB";
    case TX_MOD_LSB: return "LSB";
    case TX_MOD_CW:  return "CW";
    case TX_MOD_OFF:
    default:         return "OFF";
    }
}

transmitter::status transmitter::set_tx_power(double power_db_or_pct)
{
    d_tx_power = power_db_or_pct;
    if (d_modulator) {
        float frac = static_cast<float>(power_db_or_pct > 1.0 ? power_db_or_pct / 100.0 : power_db_or_pct);
        d_modulator->set_rf_power(frac);
    }
    if (d_snk) {
        try {
            // Also attempt hardware RF gain on SDR sink
            std::vector<std::string> gains = d_snk->get_gain_names();
            for (const auto &g : gains) {
                osmosdr::gain_range_t range = d_snk->get_gain_range(g);
                double target = range.start() + (range.stop() - range.start()) * (d_tx_power / 100.0);
                d_snk->set_gain(target, g);
            }
        }
        catch (...) {}
    }
    return STATUS_OK;
}

transmitter::status transmitter::set_mic_gain(float gain_db)
{
    if (d_modulator) {
        d_modulator->set_mic_gain(gain_db);
    }
    return STATUS_OK;
}

float transmitter::get_mic_gain() const
{
    return d_modulator ? d_modulator->mic_gain() : 0.0f;
}

transmitter::status transmitter::set_ctcss_freq(float freq_hz)
{
    if (d_modulator) {
        d_modulator->set_ctcss_freq(freq_hz);
    }
    return STATUS_OK;
}

float transmitter::get_ctcss_freq() const
{
    return d_modulator ? d_modulator->ctcss_freq() : 0.0f;
}

transmitter::status transmitter::set_split_mode(bool enabled)
{
    d_split_enabled = enabled;
    return set_tx_freq(d_tx_freq);
}

transmitter::status transmitter::set_split_offset(double offset_hz)
{
    d_split_offset = offset_hz;
    if (d_split_enabled) {
        return set_tx_freq(d_tx_freq);
    }
    return STATUS_OK;
}

transmitter::status transmitter::set_tx_rate(double rate)
{
    if (rate <= 0.0)
        return STATUS_ERROR;

    d_tx_rate = rate;
    if (d_modulator) {
        d_modulator->set_rf_rate(rate);
    }
    if (d_snk) {
        try {
            d_snk->set_sample_rate(d_tx_rate);
        }
        catch (...) {}
    }
    return STATUS_OK;
}

float transmitter::get_power_level_db()
{
    return d_modulator ? d_modulator->get_power_level_db() : -100.0f;
}

std::string transmitter::get_status_details() const
{
    std::ostringstream ss;
    double effective_freq = d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq;
    ss << std::fixed << std::setprecision(4) << (effective_freq / 1e6) << " MHz | "
       << get_tx_mode_str() << " | "
       << "Pwr: " << std::fixed << std::setprecision(1) << d_tx_power << " dB | "
       << "Mic: " << std::fixed << std::setprecision(1) << get_mic_gain() << " dB";

    if (d_split_enabled) {
        ss << " | Split: " << std::fixed << std::setprecision(1) << (d_split_offset / 1e3) << " kHz";
    }
    if (get_ctcss_freq() > 0.0f) {
        ss << " | CTCSS: " << std::fixed << std::setprecision(1) << get_ctcss_freq() << " Hz";
    }
    return ss.str();
}
