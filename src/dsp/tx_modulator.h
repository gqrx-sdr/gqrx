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
#ifndef TX_MODULATOR_H
#define TX_MODULATOR_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/blocks/conjugate_cc.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/filter/hilbert_fc.h>
#include <cmath>
#include <memory>
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"
#include "dsp/resampler_xx.h"

class tx_mod_am;
class tx_modulator;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_mod_am> tx_mod_am_sptr;
typedef boost::shared_ptr<tx_modulator> tx_modulator_sptr;
#else
typedef std::shared_ptr<tx_mod_am> tx_mod_am_sptr;
typedef std::shared_ptr<tx_modulator> tx_modulator_sptr;
#endif

enum tx_mod_type {
    TX_MOD_OFF = 0,
    TX_MOD_AM  = 1,
    TX_MOD_NFM = 2,
    TX_MOD_USB = 3,
    TX_MOD_LSB = 4,
    TX_MOD_CW  = 5
};

tx_mod_am_sptr make_tx_mod_am(float mod_index = 0.9f);
tx_modulator_sptr make_tx_modulator(double audio_rate, tx_mod_type mode = TX_MOD_NFM, double rf_rate = 2000000.0);

/*! \brief AM modulator block converting normalized audio to complex carrier + sidebands with envelope limiting. */
class tx_mod_am : public gr::sync_block
{
public:
    tx_mod_am(float mod_index = 0.9f);
    ~tx_mod_am() override = default;

    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;

    void set_mod_index(float mod_index);
    float mod_index() const { return d_mod_index; }

private:
    float d_mod_index;
};

/*! \brief Hierarchical TX modulator supporting AM, NFM, USB, LSB, and CW with CTCSS and continuous PTT gating. */
class tx_modulator : public gr::hier_block2
{
public:
    tx_modulator(double audio_rate, tx_mod_type mode = TX_MOD_NFM, double rf_rate = 2000000.0);
    ~tx_modulator() override = default;

    void set_mode(tx_mod_type mode);
    tx_mod_type mode() const { return d_mode; }

    void set_mic_gain(float gain_db);
    float mic_gain() const { return d_mic_gain_db; }

    void set_rf_power(float power_fraction);
    float rf_power() const { return d_rf_power; }

    void set_fm_maxdev(float maxdev_hz);
    float fm_maxdev() const { return d_fm_maxdev; }

    void set_ctcss_freq(float freq_hz);
    float ctcss_freq() const { return d_ctcss_freq; }

    void set_rf_rate(double rf_rate);
    double rf_rate() const { return d_rf_rate; }

    void set_ptt(bool active);
    bool is_ptt() const { return d_ptt; }

    float get_power_level_db();

private:
    void connect_mode(tx_mod_type mode);
    void disconnect_mode(tx_mod_type mode);

    double d_audio_rate;
    double d_rf_rate;
    tx_mod_type d_mode;
    float d_mic_gain_db;
    float d_rf_power;
    float d_fm_maxdev;
    float d_ctcss_freq;
    bool d_ptt;

    // Mic gain & CTCSS
    gr::blocks::multiply_const_ff::sptr d_mic_gain_block;
    gr::analog::sig_source_f::sptr d_ctcss_source;
    gr::blocks::add_blk<float>::sptr d_ctcss_adder;

    // Modulators
    tx_mod_am_sptr d_am_mod;
    gr::analog::frequency_modulator_fc::sptr d_nfm_mod;
    gr::filter::hilbert_fc::sptr d_ssb_hilbert;
    gr::blocks::conjugate_cc::sptr d_ssb_conjugate;
    gr::analog::sig_source_c::sptr d_cw_source;

    // Resampling & output power / PTT gate
    resampler_cc_sptr d_resampler;
    gr::blocks::multiply_const_cc::sptr d_rf_power_block;
    gr::blocks::multiply_const_cc::sptr d_ptt_gate;
    rx_meter_c_sptr d_meter;
};

#endif // TX_MODULATOR_H
