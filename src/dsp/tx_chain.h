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
#ifndef TX_CHAIN_H
#define TX_CHAIN_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/blocks/add_const_ff.h>
#include <gnuradio/blocks/conjugate_cc.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/hilbert_fc.h>
#include <cmath>
#include "dsp/resampler_xx.h"
#include "dsp/rx_meter.h"

class tx_chain;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_chain> tx_chain_sptr;
#else
typedef std::shared_ptr<tx_chain> tx_chain_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of tx_chain. */
tx_chain_sptr make_tx_chain(float audio_rate, float tx_rate);

/*! \brief Multi-mode transmit modulator chain.
 *  \ingroup DSP
 *
 * This class implements modulation for AM, Narrowband FM, Upper Sideband (USB),
 * Lower Sideband (LSB), and Continuous Wave (CW). It handles mic gain, CTCSS
 * tone generation, resampling to the hardware TX rate, and software RF power scaling.
 */
class tx_chain : public gr::hier_block2
{
public:
    enum tx_mode {
        TX_MODE_OFF = 0,
        TX_MODE_NFM = 1,
        TX_MODE_AM  = 2,
        TX_MODE_USB = 3,
        TX_MODE_LSB = 4,
        TX_MODE_CW  = 5
    };

    tx_chain(float audio_rate, float tx_rate);
    ~tx_chain();

    void set_mode(tx_mode mode);
    tx_mode mode() const { return d_mode; }

    void set_mic_gain(float gain_db);
    float mic_gain() const { return d_mic_gain_db; }

    void set_rf_power(float power_fraction);
    float rf_power() const { return d_rf_power; }

    void set_fm_max_dev(float max_dev);
    float fm_max_dev() const { return d_fm_max_dev; }

    void set_ctcss_freq(float freq_hz);
    float ctcss_freq() const { return d_ctcss_freq; }

    void set_tx_rate(float tx_rate);
    float tx_rate() const { return d_tx_rate; }

    float get_power_level_db();

private:
    float d_audio_rate;
    float d_tx_rate;
    tx_mode d_mode;
    float d_mic_gain_db;
    float d_rf_power;
    float d_fm_max_dev;
    float d_ctcss_freq;

    gr::blocks::multiply_const_ff::sptr d_mic_gain;
    gr::analog::sig_source_f::sptr      d_ctcss_source;
    gr::blocks::add_blk<float>::sptr    d_ctcss_adder;

    /* AM modulator blocks */
    gr::blocks::add_const_ff::sptr      d_am_add_const;
    gr::blocks::float_to_complex::sptr  d_am_f2c;

    /* NFM modulator block */
    gr::analog::frequency_modulator_fc::sptr d_fm_mod;

    /* SSB modulator blocks */
    gr::filter::hilbert_fc::sptr        d_ssb_hilbert;
    gr::blocks::conjugate_cc::sptr      d_ssb_conjugate;

    /* CW / Carrier generator block */
    gr::analog::sig_source_c::sptr      d_cw_source;

    /* Output resampler, RF gain, and meter */
    resampler_cc_sptr                   d_resampler;
    gr::blocks::multiply_const_cc::sptr d_rf_gain;
    rx_meter_c_sptr                     d_tx_meter;

    void connect_modulator(tx_mode mode);
    void disconnect_modulator(tx_mode mode);
};

#endif // TX_CHAIN_H
