/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#ifndef RECEIVER_BASE_H
#define RECEIVER_BASE_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/analog/simple_squelch_cc.h>
#include "dsp/resampler_xx.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_agc_xx.h"


class receiver_base_cf;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<receiver_base_cf> receiver_base_cf_sptr;
#else
typedef std::shared_ptr<receiver_base_cf> receiver_base_cf_sptr;
#endif


/*! \brief Base class for receivers that output audio.
 *  \ingroup RX
 *
 * This block provides a base class and common interface for receivers that
 * output audio (or other kind of float data).
 *
 */
class receiver_base_cf : public gr::hier_block2
{

public:
    /*! \brief Public constructor.
     *  \param src_name Descriptive name used in the constructor of gr::hier_block2
     */
    receiver_base_cf(std::string src_name, float pref_quad_rate, float quad_rate, int audio_rate);
    virtual ~receiver_base_cf();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void set_quad_rate(float quad_rate);

    virtual void set_filter(double low, double high, double tw) = 0;
    virtual void set_cw_offset(double offset) = 0;

    virtual float get_signal_level();

    virtual void set_demod(int demod) = 0;

    /* the rest is optional */

    /* Noise blanker */
    virtual bool has_nb();
    virtual void set_nb_on(int nbid, bool on);
    virtual void set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    virtual bool has_sql();
    virtual void set_sql_level(double level_db);
    virtual void set_sql_alpha(double alpha);

    /* AGC */
    virtual bool has_agc();
    virtual void set_agc_on(bool agc_on);
    virtual void set_agc_target_level(int target_level);
    virtual void set_agc_manual_gain(float gain);
    virtual void set_agc_max_gain(int gain);
    virtual void set_agc_attack(int attack_ms);
    virtual void set_agc_decay(int decay_ms);
    virtual void set_agc_hang(int hang_ms);
    virtual float get_agc_gain();

    /* FM parameters */
    virtual bool has_fm();
    virtual void set_fm_maxdev(float maxdev_hz);
    virtual void set_fm_deemph(double tau);

    /* AM parameters */
    virtual bool has_am();
    virtual void set_am_dcr(bool enabled);

    /* AM-Sync parameters */
    virtual bool has_amsync();
    virtual void set_amsync_dcr(bool enabled);
    virtual void set_amsync_pll_bw(float pll_bw);

    virtual void get_rds_data(std::string &outbuff, int &num);
    virtual void start_rds_decoder();
    virtual void stop_rds_decoder();
    virtual void reset_rds_parser();
    virtual bool is_rds_decoder_active();
protected:
    float  d_quad_rate;        /*!< Input sample rate. */
    int    d_audio_rate;       /*!< Audio output rate. */

    resampler_cc_sptr         iq_resamp;   /*!< Baseband resampler. */
    rx_meter_c_sptr           meter;      /*!< Signal strength. */
    rx_agc_2f_sptr            agc;        /*!< Receiver AGC. */
    gr::analog::simple_squelch_cc::sptr sql;        /*!< Squelch. */
private:
    float d_pref_quad_rate;
};

#endif // RECEIVER_BASE_H
