/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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


static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name)
    : gr::hier_block2 (src_name,
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(float)))
{

}

receiver_base_cf::~receiver_base_cf()
{

}

bool receiver_base_cf::has_nb()
{
    return false;
}

void receiver_base_cf::set_nb_on(int nbid, bool on)
{
    (void) nbid;
    (void) on;
}

void receiver_base_cf::set_nb_threshold(int nbid, float threshold)
{
    (void) nbid;
    (void) threshold;
}

bool receiver_base_cf::has_sql()
{
    return false;
}

void receiver_base_cf::set_sql_level(double level_db)
{
    (void) level_db;
}

void receiver_base_cf::set_sql_alpha(double alpha)
{
    (void) alpha;
}

bool receiver_base_cf::has_agc()
{
    return false;
}

void receiver_base_cf::set_agc_on(bool agc_on)
{
    (void) agc_on;
}

void receiver_base_cf::set_agc_hang(bool use_hang)
{
    (void) use_hang;
}

void receiver_base_cf::set_agc_threshold(int threshold)
{
    (void) threshold;
}

void receiver_base_cf::set_agc_slope(int slope)
{
    (void) slope;
}

void receiver_base_cf::set_agc_decay(int decay_ms)
{
    (void) decay_ms;
}

void receiver_base_cf::set_agc_manual_gain(int gain)
{
    (void) gain;
}

bool receiver_base_cf::has_fm()
{
    return false;
}

void receiver_base_cf::set_fm_maxdev(float maxdev_hz)
{
    (void) maxdev_hz;
}

void receiver_base_cf::set_fm_deemph(double tau)
{
    (void) tau;
}

bool receiver_base_cf::has_am()
{
    return false;
}

void receiver_base_cf::set_am_dcr(bool enabled)
{
    (void) enabled;
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
