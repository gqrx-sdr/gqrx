/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#include <iostream>
#include <cmath>

#include <gr_top_block.h>
#include <gr_multiply_const_ff.h>

#include <osmosdr_source_c.h>
#include <osmosdr_ranges.h>

#include "applications/gqrx/receiver.h"
#include "dsp/correct_iq_cc.h"
#include "dsp/rx_fft.h"
#include "pulseaudio/pa_sink.h"
#include "receivers/nbrx.h"
#include "receivers/wfmrx.h"

/*! \brief Public contructor.
 *  \param input_device Input device specifier.
 *  \param audio_device Audio output device specifier,
 *                      e.g. hw:0 when using ALSA or Portaudio.
 *
 * \todo Option to use UHD device instead of FCD.
 */
receiver::receiver(const std::string input_device, const std::string audio_device)
    : d_running(false),
      d_input_rate(96000.0),
      d_audio_rate(48000),
      d_rf_freq(144800000.0),
      d_filter_offset(0.0),
      d_recording_wav(false),
      d_sniffer_active(false),
      d_demod(RX_DEMOD_OFF)
{
    tb = gr_make_top_block("gqrx");

    if (input_device.empty())
    {
        // FIXME: other OS
        src = osmosdr_make_source_c("file=/dev/random,freq=428e6,rate=96000,repeat=true,throttle=true");
    }
    else
    {
        src = osmosdr_make_source_c(input_device);
    }

    rx = make_nbrx(d_input_rate, d_audio_rate);
    lo = gr_make_sig_source_c(d_input_rate, GR_SIN_WAVE, 0.0, 1.0);
    mixer = gr_make_multiply_cc();

    dc_corr = make_dc_corr_cc(0.01f);
    iq_fft = make_rx_fft_c(4096, 0);

    audio_fft = make_rx_fft_f(3072);
    audio_gain0 = gr_make_multiply_const_ff(0.1);
    audio_gain1 = gr_make_multiply_const_ff(0.1);

    audio_snk = make_pa_sink(audio_device, d_audio_rate, "GQRX", "Audio output");

    /* wav sink and source is created when rec/play is started */
    audio_null_sink = gr_make_null_sink(sizeof(float));
    sniffer = make_sniffer_f();
    /* sniffer_rr is created at each activation. */

    set_demod(RX_DEMOD_NFM);
}


/*! \brief Public destructor. */
receiver::~receiver()
{
    tb->stop();

    /* FIXME: delete blocks? */
}


/*! \brief Start the receiver. */
void receiver::start()
{
    /* FIXME: Check that flow graph is not running */
    if (!d_running)
    {
        tb->start();
        d_running = true;
    }
}

/*! \brief Stop the receiver. */
void receiver::stop()
{
    if (d_running)
    {
        tb->stop();
        tb->wait(); // If the graph is needed to run again, wait() must be called after stop
        d_running = false;
    }
}

/*! \brief Select new input device.
 *
 * \bug When using ALSA, program will crash if the new device
 *      is the same as the previously used device:
 *      audio_alsa_source[hw:1]: Device or resource busy
 */
void receiver::set_input_device(const std::string device)
{
    if (device.empty())
        return;

    tb->lock();

    tb->disconnect(src, 0, dc_corr, 0);
    src.reset();
    src = osmosdr_make_source_c(device);
    tb->connect(src, 0, dc_corr, 0);

    tb->unlock();
}


/*! \brief Select new audio output device. */
void receiver::set_output_device(const std::string device)
{
    tb->lock();

    tb->disconnect(audio_gain0, 0, audio_snk, 0);
    tb->disconnect(audio_gain1, 0, audio_snk, 1);

    audio_snk.reset();
    audio_snk = make_pa_sink(device, d_audio_rate); // FIXME: does this keep app and stream name?

    tb->connect(audio_gain0, 0, audio_snk, 0);
    tb->connect(audio_gain1, 0, audio_snk, 1);

    tb->unlock();
}


/*! \brief Set new input sample rate.
 *  \param rate The desired input rate
 *  \return The actual sample rate set.
 */
double receiver::set_input_rate(double rate)
{
    tb->lock();
    d_input_rate = src->set_sample_rate(rate);
    rx->set_quad_rate(d_input_rate);
    lo->set_sampling_freq(d_input_rate);
    tb->unlock();

    return d_input_rate;
}


/*! \brief Get current input sample rate. */
double receiver::get_input_rate()
{
    return d_input_rate;
}


/*! \brief Set RF frequency.
 *  \param freq_hz The desired frequency in Hz.
 *  \return RX_STATUS_ERROR if an error occurs, e.g. the frequency is out of range.
 *  \sa get_rf_freq()
 */
receiver::status receiver::set_rf_freq(double freq_hz)
{
    d_rf_freq = freq_hz;

    src->set_center_freq(d_rf_freq);
    // FIXME: read back frequency?

    return STATUS_OK;
}

/*! \brief Get RF frequency.
 *  \return The current RF frequency.
 *  \sa set_rf_freq()
 */
double receiver::get_rf_freq()
{
    d_rf_freq = src->get_center_freq();

    return d_rf_freq;
}

/*! \brief Set RF gain.
 *  \param gain_rel The desired relative gain between 0.0 and 1.0 (use -1 for AGC where supported).
 *  \return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
 */
receiver::status receiver::set_rf_gain(double gain_rel)
{
    if (gain_rel > 1.0)
        gain_rel = 1.0;

    if (gain_rel < 0.0)
    {
        src->set_gain_mode(true);
    }
    else
    {
        if (src->get_gain_mode())
            // disable HW AGC
            src->set_gain_mode(false);

        // convert relative gain to absolute gain
        osmosdr::gain_range_t range = src->get_gain_range();
        if (!range.empty())
        {
            double gain =  range.start() + gain_rel*(range.stop()-range.start());
            src->set_gain(gain);

#ifndef QT_NO_DEBUG
        std::cout << "Gain start/stop/rel/abs:" << range.start() << "/"
                  << range.stop() << "/" << gain_rel << "/" << gain << std::endl;
#endif
        }
    }

    return STATUS_OK;
}


/*! \brief Set filter offset.
 *  \param offset_hz The desired filter offset in Hz.
 *  \return RX_STATUS_ERROR if the tuning offset is out of range.
 *
 * This method sets a new tuning offset for the receiver. The tuning offset is used
 * to tune within the passband, i.e. select a specific channel within the received
 * spectrum.
 *
 * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
 * logical limit.
 *
 * \sa get_filter_offset()
 */
receiver::status receiver::set_filter_offset(double offset_hz)
{
    d_filter_offset = offset_hz;
    lo->set_frequency(-d_filter_offset);

    return STATUS_OK;
}

/*! \brief Get filterm offset.
 *  \return The current filter offset.
 *  \sa set_filter_offset()
 */
double receiver::get_filter_offset()
{
    return d_filter_offset;
}


receiver::status receiver::set_filter(double low, double high, filter_shape shape)
{
    double trans_width;

    if ((low >= high) || (abs(high-low) < RX_FILTER_MIN_WIDTH))
        return STATUS_ERROR;

    switch (shape) {

    case FILTER_SHAPE_SOFT:
        trans_width = abs(high-low)*0.2;
        break;

    case FILTER_SHAPE_SHARP:
        trans_width = abs(high-low)*0.01;
        break;

    case FILTER_SHAPE_NORMAL:
    default:
        trans_width = abs(high-low)*0.1;
        break;

    }

    rx->set_filter(low, high, trans_width);

    return STATUS_OK;
}


receiver::status receiver::set_filter_low(double freq_hz)
{
    return STATUS_OK;
}


receiver::status receiver::set_filter_high(double freq_hz)
{
    return STATUS_OK;
}


receiver::status receiver::set_filter_shape(filter_shape shape)
{
    return STATUS_OK;
}


receiver::status receiver::set_freq_corr(int ppm)
{
    src->set_freq_corr(ppm);

    return STATUS_OK;
}


receiver::status receiver::set_dc_corr(double dci, double dcq)
{
    //src->set_dc_corr(dci, dcq);   FIXME!

    return STATUS_OK;
}

receiver::status receiver::set_iq_corr(double gain, double phase)
{
    //src->set_iq_corr(gain, phase);   FIXME!

    return STATUS_OK;
}


/*! \brief Get current signal power.
 *  \param dbfs Whether to use dbfs or absolute power.
 *  \return The current signal power.
 *
 * This method returns the current signal power detected by the receiver. The detector
 * is located after the band pass filter. The full scale is 1.0
 */
float receiver::get_signal_pwr(bool dbfs)
{
    return rx->get_signal_level(dbfs);
}

/*! \brief Get latest baseband FFT data. */
void receiver::get_iq_fft_data(std::complex<float>* fftPoints, int &fftsize)
{
    iq_fft->get_fft_data(fftPoints, fftsize);
}

/*! \brief Get latest audio FFT data. */
void receiver::get_audio_fft_data(std::complex<float>* fftPoints, int &fftsize)
{
    audio_fft->get_fft_data(fftPoints, fftsize);
}

receiver::status receiver::set_nb_on(int nbid, bool on)
{
    if (rx->has_nb())
        rx->set_nb_on(nbid, on);

    return STATUS_OK; // FIXME
}

receiver::status receiver::set_nb_threshold(int nbid, float threshold)
{
    if (rx->has_nb())
        rx->set_nb_threshold(nbid, threshold);

    return STATUS_OK; // FIXME
}


/*! \brief Set squelch level.
 *  \param level_db The new level in dBFS.
 */
receiver::status receiver::set_sql_level(double level_db)
{
    if (rx->has_sql())
        rx->set_sql_level(level_db);

    return STATUS_OK; // FIXME
}


/*! \brief Set squelch alpha */
receiver::status receiver::set_sql_alpha(double alpha)
{
    if (rx->has_sql())
        rx->set_sql_alpha(alpha);

    return STATUS_OK; // FIXME
}

/*! \brief Enable/disable receiver AGC.
 *
 * When AGC is disabled a fixed manual gain is used, see set_agc_manual_gain().
 */
receiver::status receiver::set_agc_on(bool agc_on)
{
    if (rx->has_agc())
        rx->set_agc_on(agc_on);

    return STATUS_OK; // FIXME
}

/*! \brief Enable/disable AGC hang. */
receiver::status receiver::set_agc_hang(bool use_hang)
{
    if (rx->has_agc())
        rx->set_agc_hang(use_hang);

    return STATUS_OK; // FIXME
}

/*! \brief Set AGC threshold. */
receiver::status receiver::set_agc_threshold(int threshold)
{
    if (rx->has_agc())
        rx->set_agc_threshold(threshold);

    return STATUS_OK; // FIXME
}

/*! \brief Set AGC slope. */
receiver::status receiver::set_agc_slope(int slope)
{
    if (rx->has_agc())
        rx->set_agc_slope(slope);

    return STATUS_OK; // FIXME
}

/*! \brief Set AGC decay time. */
receiver::status receiver::set_agc_decay(int decay_ms)
{
    if (rx->has_agc())
        rx->set_agc_decay(decay_ms);

    return STATUS_OK; // FIXME
}

/*! \brief Set fixed gain used when AGC is OFF. */
receiver::status receiver::set_agc_manual_gain(int gain)
{
    if (rx->has_agc())
        rx->set_agc_manual_gain(gain);

    return STATUS_OK; // FIXME
}

receiver::status receiver::set_demod(rx_demod demod)
{
    bool needs_restart = d_running;
    bool wide_fm = (d_demod == RX_DEMOD_WFM_M) || (d_demod == RX_DEMOD_WFM_S);
    status ret = STATUS_OK;

    if (demod == d_demod)
        return ret;

    if (d_running)
        stop();

    switch (demod)
    {
    case RX_DEMOD_OFF:
        tb->disconnect_all();
        connect_all(RX_CHAIN_NONE);
        break;

    case RX_DEMOD_NONE:
        if ((d_demod == RX_DEMOD_OFF) || wide_fm)
        {
            tb->disconnect_all();
            connect_all(RX_CHAIN_NBRX);
        }
        rx->set_demod(nbrx::NBRX_DEMOD_NONE);
        break;

    case RX_DEMOD_AM:
        if ((d_demod == RX_DEMOD_OFF) || wide_fm)
        {
            tb->disconnect_all();
            connect_all(RX_CHAIN_NBRX);
        }
        rx->set_demod(nbrx::NBRX_DEMOD_AM);
        break;

    case RX_DEMOD_NFM:
        if ((d_demod == RX_DEMOD_OFF) || wide_fm)
        {
            tb->disconnect_all();
            connect_all(RX_CHAIN_NBRX);
        }
        rx->set_demod(nbrx::NBRX_DEMOD_FM);
        break;

    case RX_DEMOD_WFM_M:
        if (!wide_fm)
        {
            tb->disconnect_all();
            connect_all(RX_CHAIN_WFMRX);
        }
        rx->set_demod(wfmrx::WFMRX_DEMOD_MONO);
        break;

    case RX_DEMOD_WFM_S:
        if (!wide_fm)
        {
            tb->disconnect_all();
            connect_all(RX_CHAIN_WFMRX);
        }
        rx->set_demod(wfmrx::WFMRX_DEMOD_STEREO);
        break;

    case RX_DEMOD_SSB:
        if ((d_demod == RX_DEMOD_OFF) || wide_fm)
        {
            tb->disconnect_all();
            connect_all(RX_CHAIN_NBRX);
        }
        rx->set_demod(nbrx::NBRX_DEMOD_SSB);
        break;

    default:
        ret = STATUS_ERROR;
        break;
    }

    d_demod = demod;

    if (needs_restart)
        start();

    return ret;
}

/*! \brief Set maximum deviation of the FM demodulator.
 *  \param maxdev_hz The new maximum deviation in Hz.
 */
receiver::status receiver::set_fm_maxdev(float maxdev_hz)
{
    if (rx->has_fm())
        rx->set_fm_maxdev(maxdev_hz);

    return STATUS_OK;
}

receiver::status receiver::set_fm_deemph(double tau)
{
    if (rx->has_fm())
        rx->set_fm_deemph(tau);

    return STATUS_OK;
}

receiver::status receiver::set_af_gain(float gain_db)
{
    float k;

    /* convert dB to factor */
    k = pow(10.0, gain_db / 20.0);
    //std::cout << "G:" << gain_db << "dB / K:" << k << std::endl;
    audio_gain0->set_k(k);
    audio_gain1->set_k(k);

    return STATUS_OK;
}


/*! \brief Start WAV file recorder.
 *  \param filename The filename where to record.
 *
 * A new recorder object is created every time we start recording and deleted every time
 * we stop recording. The idea of creating one object and starting/stopping using different
 * file names does not work with WAV files (the initial /tmp/gqrx.wav will not be stopped
 * because the wav file can not be empty). See https://github.com/csete/gqrx/issues/36
 */
receiver::status receiver::start_audio_recording(const std::string filename)
{
    if (d_recording_wav)
    {
        /* error - we are already recording */
        std::cout << "ERROR: Can not start audio recorder (already recording)" << std::endl;

        return STATUS_ERROR;
    }
    if (!d_running)
    {
        /* receiver is not running */
        std::cout << "Can not start audio recorder (receiver not running)" << std::endl;

        return STATUS_ERROR;
    }

    // not strictly necessary to lock but I think it is safer
    tb->lock();
    wav_sink = gr_make_wavfile_sink(filename.c_str(), 2, 48000, 16);
    tb->connect(audio_gain0, 0, wav_sink, 0);
    tb->connect(audio_gain1, 0, wav_sink, 1);
    tb->unlock();
    d_recording_wav = true;

    std::cout << "Recording audio to " << filename << std::endl;

    return STATUS_OK;
}


/*! \brief Stop WAV file recorder. */
receiver::status receiver::stop_audio_recording()
{
    if (!d_recording_wav) {
        /* error: we are not recording */
        std::cout << "ERROR: Can stop audio recorder (not recording)" << std::endl;

        return STATUS_ERROR;
    }
    if (!d_running)
    {
        /* receiver is not running */
        std::cout << "Can not start audio recorder (receiver not running)" << std::endl;

        return STATUS_ERROR;
    }

    // not strictly necessary to lock but I think it is safer
    tb->lock();
    wav_sink->close();
    tb->disconnect(audio_gain0, 0, wav_sink, 0);
    tb->disconnect(audio_gain1, 0, wav_sink, 1);
    wav_sink.reset();
    tb->unlock();
    d_recording_wav = false;

    std::cout << "Audio recorder stopped" << std::endl;

    return STATUS_OK;
}


/*! \brief Start audio playback. */
receiver::status receiver::start_audio_playback(const std::string filename)
{
    try {
        wav_src = gr_make_wavfile_source(filename.c_str(), false);
    }
    catch (std::runtime_error &e) {
        std::cout << "Error loading " << filename << ": " << e.what() << std::endl;
        return STATUS_ERROR;
    }

    /** FIXME: We can only handle 48k for now (should maybe use the audio_rr)? */
    if (wav_src->sample_rate() != 48000) {
        std::cout << "BUG: Can not handle sample rate " << wav_src->sample_rate() << std::cout;
        wav_src.reset();

        return STATUS_ERROR;
    }

    stop();
    /* route demodulator output to null sink */
    tb->disconnect(rx, 0, audio_gain0, 0);
    tb->disconnect(rx, 1, audio_gain1, 0);
    tb->disconnect(rx, 0, audio_fft, 0);
    tb->connect(rx, 0, audio_null_sink, 0);
    tb->connect(wav_src, 0, audio_gain0, 0);  // FIXME: 2 channels
    tb->connect(wav_src, 0, audio_fft, 0);
    start();

    return STATUS_OK;
}


/*! \brief Stop audio playback. */
receiver::status receiver::stop_audio_playback()
{
    /* disconnect wav source and reconnect receiver */
    stop();
    tb->disconnect(wav_src, 0, audio_gain0, 0);
    tb->disconnect(wav_src, 0, audio_fft, 0);
    tb->disconnect(rx, 0, audio_null_sink, 0);
    tb->connect(rx, 0, audio_gain0, 0);
    tb->connect(rx, 1, audio_gain1, 0);
    tb->connect(rx, 0, audio_fft, 0);
    start();

    /* delete wav_src since we can not change file name */
    wav_src.reset();

    return STATUS_OK;
}


/*! \brief Start I/Q data recorder.
 *  \param filename The filename where to record.
 */
receiver::status receiver::start_iq_recording(const std::string filename)
{
#if 0
    if (d_recording_iq) {
        /* error - we are already recording */
        return STATUS_ERROR;
    }

    /* iq_sink was created in the constructor */
    if (iq_sink) {
        /* not strictly necessary to lock but I think it is safer */
        tb->lock();
        iq_sink->open(filename.c_str());
        tb->unlock();
        d_recording_iq = true;
    }
    else {
        std::cout << "BUG: I/Q file sink does not exist" << std::endl;
    }
#endif
    return STATUS_OK;
}


/*! \brief Stop I/Q data recorder. */
receiver::status receiver::stop_iq_recording()
{
#if 0
    if (!d_recording_iq) {
        /* error: we are not recording */
        return STATUS_ERROR;
    }

    tb->lock();
    iq_sink->close();
    tb->unlock();
    d_recording_iq = false;
#endif
    return STATUS_OK;
}


/*! \brief Start playback of recorded I/Q data file.
 *  \param filename The file to play from. Must be raw file containing gr_complex samples.
 *  \param samprate The sample rate (currently fixed at 96ksps)
 */
receiver::status receiver::start_iq_playback(const std::string filename, float samprate)
{
#if 0
    if (samprate != d_bandwidth) {
        return STATUS_ERROR;
    }

    try {
        iq_src = gr_make_file_source(sizeof(gr_complex), filename.c_str(), false);
    }
    catch (std::runtime_error &e) {
        std::cout << "Error loading " << filename << ": " << e.what() << std::endl;
        return STATUS_ERROR;
    }

    tb->lock();

    /* disconenct hardware source */
    tb->disconnect(src, 0, nb, 0);
    tb->disconnect(src, 0, iq_sink, 0);

    /* connect I/Q source via throttle block */
    tb->connect(iq_src, 0, nb, 0);
    tb->connect(iq_src, 0, iq_sink, 0);
    tb->unlock();
#endif
    return STATUS_OK;
}


/*! \brief Stop I/Q data file playback.
 *  \return STATUS_OK
 *
 * This method will stop the I/Q data playback, disconnect the file source and throttle
 * blocks, and reconnect the hardware source.
 *
 * FIXME: will probably crash if we try to stop playback that is not running.
 */
receiver::status receiver::stop_iq_playback()
{
#if 0
    tb->lock();

    /* disconnect I/Q source and throttle block */
    tb->disconnect(iq_src, 0, nb, 0);
    tb->disconnect(iq_src, 0, iq_sink, 0);

    /* reconenct hardware source */
    tb->connect(src, 0, nb, 0);
    tb->connect(src, 0, iq_sink, 0);

    tb->unlock();

    /* delete iq_src since we can not reuse for other files */
    iq_src.reset();
#endif
    return STATUS_OK;
}



/*! \brief Start data sniffer.
 *  \param buffsize The buffer that should be used in the sniffer.
 *  \return STATUS_OK if the sniffer was started, STATUS_ERROR if the sniffer is already in use.
 */
receiver::status receiver::start_sniffer(unsigned int samprate, int buffsize)
{
    if (d_sniffer_active) {
        /* sniffer already in use */
        return STATUS_ERROR;
    }

    sniffer->set_buffer_size(buffsize);
    sniffer_rr = make_resampler_ff((float)samprate/(float)d_audio_rate);
    tb->lock();
    tb->connect(rx, 0, sniffer_rr, 0);
    tb->connect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = true;

    return STATUS_OK;
}

/*! \brief Stop data sniffer.
 *  \return STATUS_ERROR i the sniffer is not currently active.
 */
receiver::status receiver::stop_sniffer()
{
    if (!d_sniffer_active) {
        return STATUS_ERROR;
    }

    tb->lock();
    tb->disconnect(rx, 0, sniffer_rr, 0);
    tb->disconnect(sniffer_rr, 0, sniffer, 0);
    tb->unlock();
    d_sniffer_active = false;

    /* delete resampler */
    sniffer_rr.reset();

    return STATUS_OK;
}

/*! \brief Get sniffer data. */
void receiver::get_sniffer_data(float * outbuff, int &num)
{
    sniffer->get_samples(outbuff, num);
}



/*! \brief Convenience function to connect all blocks. */
void receiver::connect_all(rx_chain type)
{
    switch (type)
    {
    case RX_CHAIN_NONE:
        tb->connect(src, 0, dc_corr, 0);
        tb->connect(dc_corr, 0, iq_fft, 0);
        break;

    case RX_CHAIN_NBRX:
        if (rx->name() != "NBRX")
        {
            rx.reset();
            rx = make_nbrx(d_input_rate, d_audio_rate);
        }
        tb->connect(src, 0, dc_corr, 0);
        tb->connect(dc_corr, 0, iq_fft, 0);
        tb->connect(dc_corr, 0, mixer, 0);
        tb->connect(lo, 0, mixer, 1);
        tb->connect(mixer, 0, rx, 0);
        tb->connect(rx, 0, audio_fft, 0);
        tb->connect(rx, 0, audio_gain0, 0);
        tb->connect(rx, 1, audio_gain1, 0);
        tb->connect(audio_gain0, 0, audio_snk, 0);
        tb->connect(audio_gain1, 0, audio_snk, 1);
        break;

    case RX_CHAIN_WFMRX:
        if (rx->name() != "WFMRX")
        {
            rx.reset();
            rx = make_wfmrx(d_input_rate, d_audio_rate);
        }
        tb->connect(src, 0, dc_corr, 0);
        tb->connect(dc_corr, 0, iq_fft, 0);
        tb->connect(dc_corr, 0, mixer, 0);
        tb->connect(lo, 0, mixer, 1);
        tb->connect(mixer, 0, rx, 0);
        tb->connect(rx, 0, audio_fft, 0);
        tb->connect(rx, 0, audio_gain0, 0);
        tb->connect(rx, 1, audio_gain1, 0);
        tb->connect(audio_gain0, 0, audio_snk, 0);
        tb->connect(audio_gain1, 0, audio_snk, 1);
        break;

    default:
        break;
    }

}
