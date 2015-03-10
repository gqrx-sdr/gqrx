/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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

#include <gnuradio/analog/simple_squelch_cc.h>
#include "receivers/receiver_base.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/stereo_demod.h"
#include "dsp/resampler_xx.h"
#include "dsp/rx_rds.h"
#include "dsp/rds/decoder.h"
#include "dsp/rds/parser.h"

class wfmrx;

typedef boost::shared_ptr<wfmrx> wfmrx_sptr;

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

    void set_quad_rate(float quad_rate);
    void set_audio_rate(float audio_rate);

    void set_filter(double low, double high, double tw);

    float get_signal_level(bool dbfs);

    /* Noise blanker */
    bool has_nb() { return false; }
    //void set_nb_on(int nbid, bool on);
    //void set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    bool has_sql() { return true; }
    void set_sql_level(double level_db);
    void set_sql_alpha(double alpha);

    /* AGC */
    bool has_agc() { return false; }
    /*void set_agc_on(bool agc_on);
    void set_agc_hang(bool use_hang);
    void set_agc_threshold(int threshold);
    void set_agc_slope(int slope);
    void set_agc_decay(int decay_ms);
    void set_agc_manual_gain(int gain);*/

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
    float  d_quad_rate;        /*!< Input sample rate. */
    int    d_audio_rate;       /*!< Audio output rate. */

    wfmrx_demod               d_demod;   /*!< Current demodulator. */

    resampler_cc_sptr         iq_resamp; /*!< Baseband resampler. */
    rx_filter_sptr            filter;    /*!< Non-translating bandpass filter.*/

    rx_meter_c_sptr           meter;     /*!< Signal strength. */
    gr::analog::simple_squelch_cc::sptr sql;       /*!< Squelch. */
    rx_demod_fm_sptr          demod_fm;  /*!< FM demodulator. */
    resampler_ff_sptr         midle_rr;  /*!< Resampler. */
    stereo_demod_sptr         stereo;    /*!< FM stereo demodulator. */
    stereo_demod_sptr         mono;      /*!< FM stereo demodulator OFF. */

    rx_rds_sptr               rds;       /*!< RDS decoder */
    rx_rds_store_sptr         rds_store; /*!< RDS decoded messages */
    gr::rds::decoder::sptr    rds_decoder;
    gr::rds::parser::sptr     rds_parser;
    bool                      rds_enabled;
};

#endif // WFMRX_H
