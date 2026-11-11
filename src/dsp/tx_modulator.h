/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <gnuradio/analog/frequency_modulator_fc.h>
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/add_blk.h>
#include <gnuradio/blocks/float_to_complex.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/hier_block2.h>
#include <memory>
#include <string>
#include <vector>

class tx_modulator;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_modulator> tx_modulator_sptr;
#else
typedef std::shared_ptr<tx_modulator> tx_modulator_sptr;
#endif

tx_modulator_sptr make_tx_modulator(double quad_rate = 48000.0, double max_dev = 5000.0);

/*! \brief Multi-mode SDR transmit modulator (NFM, AM, USB, LSB, CW) with CTCSS & click-free PTT gating (#1336). */
class tx_modulator : public gr::hier_block2
{
public:
    enum tx_mode_t {
        TX_MODE_OFF = 0,
        TX_MODE_NFM = 1,
        TX_MODE_AM  = 2,
        TX_MODE_USB = 3,
        TX_MODE_LSB = 4,
        TX_MODE_CW  = 5
    };

    tx_modulator(double quad_rate, double max_dev);
    ~tx_modulator() override;

    void set_mode(tx_mode_t mode);
    tx_mode_t get_mode() const { return d_mode; }

    void set_ptt(bool enabled);
    bool get_ptt() const { return d_ptt_enabled; }

    void set_max_dev(double max_dev);
    double get_max_dev() const { return d_max_dev; }

    void set_mic_gain_db(double gain_db);
    double get_mic_gain_db() const { return d_mic_gain_db; }

    void set_rf_power(double power_norm);
    double get_rf_power() const { return d_rf_power; }

    void set_ctcss_freq(double freq_hz);
    double get_ctcss_freq() const { return d_ctcss_freq; }

    static bool is_tx_capable_device(const std::string &device_args);

private:
    void rebuild_chain();

    double d_quad_rate;
    double d_max_dev;
    double d_mic_gain_db;
    double d_rf_power;
    double d_ctcss_freq;
    tx_mode_t d_mode;
    bool d_ptt_enabled;

    gr::blocks::multiply_const_ff::sptr d_mic_gain;
    gr::analog::sig_source_f::sptr      d_ctcss_src;
    gr::blocks::add_ff::sptr            d_audio_sum;
    gr::analog::frequency_modulator_fc::sptr d_fm_mod;
    gr::blocks::add_const_ff::sptr      d_am_carrier;
    gr::blocks::float_to_complex::sptr  d_f2c;
    gr::filter::fir_filter_ccc::sptr    d_ssb_filter;
    gr::blocks::multiply_const_cc::sptr d_ptt_gate;
};
