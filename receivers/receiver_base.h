/* -*- c++ -*- */
/*
 * Copyright 2012 Alexandru Csete OZ9AEC.
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

#include <gr_hier_block2.h>


class receiver_base_cf;

typedef boost::shared_ptr<receiver_base_cf> receiver_base_cf_sptr;


/*! \brief Base class for receivers that output audio.
 *  \ingroup RX
 *
 * This block provides a base class and common interface for receivers that
 * outpout audio (or other kind of float data).
 *
 */
class receiver_base_cf : public gr_hier_block2
{

public:
    /*! \brief Public contructor.
     *  \param src_name Descriptive name used in the contructor of gr_hier_block2
     */
    receiver_base_cf(std::string src_name);
    ~receiver_base_cf();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void set_quad_rate(float quad_rate) = 0;
    virtual void set_audio_rate(float audio_rate) = 0;

    virtual void set_filter(double low, double high, double tw) = 0;

    virtual float get_signal_level(bool dbfs) = 0;

    /* Noise blanker */
    virtual bool has_nb() = 0;
    virtual void set_nb_on(int nbid, bool on) = 0;
    virtual void set_nb_threshold(int nbid, float threshold) = 0;

    /* Squelch parameter */
    virtual bool has_sql() = 0;
    virtual void set_sql_level(double level_db) = 0;
    virtual void set_sql_alpha(double alpha) = 0;

    /* AGC */
    virtual bool has_agc() = 0;
    virtual void set_agc_on(bool agc_on) = 0;
    virtual void set_agc_hang(bool use_hang) = 0;
    virtual void set_agc_threshold(int threshold) = 0;
    virtual void set_agc_slope(int slope) = 0;
    virtual void set_agc_decay(int decay_ms) = 0;
    virtual void set_agc_manual_gain(int gain) = 0;

    virtual void set_demod(int demod) = 0;

    /* demod specific methods are not part of interface */
};

#endif // RECEIVER_BASE_H
