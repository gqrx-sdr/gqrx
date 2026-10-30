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
#pragma once

#include <gnuradio/hier_block2.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/blocks/conjugate_cc.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/filter/hilbert_fc.h>
#include <memory>
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"

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
    TX_MOD_LSB = 4
};

tx_mod_am_sptr make_tx_mod_am(float mod_index = 0.9f);
tx_modulator_sptr make_tx_modulator(double audio_rate, tx_mod_type mode = TX_MOD_NFM);

/*! \brief AM modulator block converting normalized audio to complex carrier + sidebands. */
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

/*! \brief Hierarchical TX modulator supporting AM, NFM, USB, and LSB modes with CTCSS. */
class tx_modulator : public gr::hier_block2
{
public:
    tx_modulator(double audio_rate, tx_mod_type mode = TX_MOD_NFM);
    ~tx_modulator() override;

    void set_mode(tx_mod_type mode);
    tx_mod_type mode() const { return d_mode; }

    void set_mic_gain(float gain_db);
    float mic_gain() const { return d_mic_gain_db; }

    void set_ctcss_freq(float freq_hz);
    float ctcss_freq() const { return d_ctcss_freq; }

    void set_fm_maxdev(float maxdev_hz);
    float fm_maxdev() const { return d_max_dev; }

    float get_alc_level();

private:
    void connect_current_mode();
    void disconnect_current_mode();

private:
    double d_audio_rate;
    tx_mod_type d_mode;
    float d_mic_gain_db;
    float d_ctcss_freq;
    float d_max_dev;

    gr::blocks::multiply_const_ff::sptr d_mic_gain;
    gr::analog::sig_source_f::sptr d_ctcss_source;
    gr::blocks::add_ff::sptr d_ctcss_adder;

    tx_mod_am_sptr d_mod_am;
    rx_filter_sptr d_filter_am;

    gr::analog::frequency_modulator_fc::sptr d_mod_fm;
    rx_filter_sptr d_filter_fm;

    gr::filter::hilbert_fc::sptr d_hilbert_usb;
    rx_filter_sptr d_filter_usb;

    gr::filter::hilbert_fc::sptr d_hilbert_lsb;
    gr::blocks::conjugate_cc::sptr d_conj_lsb;
    rx_filter_sptr d_filter_lsb;

    rx_meter_c_sptr d_meter;
};
