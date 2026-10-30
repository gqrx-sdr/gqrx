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

#include "applications/gqrx/transmitter.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <QDebug>

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_source.h"
#endif

#include <gnuradio/audio/source.h>

#define DEFAULT_TX_AUDIO_RATE 48000.0
#define DEFAULT_TX_RATE       2000000.0

transmitter::transmitter(const std::string &device,
                         const std::string &audio_device,
                         double tx_rate)
    : d_running(false),
      d_sink_valid(false),
      d_tx_devstr(device),
      d_audio_devstr(audio_device),
      d_audio_rate(DEFAULT_TX_AUDIO_RATE),
      d_tx_rate(tx_rate > 0.0 ? tx_rate : DEFAULT_TX_RATE),
      d_tx_freq(145500000.0),
      d_tx_power_db(0.0),
      d_mic_gain_db(0.0f),
      d_ctcss_freq(0.0f),
      d_split_enabled(false),
      d_split_offset(0.0),
      d_swr(1.0f),
      d_mode(TX_MOD_NFM),
      d_last_error("")
{
    d_tb = gr::make_top_block("gqrx_tx");
    build_graph();
}

transmitter::~transmitter()
{
    stop();
}

void transmitter::create_audio_source()
{
    d_audio_src.reset();

#ifdef WITH_PULSEAUDIO
    try
    {
        d_audio_src = make_pa_source(d_audio_devstr, (int)d_audio_rate, 1, "GQRX", "Audio input");
        qDebug() << "Created PulseAudio source for transmitter:" << d_audio_devstr.c_str();
    }
    catch (const std::exception &e)
    {
        qWarning() << "PulseAudio source failed:" << e.what() << "- attempting GNU Radio audio source";
        d_audio_src.reset();
    }
#endif

    if (!d_audio_src)
    {
        try
        {
            d_audio_src = gr::audio::source::make((int)d_audio_rate, d_audio_devstr, true);
            qDebug() << "Created gr::audio::source for transmitter:" << d_audio_devstr.c_str();
        }
        catch (const std::exception &e)
        {
            qWarning() << "gr::audio::source failed:" << e.what() << "- using null audio source fallback";
            d_audio_src = gr::blocks::null_source::make(sizeof(float));
        }
    }

    if (!d_audio_src)
    {
        d_audio_src = gr::blocks::null_source::make(sizeof(float));
    }
}

void transmitter::create_tx_sink()
{
    d_snk.reset();
    d_sink_block.reset();
    d_sink_valid = false;
    d_last_error.clear();

    if (!d_tx_devstr.empty())
    {
        try
        {
            d_snk = osmosdr::sink::make(d_tx_devstr);
            d_sink_block = d_snk;
            d_sink_valid = true;
            qDebug() << "Created osmosdr::sink for device:" << d_tx_devstr.c_str();
        }
        catch (const std::exception &e)
        {
            d_last_error = e.what();
            qWarning() << "Failed to create osmosdr::sink for" << d_tx_devstr.c_str() << ":" << e.what();
        }
    }

    if (!d_sink_block)
    {
        try
        {
            d_snk = osmosdr::sink::make("file=/dev/null");
            d_sink_block = d_snk;
        }
        catch (const std::exception &e)
        {
            qWarning() << "Failed to create fallback null sink:" << e.what();
        }
    }

    if (!d_sink_block)
    {
        d_sink_block = gr::blocks::null_sink::make(sizeof(gr_complex));
    }
    if (d_snk)
    {
        try
        {
            d_snk->set_sample_rate(d_tx_rate);
            d_snk->set_center_freq(d_tx_freq);
            d_snk->set_gain(d_tx_power_db);
        }
        catch (const std::exception &e)
        {
            qWarning() << "Error setting parameters on osmosdr::sink:" << e.what();
        }
    }
}

void transmitter::build_graph()
{
    create_audio_source();
    create_tx_sink();

    d_modulator = make_tx_modulator(d_audio_rate, d_mode);
    d_modulator->set_mic_gain(d_mic_gain_db);
    d_modulator->set_ctcss_freq(d_ctcss_freq);

    float resamp_ratio = (float)(d_tx_rate / d_audio_rate);
    d_resampler = make_resampler_cc(resamp_ratio);

    float rf_gain_linear = (float)std::pow(10.0, d_tx_power_db / 20.0);
    if (rf_gain_linear > 1.0f)
        rf_gain_linear = 1.0f;
    d_rf_gain = gr::blocks::multiply_const_cc::make(rf_gain_linear);

    d_rf_meter = make_rx_meter_c(d_tx_rate);

    d_tb->connect(d_audio_src, 0, d_modulator, 0);
    d_tb->connect(d_modulator, 0, d_resampler, 0);
    d_tb->connect(d_resampler, 0, d_rf_gain, 0);
    d_tb->connect(d_rf_gain, 0, d_rf_meter, 0);
    d_tb->connect(d_rf_gain, 0, d_sink_block, 0);
}

transmitter::status transmitter::start()
{
    if (d_running)
        return STATUS_OK;

    if (d_snk)
    {
        try
        {
            double target_freq = d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq;
            d_snk->set_sample_rate(d_tx_rate);
            d_snk->set_center_freq(target_freq);
            d_snk->set_gain(d_tx_power_db);
        }
        catch (const std::exception &e)
        {
            qWarning() << "Error updating sink settings prior to start:" << e.what();
        }
    }

    try
    {
        d_tb->start();
        d_running = true;
        qDebug() << "Transmitter started on" << (d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq)
                 << "Hz, mode:" << get_tx_mode_str().c_str();
        return STATUS_OK;
    }
    catch (const std::exception &e)
    {
        d_last_error = e.what();
        qWarning() << "Error starting transmitter top block:" << e.what();
        d_running = false;
        return STATUS_ERROR;
    }
}

transmitter::status transmitter::stop()
{
    if (!d_running)
        return STATUS_OK;

    try
    {
        d_tb->stop();
        d_tb->wait();
        d_running = false;
        qDebug() << "Transmitter stopped.";
        return STATUS_OK;
    }
    catch (const std::exception &e)
    {
        d_last_error = e.what();
        qWarning() << "Error stopping transmitter top block:" << e.what();
        d_running = false;
        return STATUS_ERROR;
    }
}

void transmitter::set_tx_device(const std::string &device)
{
    if (device == d_tx_devstr)
        return;

    bool was_running = d_running;
    if (was_running)
        stop();

    d_tx_devstr = device;

    // Disconnect old sink, rebuild graph with new sink
    try
    {
        d_tb->disconnect(d_rf_gain, 0, d_sink_block, 0);
    }
    catch (...) {}

    create_tx_sink();

    try
    {
        d_tb->connect(d_rf_gain, 0, d_sink_block, 0);
    }
    catch (const std::exception &e)
    {
        qWarning() << "Error reconnecting new sink:" << e.what();
    }

    if (was_running)
        start();
}

void transmitter::set_audio_device(const std::string &device)
{
    if (device == d_audio_devstr)
        return;

    bool was_running = d_running;
    if (was_running)
        stop();

    d_audio_devstr = device;

    try
    {
        d_tb->disconnect(d_audio_src, 0, d_modulator, 0);
    }
    catch (...) {}

    create_audio_source();

    try
    {
        d_tb->connect(d_audio_src, 0, d_modulator, 0);
    }
    catch (const std::exception &e)
    {
        qWarning() << "Error reconnecting new audio source:" << e.what();
    }

    if (was_running)
        start();
}

transmitter::status transmitter::set_tx_freq(double freq_hz)
{
    d_tx_freq = freq_hz;
    double target_freq = d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq;

    if (d_snk)
    {
        try
        {
            d_snk->set_center_freq(target_freq);
            return STATUS_OK;
        }
        catch (const std::exception &e)
        {
            d_last_error = e.what();
            return STATUS_ERROR;
        }
    }
    return STATUS_OK;
}

transmitter::status transmitter::set_tx_rate(double rate)
{
    if (rate <= 0.0 || rate == d_tx_rate)
        return STATUS_OK;

    bool was_running = d_running;
    if (was_running)
        stop();

    d_tx_rate = rate;

    // Update resampler
    try
    {
        d_tb->disconnect(d_modulator, 0, d_resampler, 0);
        d_tb->disconnect(d_resampler, 0, d_rf_gain, 0);

        float resamp_ratio = (float)(d_tx_rate / d_audio_rate);
        d_resampler = make_resampler_cc(resamp_ratio);

        d_tb->connect(d_modulator, 0, d_resampler, 0);
        d_tb->connect(d_resampler, 0, d_rf_gain, 0);
    }
    catch (const std::exception &e)
    {
        qWarning() << "Error updating resampler rate:" << e.what();
    }

    if (d_snk)
    {
        try
        {
            d_snk->set_sample_rate(d_tx_rate);
        }
        catch (...) {}
    }

    if (was_running)
        start();

    return STATUS_OK;
}

void transmitter::set_tx_mode(tx_mod_type mode)
{
    d_mode = mode;
    if (d_modulator)
    {
        bool was_running = d_running;
        if (was_running)
            stop();

        d_modulator->set_mode(mode);

        if (was_running)
            start();
    }
}

std::string transmitter::get_tx_mode_str() const
{
    switch (d_mode)
    {
    case TX_MOD_AM:  return "AM";
    case TX_MOD_NFM: return "NFM";
    case TX_MOD_USB: return "USB";
    case TX_MOD_LSB: return "LSB";
    default:         return "OFF";
    }
}

void transmitter::set_mic_gain(float gain_db)
{
    d_mic_gain_db = gain_db;
    if (d_modulator)
        d_modulator->set_mic_gain(gain_db);
}

void transmitter::set_tx_power(double power_db)
{
    d_tx_power_db = power_db;

    if (d_snk)
    {
        try
        {
            d_snk->set_gain(power_db);
        }
        catch (...) {}
    }

    if (d_rf_gain)
    {
        float gain_linear = (float)std::pow(10.0, power_db / 20.0);
        if (gain_linear > 1.0f)
            gain_linear = 1.0f;
        d_rf_gain->set_k(gain_linear);
    }
}

void transmitter::set_ctcss_freq(float freq_hz)
{
    d_ctcss_freq = freq_hz;
    if (d_modulator)
        d_modulator->set_ctcss_freq(freq_hz);
}

void transmitter::set_split_mode(bool enabled)
{
    d_split_enabled = enabled;
    set_tx_freq(d_tx_freq);
}

void transmitter::set_split_offset(double offset_hz)
{
    d_split_offset = offset_hz;
    set_tx_freq(d_tx_freq);
}

float transmitter::get_alc_level()
{
    if (d_modulator)
        return d_modulator->get_alc_level();
    return -100.0f;
}

float transmitter::get_rf_power_level()
{
    if (d_rf_meter)
        return d_rf_meter->get_level_db();
    return -100.0f;
}

std::vector<std::string> transmitter::get_gain_names() const
{
    if (d_snk)
    {
        try
        {
            return d_snk->get_gain_names();
        }
        catch (...) {}
    }
    return std::vector<std::string>();
}

void transmitter::set_gain(const std::string &name, double val)
{
    if (d_snk)
    {
        try
        {
            d_snk->set_gain(val, name);
        }
        catch (...) {}
    }
}

double transmitter::get_gain(const std::string &name) const
{
    if (d_snk)
    {
        try
        {
            return d_snk->get_gain(name);
        }
        catch (...) {}
    }
    return 0.0;
}

std::vector<std::string> transmitter::get_antennas() const
{
    if (d_snk)
    {
        try
        {
            return d_snk->get_antennas();
        }
        catch (...) {}
    }
    return std::vector<std::string>();
}

void transmitter::set_antenna(const std::string &antenna)
{
    if (d_snk && !antenna.empty())
    {
        try
        {
            d_snk->set_antenna(antenna);
        }
        catch (...) {}
    }
}

std::string transmitter::get_status_details() const
{
    std::ostringstream ss;
    double current_freq = d_split_enabled ? (d_tx_freq + d_split_offset) : d_tx_freq;

    ss << (d_running ? "TX ACTIVE" : "TX IDLE") << " | "
       << std::fixed << std::setprecision(4) << (current_freq / 1.0e6) << " MHz | "
       << get_tx_mode_str() << " | "
       << "Pwr: " << std::setprecision(1) << d_tx_power_db << " dB | "
       << "Mic: " << d_mic_gain_db << " dB";

    if (d_split_enabled)
    {
        ss << " | Split: " << std::setprecision(1) << (d_split_offset / 1.0e3) << " kHz";
    }

    if (d_ctcss_freq > 0.0f)
    {
        ss << " | CTCSS: " << std::setprecision(1) << d_ctcss_freq << " Hz";
    }

    if (d_running)
    {
        ss << " | SWR: " << std::setprecision(2) << d_swr;
    }

    return ss.str();
}
