/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#include <gnuradio/io_signature.h>
#include "receivers/receiver_base.h"


static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name)
    : gr::hier_block2 (src_name,
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(float))),
      d_filter_low(-5000),
      d_filter_high(5000),
      d_filter_tw(100),
      d_cw_offset(1000),
      d_level_db(-150),
      d_alpha(0.001),
      d_agc_on(true),
      d_agc_use_hang(false),
      d_agc_threshold(-100),
      d_agc_slope(0),
      d_agc_decay_ms(500),
      d_agc_gain(0),
      d_fm_maxdev(2500),
      d_fm_deemph(100),
      d_am_dcr(true),
      d_amsync_dcr(true),
      d_amsync_pll_bw(0.01)
{
    for (int k = 0; k < RECEIVER_NB_COUNT; k++)
    {
        d_nb_on[k] = false;
        d_nb_threshold[k] = 2;
    }

}

receiver_base_cf::~receiver_base_cf()
{

}

void receiver_base_cf::set_filter(double low, double high, double tw)
{
    d_filter_low = low;
    d_filter_high = high;
    d_filter_tw = tw;
}

void receiver_base_cf::set_cw_offset(double offset)
{
    d_cw_offset = offset;
}

bool receiver_base_cf::has_nb()
{
    return false;
}

void receiver_base_cf::set_nb_on(int nbid, bool on)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_on[nbid - 1] = on;
}

void receiver_base_cf::set_nb_threshold(int nbid, float threshold)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_threshold[nbid -1] = threshold;
}

bool receiver_base_cf::has_sql()
{
    return false;
}

void receiver_base_cf::set_sql_level(double level_db)
{
    d_level_db = level_db;
}

void receiver_base_cf::set_sql_alpha(double alpha)
{
    d_alpha = alpha;
}

bool receiver_base_cf::has_agc()
{
    return false;
}

void receiver_base_cf::set_agc_on(bool agc_on)
{
    d_agc_on = agc_on;
}

void receiver_base_cf::set_agc_hang(bool use_hang)
{
    d_agc_use_hang = use_hang;
}

void receiver_base_cf::set_agc_threshold(int threshold)
{
    d_agc_threshold = threshold;
}

void receiver_base_cf::set_agc_slope(int slope)
{
    d_agc_slope = slope;
}

void receiver_base_cf::set_agc_decay(int decay_ms)
{
    d_agc_decay_ms = decay_ms;
}

void receiver_base_cf::set_agc_manual_gain(int gain)
{
    d_agc_gain = gain;
}

bool receiver_base_cf::has_fm()
{
    return false;
}

void receiver_base_cf::set_fm_maxdev(float maxdev_hz)
{
    d_fm_maxdev = maxdev_hz;
}

void receiver_base_cf::set_fm_deemph(double tau)
{
    d_fm_deemph = tau;
}

bool receiver_base_cf::has_am()
{
    return false;
}

void receiver_base_cf::set_am_dcr(bool enabled)
{
    d_am_dcr = enabled;
}

bool receiver_base_cf::has_amsync()
{
    return false;
}

void receiver_base_cf::set_amsync_dcr(bool enabled)
{
    d_amsync_dcr = enabled;
}

void receiver_base_cf::set_amsync_pll_bw(float pll_bw)
{
    d_amsync_pll_bw = pll_bw;
}

void receiver_base_cf::get_rds_data(std::string &outbuff, int &num)
{
        (void) outbuff;
        (void) num;
}

void receiver_base_cf::start_rds_decoder()
{
}

void receiver_base_cf::stop_rds_decoder()
{
}

void receiver_base_cf::reset_rds_parser()
{
}

bool receiver_base_cf::is_rds_decoder_active()
{
    return false;
}

void receiver_base_cf::restore_settings(receiver_base_cf_sptr from)
{
    set_filter(from->d_filter_low, from->d_filter_high, from->d_filter_tw);
    set_cw_offset(from->d_cw_offset);
    for (int k = 0; k < RECEIVER_NB_COUNT; k++)
    {
        set_nb_on(k + 1, from->d_nb_on[k]);
        set_nb_threshold(k + 1, from->d_nb_threshold[k]);
    }
    set_sql_level(from->d_level_db);
    set_sql_alpha(from->d_alpha);
    set_agc_on(from->d_agc_on);
    set_agc_hang(from->d_agc_use_hang);
    set_agc_threshold(from->d_agc_threshold);
    set_agc_slope(from->d_agc_slope);
    set_agc_decay(from->d_agc_decay_ms);
    set_agc_manual_gain(from->d_agc_gain);
    set_fm_maxdev(from->d_fm_maxdev);
    set_fm_deemph(from->d_fm_deemph);
    set_am_dcr(from->d_am_dcr);
    set_amsync_dcr(from->d_amsync_dcr);
    set_amsync_pll_bw(from->d_amsync_pll_bw);
}
