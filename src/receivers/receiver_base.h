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

#define RECEIVER_NB_COUNT 2
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
    receiver_base_cf(std::string src_name);
    virtual ~receiver_base_cf();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void set_quad_rate(float quad_rate) = 0;
    virtual void set_audio_rate(float audio_rate) = 0;

    virtual void set_filter(double low, double high, double tw);
    virtual void set_cw_offset(double offset);

    virtual float get_signal_level() = 0;

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
    virtual void set_agc_hang(bool use_hang);
    virtual void set_agc_threshold(int threshold);
    virtual void set_agc_slope(int slope);
    virtual void set_agc_decay(int decay_ms);
    virtual void set_agc_manual_gain(int gain);

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

    virtual void restore_settings(receiver_base_cf_sptr from);
protected:
    double d_filter_low;
    double d_filter_high;
    double d_filter_tw;
    double d_cw_offset;
    double d_level_db;
    double d_alpha;
    bool d_agc_on;
    bool d_agc_use_hang;
    int d_agc_threshold;
    int d_agc_slope;
    int d_agc_decay_ms;
    int d_agc_gain;
    float d_fm_maxdev;
    double d_fm_deemph;
    bool d_am_dcr;
    bool d_amsync_dcr;
    float d_amsync_pll_bw;
    bool d_nb_on[RECEIVER_NB_COUNT];
    float d_nb_threshold[RECEIVER_NB_COUNT];
};

#endif // RECEIVER_BASE_H
