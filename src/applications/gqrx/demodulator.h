/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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
#ifndef DEMODULATOR_H
#define DEMODULATOR_H

#include <memory>
#include <string>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_const_ff.h>
#else
#include <gnuradio/blocks/multiply_const.h>
#endif
#include <gnuradio/top_block.h>
//#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/wavfile_sink.h>
//#include <gnuradio/blocks/wavfile_source.h>

#include <QObject>
#include <QDebug>

#include "dsp/downconverter.h"
#include "dsp/sniffer_f.h"
#include "dsp/resampler_xx.h"
#include "dsp/rx_fft.h"
#include "interfaces/udp_sink_f.h"
#include "receivers/receiver_base.h"

#ifdef WITH_PULSEAUDIO
#include "pulseaudio/pa_sink.h"
#elif WITH_PORTAUDIO
#include "portaudio/portaudio_sink.h"
#else
#include <gnuradio/audio/sink.h>
#endif

#include "applications/gqrx/receiver_types.h"

class demodulator : public QObject
{
Q_OBJECT

public:
    typedef std::shared_ptr<demodulator> sptr;

    demodulator(
            gr::top_block_sptr tb,
            gr::basic_block_sptr src,
            const std::string audio_device,
            const int idx,
            int d_ddc_decim,
            int d_decim_rate,
            int d_quad_rate,
            int audio_rate
    );
    ~demodulator();

public:
    /* General */
    size_t      get_idx() const { return idx; }
    void        set_idx(size_t next_idx) {
        qInfo() << "demodulator" << idx << "gets new idx" << next_idx;
        idx = next_idx;
        emit indexChanged(idx);
    }

    /* I/O control */
    void        set_output_device(const std::string device, const int audio_rate);
    void        set_input_rate(const int d_ddc_decim, const int d_decim_rate, const int d_quad_rate);
    bool        supports_stream_naming() const { return d_supports_stream_naming; }

    int         get_audio_rate() const { return d_audio_rate; }

    /* Demodulation type */
    rx_status   set_demod(rx_demod demod, bool force, gr::basic_block_sptr src, int d_quad_rate, int d_audio_rate);
    rx_demod    get_demod() const {
        return d_demod;
    };

    /* Frequency control */
    rx_status       set_filter(double low, double high, rx_filter_shape shape);
    double          get_filter_lowcut() const;
    double          get_filter_highcut() const;
    rx_filter_shape get_filter_shape() const { return m_shape; }

    rx_status   set_filter_offset(double offset_hz);
    double      get_filter_offset(void) const;

    rx_status   set_cw_offset(double offset_hz);
    double      get_cw_offset(void) const;

    /* Noise blanker */
    rx_status   set_nb_on(int nbid, bool on);
    rx_status   set_nb_threshold(int nbid, float threshold);

    /* Squelch parameter */
    rx_status   set_sql_level(double level_db);
    rx_status   set_sql_alpha(double alpha);

    /* AGC */
    rx_status   set_agc_on(bool agc_on);
    rx_status   set_agc_hang(bool use_hang);
    rx_status   set_agc_threshold(int threshold);
    rx_status   set_agc_slope(int slope);
    rx_status   set_agc_decay(int decay_ms);
    rx_status   set_agc_manual_gain(int gain);

    /* FM parameters */
    rx_status   set_fm_maxdev(float maxdev_hz);
    rx_status   set_fm_deemph(double tau);

    /* AM parameters */
    rx_status   set_am_dcr(bool enabled);

    /* AM-Sync parameters */
    rx_status   set_amsync_dcr(bool enabled);
    rx_status   set_amsync_pll_bw(float pll_bw);

    /* Audio parameters */
    rx_status   set_af_gain(float gain_db);

    /* Data inspection */

    /**
     * @brief Get current signal power.
     * @param dbfs Whether to use dbfs or absolute power.
     * @return The current signal power.
     *
     * This method returns the current signal power detected by the receiver. The detector
     * is located after the band pass filter. The full scale is 1.0
     */
    float get_signal_pwr(bool dbfs) const {
        return rx->get_signal_level(dbfs);
    }

    void get_audio_fft_data(std::complex<float>* fftPoints, unsigned int &fftsize) const {
        audio_fft->get_fft_data(fftPoints, fftsize);
    }

    /* Audio Record/Playback */
    rx_status       start_audio_recording(const std::string filename);
    rx_status       stop_audio_recording();
//    bool                is_recording_audio(void) const { return d_recording_wav; }

//    rx_status      start_audio_playback(const std::string filename);
//    rx_status      stop_audio_playback();

    /* UDP Audio streaming */
    rx_status       start_udp_streaming(const std::string host, int port, bool stereo);
    rx_status       stop_udp_streaming();

    /* sample sniffer */
    rx_status       start_sniffer(unsigned int samplrate, int buffsize);
    rx_status       stop_sniffer();
    void            get_sniffer_data(float * outbuff, unsigned int &num);
    bool            is_snifffer_active(void) const { return d_sniffer_active; }

    /* rds functions */
    void        get_rds_data(std::string &outbuff, int &num);
    void        start_rds_decoder(void);
    void        stop_rds_decoder();
    bool        is_rds_decoder_active(void) const;
    void        reset_rds_parser(void);

    /* Plumbing */
    void connect_all(rx_chain type, gr::basic_block_sptr src, int d_quad_rate, int d_audio_rate);

signals:
    void indexChanged(size_t idx);

private:
    gr::top_block_sptr tb;                 /*!< Top Block */
    int                idx;                /*!< This sub-receiver index */

    double             d_filter_offset;    /*!< Current filter offset */
    rx_filter_shape    m_shape;
    double             d_cw_offset;        /*!< CW offset */
    bool               d_sniffer_active;   /*!< Only one data decoder allowed... ? */

    rx_demod           d_demod;            /*!< Current demodulator. */

    downconverter_cc_sptr               ddc;                /*!< Digital down-converter for demod chain. */

    gr::blocks::multiply_const_ff::sptr audio_gain0;        /*!< Audio gain block. */
    gr::blocks::multiply_const_ff::sptr audio_gain1;        /*!< Audio gain block. */

    // Audio recording
    bool                                d_recording_wav;    /*!< Whether we are recording WAV file. */
    int                                 d_audio_rate;
    gr::blocks::wavfile_sink::sptr      wav_sink;           /*!< WAV file sink for recording. */

    // Audio playback
//    gr::blocks::wavfile_source::sptr    wav_src;          /*!< WAV file source for playback. */
//    gr::blocks::null_sink::sptr         audio_null_sink0; /*!< Audio null sink used during playback. */
//    gr::blocks::null_sink::sptr         audio_null_sink1; /*!< Audio null sink used during playback. */

    // UDP streaming
    udp_sink_f_sptr             audio_udp_sink; /*!< UDP sink to stream audio over the network. */

    // Sample sniffer
    sniffer_f_sptr              sniffer;    /*!< Sample sniffer for data decoders. */
    resampler_ff_sptr           sniffer_rr; /*!< Sniffer resampler. */

    receiver_base_cf_sptr       rx;         /*!< receive hierblock. */
    rx_fft_f_sptr               audio_fft;  /*!< Audio FFT block. */

    bool                        audio_snk_connected;
    bool                        d_supports_stream_naming;
#ifdef WITH_PULSEAUDIO
    pa_sink_sptr                audio_snk;  /*!< Pulse audio sink. */
#elif WITH_PORTAUDIO
    portaudio_sink_sptr         audio_snk;  /*!< portaudio sink */
#else
    gr::audio::sink::sptr       audio_snk;  /*!< gr audio sink */
#endif
};

#endif // DEMODULATOR_H
