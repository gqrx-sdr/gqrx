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
#include <cmath>
#include <iostream>
#include <QDebug>

#include <gnuradio/io_signature.h>
#include <osmosdr/ranges.h>

#include "applications/gqrx/transmitter.h"

/**
 * @brief Public constructor.
 * @param input_device TX device specifier.
 * @param audio_device Audio input device specifier.
 */
transmitter::transmitter(const std::string input_device,
                         const std::string audio_device)
    : d_running(false),
      d_rf_freq(144500000.0),
      d_rf_power(1.0),
      d_mic_gain(0.0),
      d_muted(false),
      d_mod(TX_MOD_NFM)
{
    tb = gr::make_top_block("gqrx-tx");

    /* modulators work at the audio rate and are device independent */
    mod_nfm = make_tx_mod_nfm(TX_AUDIO_RATE, 5000.0);
    mod_am  = make_tx_mod_am(TX_AUDIO_RATE, 0.9);
    mod_ssb = make_tx_mod_ssb(TX_AUDIO_RATE, true);

    /* software mic gain (hardware agnostic) */
    mic_gain = gr::blocks::multiply_const_ff::make(1.0);

    /* amplitude normalisation of the modulator output */
    rf_power = gr::blocks::multiply_const_cc::make(1.0);

    audio_devstr = audio_device;

    if (!input_device.empty())
    {
        set_tx_device(input_device);
    }
}

transmitter::~transmitter()
{
    tb->stop();
}

/** Start the transmitter (PTT on). */
void transmitter::start()
{
    if (!d_running && snk)
    {
        tb->start();
        d_running = true;
    }
}

/** Stop the transmitter (PTT off). */
void transmitter::stop()
{
    if (d_running)
    {
        tb->stop();
        tb->wait(); // If the graph is needed to run again, wait() must be called after stop
        d_running = false;
    }
}

/**
 * @brief Select new TX device.
 * @param device The new TX device specifier.
 */
void transmitter::set_tx_device(const std::string device)
{
    double rate;

    qDebug() << "Set TX device:";
    qDebug() << "  old:" << tx_devstr.c_str();
    qDebug() << "  new:" << device.c_str();

    if (device.empty())
        return;

    /* nothing to do if the device is already in use */
    if (device == tx_devstr && snk)
        return;

    // tb->lock() can hang occasionally
    if (d_running)
    {
        tb->stop();
        tb->wait();
    }

    tx_devstr = device;

    if (snk)
    {
        tb->disconnect(audio_src, 0, mic_gain, 0);
        tb->disconnect(mic_gain, 0, mod_nfm, 0);
        tb->disconnect(mic_gain, 0, mod_am, 0);
        tb->disconnect(mic_gain, 0, mod_ssb, 0);
        tb->disconnect(rf_power, 0, snk, 0);

        switch (d_mod)
        {
            case TX_MOD_NFM:
                tb->disconnect(mod_nfm, 0, resampler, 0);
                break;
            case TX_MOD_AM:
                tb->disconnect(mod_am, 0, resampler, 0);
                break;
            case TX_MOD_USB:
            case TX_MOD_LSB:
                tb->disconnect(mod_ssb, 0, resampler, 0);
                break;
            default:
                break;
        }

        tb->disconnect(resampler, 0, rf_power, 0);
        snk.reset();
        resampler.reset();
        audio_src.reset();
    }

    /* create the SDR sink */
    snk = osmosdr::sink::make(device);
    snk->set_sample_rate(1e6);
    rate = snk->get_sample_rate();
    snk->set_center_freq(d_rf_freq);
    set_rf_power(d_rf_power);

    /* create the audio-to-RF resampler */
    resampler = gr::filter::pfb_arb_resampler_ccf::make(
                    (float)(rate / TX_AUDIO_RATE));

#ifdef WITH_PULSEAUDIO
    audio_src = make_pa_source(audio_devstr, TX_AUDIO_RATE, 1,
                               "GQRX", "Audio input");
#else
    audio_src = gr::audio::source::make(TX_AUDIO_RATE, audio_devstr, true);
#endif

    /* connect the flow graph */
    tb->connect(audio_src, 0, mic_gain, 0);
    tb->connect(mic_gain, 0, mod_nfm, 0);
    tb->connect(mic_gain, 0, mod_am, 0);
    tb->connect(mic_gain, 0, mod_ssb, 0);

    connect_mod(d_mod);

    tb->connect(resampler, 0, rf_power, 0);
    tb->connect(rf_power, 0, snk, 0);

    if (d_running)
    {
        tb->start();
    }
}

/**
 * @brief Select new audio input device.
 * @param device The new audio device specifier.
 */
void transmitter::set_audio_device(const std::string device)
{
    if (device.empty() || device == audio_devstr)
        return;

    audio_devstr = device;

    if (!snk || !audio_src)
        return; /* will be created together with the TX device */

    /* recreate the audio source */
    tb->lock();

    tb->disconnect(audio_src, 0, mic_gain, 0);
    audio_src.reset();

#ifdef WITH_PULSEAUDIO
    audio_src = make_pa_source(audio_devstr, TX_AUDIO_RATE, 1,
                               "GQRX", "Audio input");
#else
    audio_src = gr::audio::source::make(TX_AUDIO_RATE, audio_devstr, true);
#endif
    tb->connect(audio_src, 0, mic_gain, 0);

    tb->unlock();
}

/**
 * @brief Connect the currently selected modulator to the resampler.
 */
void transmitter::connect_mod(tx_mod mod)
{
    switch (mod)
    {
        case TX_MOD_NFM:
            tb->connect(mod_nfm, 0, resampler, 0);
            break;
        case TX_MOD_AM:
            tb->connect(mod_am, 0, resampler, 0);
            break;
        case TX_MOD_USB:
        case TX_MOD_LSB:
            tb->connect(mod_ssb, 0, resampler, 0);
            break;
        default:
            break;
    }
}

/**
 * @brief Select new modulator.
 * @param mod The new modulator.
 * @sa get_mod()
 */
transmitter::status transmitter::set_mod(tx_mod mod)
{
    if (mod == d_mod)
        return STATUS_OK;

    tb->lock();

    switch (d_mod)
    {
        case TX_MOD_NFM:
            tb->disconnect(mod_nfm, 0, resampler, 0);
            break;
        case TX_MOD_AM:
            tb->disconnect(mod_am, 0, resampler, 0);
            break;
        case TX_MOD_USB:
        case TX_MOD_LSB:
            tb->disconnect(mod_ssb, 0, resampler, 0);
            break;
        default:
            break;
    }

    d_mod = mod;
    connect_mod(mod);

    /* AM output amplitude must be scaled down to fit within +/-1 */
    rf_power->set_k(d_mod == TX_MOD_AM ? 0.5 : 1.0);

    tb->unlock();

    return STATUS_OK;
}

/**
 * @brief Set new RF frequency.
 * @param freq_hz The new frequency in Hz.
 * @sa get_rf_freq()
 */
double transmitter::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;

    if (snk)
        snk->set_center_freq(d_rf_freq);

    return d_rf_freq;
}

/**
 * @brief Get RF frequency.
 * @return The current RF frequency.
 * @sa set_rf_freq()
 */
double transmitter::get_rf_freq(void)
{
    if (snk)
        d_rf_freq = snk->get_center_freq();

    return d_rf_freq;
}

/**
 * @brief Get the RF frequency range of the current TX device.
 * @param start The lower limit of the range in Hz.
 * @param stop  The upper limit of the range in Hz.
 * @param step  The frequency step in Hz.
 * @returns STATUS_OK if the range could be retrieved, STATUS_ERROR if an error has occurred.
 */
transmitter::status transmitter::get_rf_range(double *start, double *stop, double *step)
{
    osmosdr::freq_range_t range;

    if (!snk)
        return STATUS_ERROR;

    range = snk->get_freq_range();

    if (!range.empty())
    {
        if (range.start() < range.stop())
        {
            *start = range.start();
            *stop  = range.stop();
            *step  = range.step();

            return STATUS_OK;
        }
    }

    return STATUS_ERROR;
}

/**
 * @brief Set RF power.
 * @param value Relative RF power between 0.0 and 1.0. The value is
 *              mapped onto the gain range of the TX device.
 */
transmitter::status transmitter::set_rf_power(double value)
{
    osmosdr::gain_range_t range;
    double gain;

    if (value < 0.0)
        value = 0.0;
    else if (value > 1.0)
        value = 1.0;

    d_rf_power = value;

    if (!snk)
        return STATUS_OK;

    range = snk->get_gain_range();
    if (range.start() < range.stop())
    {
        gain = range.start() + value * (range.stop() - range.start());
        snk->set_gain(gain);
    }

    return STATUS_OK;
}

/**
 * @brief Get the gain range of the current TX device.
 * @param start The lower limit in dB.
 * @param stop  The upper limit in dB.
 * @param step  The gain step in dB.
 */
transmitter::status transmitter::get_gain_range(double *start, double *stop, double *step)
{
    osmosdr::gain_range_t range;

    if (!snk)
        return STATUS_ERROR;

    range = snk->get_gain_range();

    if (range.start() < range.stop())
    {
        *start = range.start();
        *stop  = range.stop();
        *step  = range.step();

        return STATUS_OK;
    }

    return STATUS_ERROR;
}

/**
 * @brief Set mic gain.
 * @param gain_db The new mic gain in dB (-20 to +20 dB).
 */
transmitter::status transmitter::set_mic_gain(float gain_db)
{
    float linear;

    if (gain_db > 20.0f)
        gain_db = 20.0f;
    else if (gain_db < -20.0f)
        gain_db = -20.0f;

    d_mic_gain = gain_db;

    if (!d_muted)
    {
        linear = powf(10.0f, gain_db / 20.0f);
        mic_gain->set_k(linear);
    }

    return STATUS_OK;
}

/**
 * @brief Mute or unmute the audio path.
 * @param mute True to mute, false to restore the mic gain setting.
 *
 * The mic gain setting itself is not touched, so unmuting restores
 * the previous gain. Muting is used for click-free PTT: the audio is
 * muted before the flowgraph stops (no cut-off click) and stays muted
 * briefly after the flowgraph starts (no stale audio burst).
 */
void transmitter::set_muted(bool mute)
{
    d_muted = mute;

    if (mute)
        mic_gain->set_k(0.0f);
    else
        mic_gain->set_k(powf(10.0f, d_mic_gain / 20.0f));
}

/** Set maximum FM deviation. */
transmitter::status transmitter::set_max_dev(float max_dev)
{
    mod_nfm->set_max_dev(max_dev);

    return STATUS_OK;
}

/** Set CTCSS tone frequency; 0 disables the tone. */
transmitter::status transmitter::set_ctcss(float freq)
{
    mod_nfm->set_ctcss(freq);

    return STATUS_OK;
}

/** Set AM modulation index. */
transmitter::status transmitter::set_mod_index(float mod_index)
{
    mod_am->set_mod_index(mod_index);

    return STATUS_OK;
}

/**
 * @brief Check whether the given device string refers to a device
 *        with transmit capability.
 * @param device The device specifier, e.g. "hackrf=0" or "soapy=0,driver=lime".
 *
 * Drivers known to have no TX capability are rejected immediately.
 * Soapy devices can be either RX or TX capable and osmosdr::sink
 * can not be trusted for the answer: sink::make() falls back to the
 * first TX-capable device for device strings it can not parse, and
 * Soapy RX-only drivers (sdrplay, airspy, ...) do report channels.
 * For that reason the capability is decided from the soapy driver
 * name; unknown soapy drivers are treated as RX-only to be on the
 * safe side.
 */
bool transmitter::device_supports_tx(const std::string &device)
{
    std::string driver;

    if (device.empty())
        return false;

    /* extract the driver name (the text before the first '=', ',' or space) */
    driver = device;
    {
        std::size_t pos = driver.find_first_of("=, \t");
        if (pos != std::string::npos)
            driver = driver.substr(0, pos);
    }

    static const char *tx_drivers[] = {
        "usrp", "hackrf", "plutosdr", "bladerf", "lime", "xtrx"
    };

    for (const char *drv : tx_drivers)
    {
        if (driver == drv)
            return true;
    }

    if (driver == "soapy")
    {
        /* the actual driver is passed in the "driver=" parameter */
        std::size_t begin = 0;
        while ((begin = device.find(',', begin)) != std::string::npos)
        {
            begin++;
            std::size_t end = device.find(',', begin);
            std::string token = device.substr(begin,
                                              end == std::string::npos
                                                  ? end : end - begin);

            if (token.compare(0, 7, "driver=") == 0)
            {
                std::string soapy_driver = token.substr(7);

                for (const char *drv : tx_drivers)
                {
                    if (soapy_driver == drv)
                        return true;
                }

                return false;
            }
        }

        /* no driver specified; assume RX-only */
        return false;
    }

    return false;
}
