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
#include <cmath>
#include <stdexcept>
#include <iostream>

#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/null_sink.h>

#include "applications/gqrx/transmitter.h"
#include "dsp/tx_mod_nfm.h"
#include "dsp/tx_mod_am.h"
#include "dsp/tx_mod_ssb.h"

/* Convert dB to linear amplitude multiplier */
static inline float db_to_linear(float db)
{
    return std::pow(10.0f, db / 20.0f);
}

/* -----------------------------------------------------------------------
 * Construction / Destruction
 * ----------------------------------------------------------------------- */

transmitter::transmitter(gr::top_block_sptr  tb,
                         const std::string  &output_device,
                         unsigned int        audio_rate,
                         double              rf_rate)
    : d_tb(tb),
      d_output_devstr(output_device),
      d_audio_rate(audio_rate),
      d_rf_rate(rf_rate),
      d_rf_freq(145.5e6),
      d_tx_offset(0.0),
      d_tx_mode(TX_MODE_DISABLED),
      d_mod(TX_MOD_NFM),
      d_tx_enabled(false),
      d_ptt_active(false),
      d_chain_connected(false),
      d_mic_gain_db(0.0f)
{
    d_tx_mode = detect_tx_mode(output_device);

    /* Pre-build PTT gate: gr_complex(0,0) = mute, gr_complex(1,0) = pass */
    d_ptt_gate   = gr::blocks::multiply_const_cc::make(gr_complex(0, 0));
    d_mic_gain   = gr::blocks::multiply_const_ff::make(db_to_linear(d_mic_gain_db));
    d_null_sink  = gr::blocks::null_sink::make(sizeof(gr_complex));
}

transmitter::~transmitter()
{
    try {
        if (d_tx_enabled)
            set_tx_enabled(false);
    } catch (...) {
        /* Swallow all exceptions in destructor */
    }
}

/* -----------------------------------------------------------------------
 * TX enable / disable
 * ----------------------------------------------------------------------- */

transmitter::status transmitter::set_tx_enabled(bool enable)
{
    if (enable == d_tx_enabled)
        return STATUS_OK;

    if (enable) {
        try {
            /* Create sink */
            d_sink = osmosdr::sink::make(d_output_devstr);
            d_sink->set_sample_rate(d_rf_rate);
            d_sink->set_center_freq(d_rf_freq + d_tx_offset, 0);

            /* Create audio source (microphone) */
            d_audio_src = gr::audio::source::make(d_audio_rate, "", true);

            /* Build modulator chain */
            rebuild_modulator();

            /* Connect everything into the shared top block */
            connect_tx_chain();

            d_tx_enabled = true;
        } catch (const std::exception &e) {
            std::cerr << "[TX] set_tx_enabled(true) failed: " << e.what() << std::endl;
            /* Clean up partially created objects */
            d_sink.reset();
            d_audio_src.reset();
            d_modulator.reset();
            return STATUS_ERROR;
        }
    } else {
        /* Ensure PTT is off before tearing down */
        if (d_ptt_active)
            set_ptt(false);

        try {
            disconnect_tx_chain();
        } catch (const std::exception &e) {
            std::cerr << "[TX] disconnect during disable: " << e.what() << std::endl;
        }

        d_sink.reset();
        d_audio_src.reset();
        d_modulator.reset();
        d_tx_enabled = false;
    }

    return STATUS_OK;
}

/* -----------------------------------------------------------------------
 * PTT
 * ----------------------------------------------------------------------- */

transmitter::status transmitter::set_ptt(bool on)
{
    if (!d_tx_enabled)
        return STATUS_ERROR;

    if (d_tx_mode == TX_MODE_FULLDUP) {
        /* Simply open/close the gate block — no flowgraph restart needed */
        d_ptt_gate->set_k(on ? gr_complex(1, 0) : gr_complex(0, 0));
        d_ptt_active = on;
        return STATUS_OK;
    }

    if (d_tx_mode == TX_MODE_HALFDUP) {
        /* Half-duplex: receiver must be stopped before we transmit.
         * The MainWindow is responsible for calling receiver::pause_for_tx()
         * before calling set_ptt(true), and receiver::resume_from_tx()
         * after set_ptt(false).  We just open/close the gate here. */
        d_ptt_gate->set_k(on ? gr_complex(1, 0) : gr_complex(0, 0));
        d_ptt_active = on;
        return STATUS_OK;
    }

    return STATUS_ERROR; /* TX_MODE_DISABLED */
}

/* -----------------------------------------------------------------------
 * Modulation
 * ----------------------------------------------------------------------- */

transmitter::status transmitter::set_modulation(tx_modulation mod)
{
    if (mod == d_mod)
        return STATUS_OK;

    d_mod = mod;

    if (d_tx_enabled) {
        try {
            d_tb->lock();
            /* Disconnect current chain, rebuild modulator, reconnect */
            disconnect_tx_chain();
            rebuild_modulator();
            connect_tx_chain();
            d_tb->unlock();
        } catch (const std::exception &e) {
            d_tb->unlock();
            std::cerr << "[TX] set_modulation failed: " << e.what() << std::endl;
            return STATUS_ERROR;
        }
    }

    return STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Frequency
 * ----------------------------------------------------------------------- */

transmitter::status transmitter::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;
    if (d_sink)
        d_sink->set_center_freq(freq_hz + d_tx_offset, 0);
    return STATUS_OK;
}

transmitter::status transmitter::set_tx_offset(double offset_hz)
{
    d_tx_offset = offset_hz;
    if (d_sink)
        d_sink->set_center_freq(d_rf_freq + offset_hz, 0);
    return STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Gains
 * ----------------------------------------------------------------------- */

std::vector<std::string> transmitter::get_tx_gain_names() const
{
    if (d_sink)
        return d_sink->get_gain_names(0);
    return {};
}

transmitter::status transmitter::get_tx_gain_range(const std::string &name,
                                                    double *start,
                                                    double *stop,
                                                    double *step) const
{
    if (!d_sink)
        return STATUS_ERROR;
    osmosdr::gain_range_t r = d_sink->get_gain_range(name, 0);
    if (start) *start = r.start();
    if (stop)  *stop  = r.stop();
    if (step)  *step  = r.step();
    return STATUS_OK;
}

transmitter::status transmitter::set_tx_gain(const std::string &name, double value)
{
    if (!d_sink)
        return STATUS_ERROR;
    d_sink->set_gain(value, name, 0);
    return STATUS_OK;
}

double transmitter::get_tx_gain(const std::string &name) const
{
    if (!d_sink)
        return 0.0;
    return d_sink->get_gain(name, 0);
}

transmitter::status transmitter::set_mic_gain(float gain_db)
{
    d_mic_gain_db = gain_db;
    if (d_mic_gain)
        d_mic_gain->set_k(db_to_linear(gain_db));
    return STATUS_OK;
}

transmitter::status transmitter::set_audio_device(const std::string &device)
{
    /* Only effective before TX is enabled; rebuild chain otherwise */
    if (d_tx_enabled) {
        std::cerr << "[TX] Cannot change audio device while TX is active.\n";
        return STATUS_ERROR;
    }
    /* Store for next enable; audio source string is passed at audio::source::make */
    (void)device; /* TODO: store device name */
    return STATUS_OK;
}

transmitter::status transmitter::set_nfm_maxdev(float dev_hz)
{
    if (d_modulator) {
        auto nfm = std::dynamic_pointer_cast<tx_mod_nfm>(d_modulator);
        if (nfm)
            nfm->set_max_dev(dev_hz);
    }
    return STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Static helper: detect TX capability from device string
 * ----------------------------------------------------------------------- */

transmitter::tx_mode transmitter::detect_tx_mode(const std::string &device_string)
{
    /* HackRF cannot have source and sink open simultaneously */
    if (device_string.find("hackrf") != std::string::npos)
        return TX_MODE_HALFDUP;

    /* Full-duplex capable devices */
    if (device_string.find("uhd")      != std::string::npos ||
        device_string.find("usrp")     != std::string::npos ||
        device_string.find("plutosdr") != std::string::npos ||
        device_string.find("pluto")    != std::string::npos ||
        device_string.find("lime")     != std::string::npos ||
        device_string.find("bladerf")  != std::string::npos ||
        device_string.find("soapy")    != std::string::npos)
        return TX_MODE_FULLDUP;

    return TX_MODE_DISABLED;
}

/* -----------------------------------------------------------------------
 * Private helpers
 * ----------------------------------------------------------------------- */

void transmitter::rebuild_modulator()
{
    switch (d_mod) {
    case TX_MOD_NFM:
        d_modulator = make_tx_mod_nfm(d_audio_rate, d_rf_rate);
        break;
    case TX_MOD_AM:
        d_modulator = make_tx_mod_am(d_audio_rate, d_rf_rate);
        break;
    case TX_MOD_USB:
        d_modulator = make_tx_mod_ssb(d_audio_rate, d_rf_rate, true);
        break;
    case TX_MOD_LSB:
        d_modulator = make_tx_mod_ssb(d_audio_rate, d_rf_rate, false);
        break;
    default:
        /* TX_MOD_OFF: use a null modulator — just route zeros */
        d_modulator = make_tx_mod_nfm(d_audio_rate, d_rf_rate); /* safe fallback */
        break;
    }
}

void transmitter::connect_tx_chain()
{
    if (d_chain_connected)
        return;

    /*
     *  audio_src → mic_gain → modulator → ptt_gate → sink
     *
     *  The gate outputs zeros when PTT is off (full-duplex keeps
     *  the flowgraph running continuously — zero samples are fine
     *  for the hardware sink).
     */
    d_tb->connect(d_audio_src, 0, d_mic_gain,  0);
    d_tb->connect(d_mic_gain,  0, d_modulator, 0);
    d_tb->connect(d_modulator, 0, d_ptt_gate,  0);
    d_tb->connect(d_ptt_gate,  0, d_sink,      0);

    d_chain_connected = true;
}

void transmitter::disconnect_tx_chain()
{
    if (!d_chain_connected)
        return;

    try {
        d_tb->disconnect(d_audio_src, 0, d_mic_gain,  0);
        d_tb->disconnect(d_mic_gain,  0, d_modulator, 0);
        d_tb->disconnect(d_modulator, 0, d_ptt_gate,  0);
        d_tb->disconnect(d_ptt_gate,  0, d_sink,      0);
    } catch (const std::exception &e) {
        std::cerr << "[TX] disconnect_tx_chain: " << e.what() << std::endl;
    }

    d_chain_connected = false;
}

