/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2015 Alexandru Csete OZ9AEC.
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
#ifndef RX_RDS_H
#define RX_RDS_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/fir_filter_ccc.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_fcf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/digital/constellation_receiver_cb.h>
#include <gnuradio/blocks/keep_one_in_n.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/udp_sink.h>
#include <gnuradio/blocks/message_debug.h>
#include <boost/circular_buffer.hpp>
#include "dsp/rds/decoder.h"
#include "dsp/rds/parser.h"

class rx_rds;
class rx_rds_store;

typedef boost::shared_ptr<rx_rds> rx_rds_sptr;
typedef boost::shared_ptr<rx_rds_store> rx_rds_store_sptr;


rx_rds_sptr make_rx_rds(double sample_rate);

rx_rds_store_sptr make_rx_rds_store();

class rx_rds_store : public gr::block
{
public:
    rx_rds_store();
    ~rx_rds_store();

    void get_message(std::string &out, int &type);

private:
    void store(pmt::pmt_t msg);

    boost::mutex d_mutex;
    boost::circular_buffer<pmt::pmt_t> d_messages;

};

class rx_rds : public gr::hier_block2
{

public:
    rx_rds(double sample_rate=96000.0);
    ~rx_rds();

    void set_param(double low, double high, double trans_width);

private:
    std::vector<gr_complex> d_taps;
    std::vector<float> d_taps2;
    gr::filter::fir_filter_ccc::sptr  d_bpf;
    gr::filter::fir_filter_ccf::sptr  d_bpf2;
    gr::filter::freq_xlating_fir_filter_fcf::sptr f_fxff;
    gr::filter::freq_xlating_fir_filter_ccf::sptr f_fxff_ccf;
    std::vector<float> f_rrcf;
    gr::digital::constellation_receiver_cb::sptr d_mpsk;
    gr::blocks::keep_one_in_n::sptr b_koin;
    gr::digital::diff_decoder_bb::sptr d_ddbb;
    gr::rds::decoder::sptr rds_decoder;
    gr::rds::parser::sptr rds_parser;
    gr::blocks::udp_sink::sptr udp_sink1;
    gr::blocks::udp_sink::sptr udp_sink2;
    gr::blocks::udp_sink::sptr udp_sink3;
    gr::blocks::udp_sink::sptr udp_sink4;

    double d_sample_rate;
};


#endif // RX_RDS_H
