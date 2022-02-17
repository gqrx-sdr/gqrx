/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
 * FM stereo implementation by Alex Grinkov a.grinkov(at)gmail.com.
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
#ifndef WFMRX_H
#define WFMRX_H

#include "receivers/receiver_base.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/stereo_demod.h"
#include "dsp/rx_rds.h"
#include "dsp/rds/decoder.h"
#include "dsp/rds/parser.h"

class wfmrx;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<wfmrx> wfmrx_sptr;
#else
typedef std::shared_ptr<wfmrx> wfmrx_sptr;
#endif

/*! \brief Public constructor of wfm_rx. */
wfmrx_sptr make_wfmrx(float quad_rate, float audio_rate);

/*! \brief Wide band FM receiver.
 *  \ingroup RX
 *
 * This block provides receiver for broadcast FM transmissions.
 */
class wfmrx : public receiver_base_cf
{

public:
    wfmrx(float quad_rate, float audio_rate);
    ~wfmrx();

    bool start() override;
    bool stop() override;


    void set_filter(int low, int high, int tw) override;

    /* Noise blanker */
    bool has_nb() override { return false; }

    void set_demod(Modulations::idx demod) override;

    void get_rds_data(std::string &outbuff, int &num) override;
    void start_rds_decoder() override;
    void stop_rds_decoder() override;
    void reset_rds_parser() override;
    bool is_rds_decoder_active() override;

private:
    bool   d_running;          /*!< Whether receiver is running or not. */

    rx_filter_sptr            filter;    /*!< Non-translating bandpass filter.*/

    rx_demod_fm_sptr          demod_fm;  /*!< FM demodulator. */
    stereo_demod_sptr         stereo;    /*!< FM stereo demodulator. */
    stereo_demod_sptr         stereo_oirt;    /*!< FM stereo oirt demodulator. */
    stereo_demod_sptr         mono;      /*!< FM stereo demodulator OFF. */

    rx_rds_sptr               rds;       /*!< RDS decoder */
    rx_rds_store_sptr         rds_store; /*!< RDS decoded messages */
    gr::rds::decoder::sptr    rds_decoder;
    gr::rds::parser::sptr     rds_parser;
    bool                      rds_enabled;
};

#endif // WFMRX_H
