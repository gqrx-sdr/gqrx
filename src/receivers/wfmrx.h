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
    /*! \brief Available demodulators. */
    enum wfmrx_demod {
        WFMRX_DEMOD_MONO       = 0,  /*!< Mono. */
        WFMRX_DEMOD_STEREO     = 1,  /*!< FM stereo. */
        WFMRX_DEMOD_STEREO_UKW = 2,  /*!< UKW stereo. */
        WFMRX_DEMOD_NUM        = 3   /*!< Included for convenience. */
    };
    wfmrx(float quad_rate, float audio_rate);
    ~wfmrx();

    bool start();
    bool stop();


    void set_filter(double low, double high, double tw);
    void set_cw_offset(double offset) { (void)offset; }

    /* Noise blanker */
    bool has_nb() { return false; }
    //void set_nb_on(int nbid, bool on);
    //void set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    bool has_sql() { return true; }

    /* AGC */
    bool has_agc() { return true; }

    void set_demod(int demod);

    /* FM parameters */
    bool has_fm() {return true; }
    void set_fm_maxdev(float maxdev_hz);
    void set_fm_deemph(double tau);

    void get_rds_data(std::string &outbuff, int &num);
    void start_rds_decoder();
    void stop_rds_decoder();
    void reset_rds_parser();
    bool is_rds_decoder_active();

private:
    bool   d_running;          /*!< Whether receiver is running or not. */

    wfmrx_demod               d_demod;   /*!< Current demodulator. */

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
