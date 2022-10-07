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
#include <gnuradio/blocks/wavfile_sink.h>
#include "dsp/resampler_xx.h"
#include "dsp/rx_meter.h"
#include "dsp/rx_agc_xx.h"
#include "dsp/rx_squelch.h"
#include "dsp/downconverter.h"
#include "interfaces/wav_sink.h"
#include "interfaces/udp_sink_f.h"
#include "receivers/vfo.h"
#include "defines.h"

class receiver_base_cf;

#if 0
/** Available demodulators. */
enum rx_demod {
    RX_DEMOD_OFF   = 0,  /*!< No receiver. */
    RX_DEMOD_NONE  = 1,  /*!< No demod. Raw I/Q to audio. */
    RX_DEMOD_AM    = 2,  /*!< Amplitude modulation. */
    RX_DEMOD_NFM   = 3,  /*!< Frequency modulation. */
    RX_DEMOD_WFM_M = 4,  /*!< Frequency modulation (wide, mono). */
    RX_DEMOD_WFM_S = 5,  /*!< Frequency modulation (wide, stereo). */
    RX_DEMOD_WFM_S_OIRT = 6,  /*!< Frequency modulation (wide, stereo oirt). */
    RX_DEMOD_SSB   = 7,  /*!< Single Side Band. */
    RX_DEMOD_AMSYNC = 8  /*!< Amplitude modulation (synchronous demod). */
};
#endif

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
class receiver_base_cf : public gr::hier_block2, public vfo_s
{

public:
    /*! \brief Public constructor.
     *  \param src_name Descriptive name used in the constructor of gr::hier_block2
     */
    typedef std::function<void(int, std::string, bool)> rec_event_handler_t;
    receiver_base_cf(std::string src_name, float pref_quad_rate, double quad_rate, int audio_rate);
    virtual ~receiver_base_cf();

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void set_quad_rate(double quad_rate);
    virtual void set_center_freq(double center_freq);
    void set_offset(int offset) override;

    /* Audio recording */
    void set_audio_rec_dir(const std::string& dir) override;
    void set_audio_rec_sql_triggered(bool enabled) override;
    void set_audio_rec_min_time(const int time_ms) override;
    void set_audio_rec_max_gap(const int time_ms) override;

    /* UDP  streaming */
    bool         set_udp_host(const std::string &host) override;
    bool         set_udp_port(int port) override;
    bool         set_udp_stereo(bool stereo) override;
    virtual bool set_udp_streaming(bool streaming);
    inline bool  get_udp_streaming() const { return d_udp_streaming; }

    virtual float get_signal_level();

    void set_demod(Modulations::idx demod) override;

    /* Noise blanker */
    virtual bool has_nb();

    /* Squelch parameter */
    void set_sql_level(double level_db) override;
    void set_sql_alpha(double alpha) override;

    /* AGC */
    void  set_agc_on(bool agc_on) override;
    void  set_agc_target_level(int target_level) override;
    void  set_agc_manual_gain(float gain) override;
    void  set_agc_max_gain(int gain) override;
    void  set_agc_attack(int attack_ms) override;
    void  set_agc_decay(int decay_ms) override;
    void  set_agc_hang(int hang_ms) override;
    void  set_agc_panning(int panning) override;
    void  set_agc_mute(bool agc_mute) override;
    virtual float get_agc_gain();

    /* CW parameters */
    void set_cw_offset(int offset) override;

    virtual void get_rds_data(std::string &outbuff, int &num);
    virtual void start_rds_decoder();
    virtual void stop_rds_decoder();
    virtual void reset_rds_parser();
    virtual bool is_rds_decoder_active();
    virtual int  start_audio_recording();
    virtual void stop_audio_recording();
    virtual void continue_audio_recording(receiver_base_cf_sptr from);
    virtual bool get_audio_recording();
    virtual std::string get_last_audio_filename();
    template <typename T> void set_rec_event_handler(T handler)
    {
        d_rec_event = handler;
    }
    using vfo_s::restore_settings;
    virtual void restore_settings(receiver_base_cf& from);
    bool connected() { return d_connected; }
    void connected(bool value) { d_connected = value; }

protected:
    bool         d_connected;
    double       d_decim_rate;   /*!< Quadrature rate (before down-conversion) */
    double       d_quad_rate;    /*!< Quadrature rate (after down-conversion) */
    unsigned int d_ddc_decim;    /*!< Down-conversion decimation. */
    int          d_audio_rate;   /*!< Audio output rate. */
    double       d_center_freq;
    std::string  d_audio_filename;
    bool         d_udp_streaming;

    downconverter_cc_sptr     ddc;        /*!< Digital down-converter for demod chain. */
    resampler_cc_sptr         iq_resamp;   /*!< Baseband resampler. */
    rx_meter_c_sptr           meter;      /*!< Signal strength. */
    rx_agc_2f_sptr            agc;        /*!< Receiver AGC. */
    rx_sql_cc_sptr            sql;        /*!< Squelch. */
    wavfile_sink_gqrx::sptr   wav_sink;   /*!< WAV file sink for recording. */
    udp_sink_f_sptr           audio_udp_sink;  /*!< UDP sink to stream audio over the network. */
private:
    float d_pref_quad_rate;
    rec_event_handler_t d_rec_event;
    static void rec_event(receiver_base_cf * self, std::string filename, bool is_running);

};

#endif // RECEIVER_BASE_H
