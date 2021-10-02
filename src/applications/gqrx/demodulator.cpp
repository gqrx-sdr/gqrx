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

#include "receivers/nbrx.h"
#include "receivers/wfmrx.h"

#include "applications/gqrx/demodulator.h"

#define DEFAULT_AUDIO_GAIN -6.0

demodulator::demodulator(
        gr::top_block_sptr tb,
        gr::basic_block_sptr src,
        const std::string audio_device,
        const int idx,
        int d_ddc_decim,
        int d_decim_rate,
        int d_quad_rate,
        int audio_rate
    ) :
      tb(tb),
      idx(idx),
      d_filter_offset(0.0),
      d_cw_offset(0.0),
      d_sniffer_active(false),
      d_demod(RX_DEMOD_NFM),
      d_recording_wav(false),
      d_audio_rate(audio_rate)
{

    ddc = make_downconverter_cc(d_ddc_decim, 0.0, d_decim_rate);
    rx  = make_nbrx(d_quad_rate, d_audio_rate);

    audio_gain0 = gr::blocks::multiply_const_ff::make(0);
    audio_gain1 = gr::blocks::multiply_const_ff::make(0);
    set_af_gain(DEFAULT_AUDIO_GAIN);

    audio_fft = make_rx_fft_f(8192u, d_audio_rate, gr::fft::window::WIN_HANN);

    audio_udp_sink = make_udp_sink_f();

    /* wav sink and source is created when rec/play is started */
//    audio_null_sink0 = gr::blocks::null_sink::make(sizeof(float));
//    audio_null_sink1 = gr::blocks::null_sink::make(sizeof(float));

//    sniffer = make_sniffer_f();
    /* sniffer_rr is created at each activation. */

    // XXX somewhat duplicated in set_output_device
    // XXX needs to update when set_idx called
    QString portName = QString("Receiver %0").arg(idx);
#ifdef WITH_PULSEAUDIO
    audio_snk = make_pa_sink(audio_device, d_audio_rate, "GQRX", portName.toStdString());
#elif WITH_PORTAUDIO
    audio_snk = make_portaudio_sink(audio_device, d_audio_rate, "GQRX", portName.toStdString());
#else
    audio_snk = gr::audio::sink::make(d_audio_rate, audio_device, true);
#endif
}

demodulator::~demodulator()
{
    // qInfo() << "demodulator::~demodulator called";
}

void demodulator::set_output_device(const std::string device, const int audio_rate)
{
    d_audio_rate = audio_rate;

    // qInfo() << "demodulator sets output device" << device.c_str() << "at rate" << d_audio_rate;
    QString portName = QString("Receiver %0").arg(idx);

#ifdef WITH_PULSEAUDIO
    // pulse sink can be reconfigured without affecting the tb flowgraph
    audio_snk->select_device(device, d_audio_rate, portName.toStdString());
#else // not pulse

    if (audio_snk_connected)
    {
        tb->disconnect(audio_gain0, 0, audio_snk, 0);
        tb->disconnect(audio_gain1, 0, audio_snk, 1);
        audio_snk_connected = false;
    }
    audio_snk.reset();

#if WITH_PORTAUDIO
    audio_snk = make_portaudio_sink(device, d_audio_rate, "GQRX", portName.toStdString());
#else
    audio_snk = gr::audio::sink::make(d_audio_rate, device, true);
#endif

    if (d_demod != RX_DEMOD_OFF && !audio_snk_connected)
    {
        tb->connect(audio_gain0, 0, audio_snk, 0);
        tb->connect(audio_gain1, 0, audio_snk, 1);
        audio_snk_connected = true;
    }

#endif // not pulse
}

void demodulator::set_input_rate(const int d_ddc_decim, const int d_decim_rate, const int d_quad_rate)
{
    ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
    rx->set_quad_rate(d_quad_rate);
}

/**
 * @brief Set filter offset.
 * @param offset_hz The desired filter offset in Hz.
 * @return RX_STATUS_ERROR if the tuning offset is out of range.
 *
 * This method sets a new tuning offset for the receiver. The tuning offset is used
 * to tune within the passband, i.e. select a specific channel within the received
 * spectrum.
 *
 * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
 * logical limit.
 *
 * @sa get_filter_offset()
 */
rx_status demodulator::set_filter_offset(double offset_hz)
{
    d_filter_offset = offset_hz;
    ddc->set_center_freq(d_filter_offset - d_cw_offset);

    return STATUS_OK;
}

/**
 * @brief Get filter offset.
 * @return The current filter offset.
 * @sa set_filter_offset()
 */
double demodulator::get_filter_offset(void) const
{
    return d_filter_offset;
}

/* CW offset can serve as a "BFO" if the GUI needs it */
rx_status demodulator::set_cw_offset(double offset_hz)
{
    d_cw_offset = offset_hz;
    ddc->set_center_freq(d_filter_offset - d_cw_offset);
    rx->set_cw_offset(d_cw_offset);

    return STATUS_OK;
}

double demodulator::get_cw_offset(void) const
{
    return d_cw_offset;
}

rx_status demodulator::set_filter(double low, double high, rx_filter_shape shape)
{
    double trans_width;

    if ((low >= high) || (std::abs(high-low) < RX_FILTER_MIN_WIDTH))
        return STATUS_ERROR;

    switch (shape) {

    case FILTER_SHAPE_SOFT:
        trans_width = std::abs(high - low) * 0.5;
        break;

    case FILTER_SHAPE_SHARP:
        trans_width = std::abs(high - low) * 0.1;
        break;

    case FILTER_SHAPE_NORMAL:
    default:
        trans_width = std::abs(high - low) * 0.2;
        break;

    }

    rx->set_filter(low, high, trans_width);

    return STATUS_OK;
}

rx_status demodulator::set_nb_on(int nbid, bool on)
{
    if (rx->has_nb())
        rx->set_nb_on(nbid, on);

    return STATUS_OK; // FIXME
}

rx_status demodulator::set_nb_threshold(int nbid, float threshold)
{
    if (rx->has_nb())
        rx->set_nb_threshold(nbid, threshold);

    return STATUS_OK; // FIXME
}

/**
 * @brief Set squelch level.
 * @param level_db The new level in dBFS.
 */
rx_status demodulator::set_sql_level(double level_db)
{
    if (rx->has_sql())
        rx->set_sql_level(level_db);

    return STATUS_OK; // FIXME
}

/** Set squelch alpha */
rx_status demodulator::set_sql_alpha(double alpha)
{
    if (rx->has_sql())
        rx->set_sql_alpha(alpha);

    return STATUS_OK; // FIXME
}

/**
 * @brief Enable/disable receiver AGC.
 *
 * When AGC is disabled a fixed manual gain is used, see set_agc_manual_gain().
 */
rx_status demodulator::set_agc_on(bool agc_on)
{
    if (rx->has_agc())
        rx->set_agc_on(agc_on);

    return STATUS_OK; // FIXME
}

/** Enable/disable AGC hang. */
rx_status demodulator::set_agc_hang(bool use_hang)
{
    if (rx->has_agc())
        rx->set_agc_hang(use_hang);

    return STATUS_OK; // FIXME
}

/** Set AGC threshold. */
rx_status demodulator::set_agc_threshold(int threshold)
{
    if (rx->has_agc())
        rx->set_agc_threshold(threshold);

    return STATUS_OK; // FIXME
}

/** Set AGC slope. */
rx_status demodulator::set_agc_slope(int slope)
{
    if (rx->has_agc())
        rx->set_agc_slope(slope);

    return STATUS_OK; // FIXME
}

/** Set AGC decay time. */
rx_status demodulator::set_agc_decay(int decay_ms)
{
    if (rx->has_agc())
        rx->set_agc_decay(decay_ms);

    return STATUS_OK; // FIXME
}

/** Set fixed gain used when AGC is OFF. */
rx_status demodulator::set_agc_manual_gain(int gain)
{
    if (rx->has_agc())
        rx->set_agc_manual_gain(gain);

    return STATUS_OK; // FIXME
}

rx_status demodulator::set_demod(rx_demod demod, bool force, gr::basic_block_sptr src, int d_quad_rate, int d_audio_rate)
{
    // qInfo() << "demodulator" << idx << "set_demod starts";

    rx_status ret = STATUS_OK;

    if (!force && (demod == d_demod)) {
        // qInfo() << "demodulator" << idx << "set_demod return early";
        return ret;
    }

    switch (demod)
    {
    case RX_DEMOD_OFF:
        connect_all(RX_CHAIN_NONE, src, d_quad_rate, d_audio_rate);
        break;

    case RX_DEMOD_NONE:
        connect_all(RX_CHAIN_NBRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(nbrx::NBRX_DEMOD_NONE);
        break;

    case RX_DEMOD_AM:
        connect_all(RX_CHAIN_NBRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(nbrx::NBRX_DEMOD_AM);
        break;

    case RX_DEMOD_AMSYNC:
        connect_all(RX_CHAIN_NBRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(nbrx::NBRX_DEMOD_AMSYNC);
        break;

    case RX_DEMOD_NFM:
        connect_all(RX_CHAIN_NBRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(nbrx::NBRX_DEMOD_FM);
        break;

    case RX_DEMOD_WFM_M:
        connect_all(RX_CHAIN_WFMRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(wfmrx::WFMRX_DEMOD_MONO);
        break;

    case RX_DEMOD_WFM_S:
        connect_all(RX_CHAIN_WFMRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(wfmrx::WFMRX_DEMOD_STEREO);
        break;

    case RX_DEMOD_WFM_S_OIRT:
        connect_all(RX_CHAIN_WFMRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(wfmrx::WFMRX_DEMOD_STEREO_UKW);
        break;

    case RX_DEMOD_SSB:
        connect_all(RX_CHAIN_NBRX, src, d_quad_rate, d_audio_rate);
        rx->set_demod(nbrx::NBRX_DEMOD_SSB);
        break;

    default:
        ret = STATUS_ERROR;
        break;
    }

    d_demod = demod;

    // qInfo() << "demodulator" << idx << "set_demod done";

    return ret;
}

/**
 * @brief Set maximum deviation of the FM demodulator.
 * @param maxdev_hz The new maximum deviation in Hz.
 */
rx_status demodulator::set_fm_maxdev(float maxdev_hz)
{
    if (rx->has_fm())
        rx->set_fm_maxdev(maxdev_hz);

    return STATUS_OK;
}

rx_status demodulator::set_fm_deemph(double tau)
{
    if (rx->has_fm())
        rx->set_fm_deemph(tau);

    return STATUS_OK;
}

rx_status demodulator::set_am_dcr(bool enabled)
{
    if (rx->has_am())
        rx->set_am_dcr(enabled);

    return STATUS_OK;
}

rx_status demodulator::set_amsync_dcr(bool enabled)
{
    if (rx->has_amsync())
        rx->set_amsync_dcr(enabled);

    return STATUS_OK;
}

rx_status demodulator::set_amsync_pll_bw(float pll_bw)
{
    if (rx->has_amsync())
        rx->set_amsync_pll_bw(pll_bw);

    return STATUS_OK;
}

rx_status demodulator::set_af_gain(float gain_db)
{
    float k;

    /* convert dB to factor */
    k = pow(10.0, gain_db / 20.0);
    //std::cout << "G:" << gain_db << "dB / K:" << k << std::endl;
    audio_gain0->set_k(k);
    audio_gain1->set_k(k);

    return STATUS_OK;
}

/**
 * @brief Start WAV file recorder.
 * @param filename The filename where to record.
 *
 * A new recorder object is created every time we start recording and deleted every time
 * we stop recording. The idea of creating one object and starting/stopping using different
 * file names does not work with WAV files (the initial /tmp/gqrx.wav will not be stopped
 * because the wav file can not be empty). See https://github.com/gqrx-sdr/gqrx/issues/36
 */
rx_status demodulator::start_audio_recording(const std::string filename)
{
    if (d_recording_wav)
    {
        /* error - we are already recording */
        std::cout << "ERROR: Can not start audio recorder (already recording)" << std::endl;

        return STATUS_ERROR;
    }

    // if this fails, we don't want to go and crash now, do we
    try {
#if GNURADIO_VERSION < 0x030900
        wav_sink = gr::blocks::wavfile_sink::make(filename.c_str(), 2,
                                                  (unsigned int) d_audio_rate,
                                                  16);
#else
        wav_sink = gr::blocks::wavfile_sink::make(filename.c_str(), 2,
                                                  (unsigned int) d_audio_rate,
                                                  gr::blocks::FORMAT_WAV, gr::blocks::FORMAT_PCM_16);
#endif
    }
    catch (std::runtime_error &e) {
        std::cout << "Error opening " << filename << ": " << e.what() << std::endl;
        return STATUS_ERROR;
    }

    tb->lock();
    tb->connect(rx, 0, wav_sink, 0);
    tb->connect(rx, 1, wav_sink, 1);
    tb->unlock();
    d_recording_wav = true;

    std::cout << "Recording audio to " << filename << std::endl;

    return STATUS_OK;
}

/** Stop WAV file recorder. */
rx_status demodulator::stop_audio_recording()
{
    if (!d_recording_wav) {
        /* error: we are not recording */
        std::cout << "ERROR: Can not stop audio recorder (not recording)" << std::endl;

        return STATUS_ERROR;
    }

    // not strictly necessary to lock but I think it is safer
    tb->lock();
    wav_sink->close();
    tb->disconnect(rx, 0, wav_sink, 0);
    tb->disconnect(rx, 1, wav_sink, 1);
    tb->unlock();
    wav_sink.reset();
    d_recording_wav = false;

    std::cout << "Audio recorder stopped" << std::endl;

    return STATUS_OK;
}

/** Start UDP streaming of audio. */
rx_status demodulator::start_udp_streaming(const std::string host, int port, bool stereo)
{
    tb->lock();
    qInfo() << "demodulator::start_udp_streaming";
    audio_udp_sink->start_streaming(host, port, stereo);
    tb->unlock();

    // qInfo() << "\n\n" << gr::dot_graph(tb).c_str() << "\n\n";

    return STATUS_OK;
}

/** Stop UDP streaming of audio. */
rx_status demodulator::stop_udp_streaming()
{
    tb->lock();
    audio_udp_sink->stop_streaming();
    tb->unlock();

    // qInfo() << "\n\n" << gr::dot_graph(tb).c_str() << "\n\n";

    return STATUS_OK;
}

void demodulator::get_rds_data(std::string &outbuff, int &num)
{
    rx->get_rds_data(outbuff, num);
}

void demodulator::start_rds_decoder(void)
{
    rx->start_rds_decoder();
}

void demodulator::stop_rds_decoder(void)
{
    rx->stop_rds_decoder();
}

bool demodulator::is_rds_decoder_active(void) const
{
    return rx->is_rds_decoder_active();
}

void demodulator::reset_rds_parser(void)
{
    rx->reset_rds_parser();
}

void demodulator::connect_all(rx_chain type, gr::basic_block_sptr src, int d_quad_rate, int d_audio_rate)
{
    // qInfo() << "demodulator" << idx << "connect_all starts";

    // RX demod chain
    switch (type)
    {
    case RX_CHAIN_NBRX:
        if (rx->name() != "NBRX")
        {
            rx.reset();
            rx = make_nbrx(d_quad_rate, d_audio_rate);
        }
        break;

    case RX_CHAIN_WFMRX:
        if (rx->name() != "WFMRX")
        {
            rx.reset();
            rx = make_wfmrx(d_quad_rate, d_audio_rate);
        }
        break;

    default:
        break;
    }

    // qInfo() << "demodulator" << idx << "connect_all created rx";

    // Audio path (if there is a receiver)
    if (type != RX_CHAIN_NONE)
    {
        tb->connect(src, 0, ddc, 0);
        tb->connect(ddc, 0, rx, 0);
        tb->connect(rx, 0, audio_fft, 0);
        tb->connect(rx, 0, audio_udp_sink, 0);
        tb->connect(rx, 1, audio_udp_sink, 1);
        tb->connect(rx, 0, audio_gain0, 0);
        tb->connect(rx, 1, audio_gain1, 0);
        tb->connect(audio_gain0, 0, audio_snk, 0);
        tb->connect(audio_gain1, 0, audio_snk, 1);

        // qInfo() << "demodulator" << idx << "connect_all created connections";
    }

    // Audio recording
    if (d_recording_wav)
    {
        tb->connect(rx, 0, wav_sink, 0);
        tb->connect(rx, 1, wav_sink, 1);
    }

    // Sample sniffer
//    if (d_sniffer_active)
//    {
//        tb->connect(rx, 0, sniffer_rr, 0);
//        tb->connect(sniffer_rr, 0, sniffer, 0);
//    }
    // qInfo() << "demodulator" << idx << "connect_all done";
}




///** Start audio playback. */
//rx_status receiver::start_audio_playback(const std::string filename)
//{
//    if (!d_running)
//    {
//        /* receiver is not running */
//        std::cout << "Can not start audio playback (receiver not running)" << std::endl;

//        return STATUS_ERROR;
//    }

//    try {
//        // output ports set automatically from file
//        wav_src = gr::blocks::wavfile_source::make(filename.c_str(), false);
//    }
//    catch (std::runtime_error &e) {
//        std::cout << "Error loading " << filename << ": " << e.what() << std::endl;
//        return STATUS_ERROR;
//    }

//    /** FIXME: We can only handle native rate (should maybe use the audio_rr)? */
//    unsigned int audio_rate = (unsigned int) d_audio_rate;
//    if (wav_src->sample_rate() != audio_rate)
//    {
//        std::cout << "BUG: Can not handle sample rate " << wav_src->sample_rate() << std::endl;
//        wav_src.reset();

//        return STATUS_ERROR;
//    }

//    /** FIXME: We can only handle stereo files */
//    if (wav_src->channels() != 2)
//    {
//        std::cout << "BUG: Can not handle other than 2 channels. File has " << wav_src->channels() << std::endl;
//        wav_src.reset();

//        return STATUS_ERROR;
//    }

//    stop();
//    /* route demodulator output to null sink */
//    tb->disconnect(rx, 0, audio_gain0, 0);
//    tb->disconnect(rx, 1, audio_gain1, 0);
//    tb->disconnect(rx, 0, audio_fft, 0);
//    tb->disconnect(rx, 0, audio_udp_sink, 0);
//    tb->disconnect(rx, 1, audio_udp_sink, 1);
//    tb->connect(rx, 0, audio_null_sink0, 0); /** FIXME: other channel? */
//    tb->connect(rx, 1, audio_null_sink1, 0); /** FIXME: other channel? */
//    tb->connect(wav_src, 0, audio_gain0, 0);
//    tb->connect(wav_src, 1, audio_gain1, 0);
//    tb->connect(wav_src, 0, audio_fft, 0);
//    tb->connect(wav_src, 0, audio_udp_sink, 0);
//    tb->connect(wav_src, 1, audio_udp_sink, 1);
//    start();

//    std::cout << "Playing audio from " << filename << std::endl;

//    return STATUS_OK;
//}

///** Stop audio playback. */
//rx_status receiver::stop_audio_playback()
//{
//    /* disconnect wav source and reconnect receiver */
//    stop();
//    tb->disconnect(wav_src, 0, audio_gain0, 0);
//    tb->disconnect(wav_src, 1, audio_gain1, 0);
//    tb->disconnect(wav_src, 0, audio_fft, 0);
//    tb->disconnect(wav_src, 0, audio_udp_sink, 0);
//    tb->disconnect(wav_src, 1, audio_udp_sink, 1);
//    tb->disconnect(rx, 0, audio_null_sink0, 0);
//    tb->disconnect(rx, 1, audio_null_sink1, 0);
//    tb->connect(rx, 0, audio_gain0, 0);
//    tb->connect(rx, 1, audio_gain1, 0);
//    tb->connect(rx, 0, audio_fft, 0);  /** FIXME: other channel? */
//    tb->connect(rx, 0, audio_udp_sink, 0);
//    tb->connect(rx, 1, audio_udp_sink, 1);
//    start();

//    /* delete wav_src since we can not change file name */
//    wav_src.reset();

//    return STATUS_OK;
//}



///**
// * @brief Start data sniffer.
// * @param buffsize The buffer that should be used in the sniffer.
// * @return STATUS_OK if the sniffer was started, STATUS_ERROR if the sniffer is already in use.
// */
//rx_status receiver::start_sniffer(unsigned int samprate, int buffsize)
//{
//    if (d_sniffer_active) {
//        /* sniffer already in use */
//        return STATUS_ERROR;
//    }

//    sniffer->set_buffer_size(buffsize);
//    sniffer_rr = make_resampler_ff((float)samprate/(float)d_audio_rate);
//    tb->lock();
//    tb->connect(rx, 0, sniffer_rr, 0);
//    tb->connect(sniffer_rr, 0, sniffer, 0);
//    tb->unlock();
//    d_sniffer_active = true;

//    return STATUS_OK;
//}

///**
// * @brief Stop data sniffer.
// * @return STATUS_ERROR i the sniffer is not currently active.
// */
//rx_status receiver::stop_sniffer()
//{
//    if (!d_sniffer_active) {
//        return STATUS_ERROR;
//    }

//    tb->lock();
//    tb->disconnect(rx, 0, sniffer_rr, 0);
//    tb->disconnect(sniffer_rr, 0, sniffer, 0);
//    tb->unlock();
//    d_sniffer_active = false;

//    /* delete resampler */
//    sniffer_rr.reset();

//    return STATUS_OK;
//}

///** Get sniffer data. */
//void receiver::get_sniffer_data(float * outbuff, unsigned int &num)
//{
//    sniffer->get_samples(outbuff, num);
//}
