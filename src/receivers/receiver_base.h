/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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


class receiver_base_cf;

typedef boost::shared_ptr<receiver_base_cf> receiver_base_cf_sptr;


/*! \brief Base class for receivers that output audio.
 *  \ingroup RX
 *
 * This block provides a base class and common interface for receivers that
 * outpout audio (or other kind of float data).
 *
 */
class receiver_base_cf : public gr::hier_block2
{

public:
    /*! \brief Public contructor.
     *  \param src_name Descriptive name used in the contructor of gr::hier_block2
     */
    receiver_base_cf(std::string src_name);
    virtual ~receiver_base_cf();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void set_quad_rate(float quad_rate) = 0;
    virtual void set_audio_rate(float audio_rate) = 0;

    virtual void set_filter(double low, double high, double tw) = 0;
    virtual void set_cw_offset(double offset) = 0;

    virtual float get_signal_level(bool dbfs) = 0;

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

    virtual void get_rds_data(std::string &outbuff, int &num);
    virtual void start_rds_decoder();
    virtual void stop_rds_decoder();
    virtual void reset_rds_parser();
    virtual bool is_rds_decoder_active();

};

#endif // RECEIVER_BASE_H
