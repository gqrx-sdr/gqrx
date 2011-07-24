/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#include <gr_audio_sink.h>
#include <gr_complex_to_xxx.h>
#include <gr_multiply_const_ff.h>
#include <gr_agc2_cc.h>
#include <gr_simple_squelch_cc.h>

#include <receiver.h>
#include <fcd/fcd_source_c.h>
#include <dsp/rx_filter.h>
#include <dsp/rx_meter.h>
#include <dsp/rx_demod_fm.h>
#include <dsp/rx_demod_am.h>
#include <dsp/rx_fft.h>



/*! \brief Public contructor.
 *  \param input_device Input device specifier, e.g. hw:1 for FCD source.
 *  \param audio_device Audio output device specifier,
 *                      e.g. hw:0 when using ALSA or Portaudio.
 *
 * \todo Option to use UHD device instead of FCD.
 */
receiver::receiver(const std::string input_device, const std::string audio_device)
    : d_bandwidth(96000.0), d_audio_rate(48000),
      d_rf_freq(144800000.0), d_filter_offset(0.0),
      d_demod(DEMOD_FM),
      d_recording_wav(false)
{
    tb = gr_make_top_block("gqrx");

    fcd_src = fcd_make_source_c(input_device);
    fcd_src->set_freq(d_rf_freq);

    fft = make_rx_fft_c(3840, 0, false);  // FIXME: good for FCD with 96000 ksps

    filter = make_rx_filter(d_bandwidth, d_filter_offset, -5000.0, 5000.0, 1000.0);
    bb_gain = gr_make_multiply_const_cc(1.0);
    agc = gr_make_agc2_cc (0.5, 1.0e-4, 0.7, 1.0, 0.0);
    sql = gr_make_simple_squelch_cc(-100.0, 0.001);
    meter = make_rx_meter_c(false);
    demod_ssb = gr_make_complex_to_real(1);
    demod_fm = make_rx_demod_fm(48000.0, 48000.0, 5000.0, 50.0e-6);
    demod_am = make_rx_demod_am(48000.0, 48000.0, true);
    audio_gain = gr_make_multiply_const_ff(0.1);
    audio_snk = audio_make_sink(d_audio_rate, audio_device, true);
    wav_sink = gr_make_wavfile_sink("/tmp/gqrx.wav", 1, 48000, 16);

    tb->connect(fcd_src, 0, fft, 0);
    tb->connect(fcd_src, 0, filter, 0);
    tb->connect(filter, 0, meter, 0);
    tb->connect(filter, 0, sql, 0);
    tb->connect(sql, 0, bb_gain, 0);
    tb->connect(bb_gain, 0, agc, 0);
    tb->connect(agc, 0, demod_fm, 0);
    tb->connect(demod_fm, 0, wav_sink, 0);
    tb->connect(demod_fm, 0, audio_gain, 0);
    tb->connect(audio_gain, 0, audio_snk, 0);

    /* close wav file; this will disable writing */
    wav_sink->close();
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
    tb->start();
}


/*! \brief Stop the receiver. */
void receiver::stop()
{
    tb->stop();
    tb->wait(); // If the graph is needed to run again, wait() must be called after stop
    // FIXME: aUaO
}


/*! \brief Select new input device.
 *
 * \bug When using ALSA, program will crash if the new device
 *      is the same as the previously used device:
 *      audio_alsa_source[hw:1]: Device or resource busy
 */
void receiver::set_input_device(const std::string device)
{
    tb->lock();

    tb->disconnect(fcd_src, 0, fft, 0);
    tb->disconnect(fcd_src, 0, filter, 0);

    fcd_src.reset();
    fcd_src = fcd_make_source_c(device);
    fcd_src->set_freq(d_rf_freq);

    tb->connect(fcd_src, 0, fft, 0);
    tb->connect(fcd_src, 0, filter, 0);

    tb->unlock();
}


/*! \brief Select new audio output device. */
void receiver::set_output_device(const std::string device)
{
    tb->lock();

    tb->disconnect(audio_gain, 0, audio_snk, 0);
    audio_snk.reset();
    audio_snk = audio_make_sink(d_audio_rate, device, true);
    tb->connect(audio_gain, 0, audio_snk, 0);

    tb->unlock();
}



/*! \brief Set RF frequency.
 *  \param freq_hz The desired frequency in Hz.
 *  \return RX_STATUS_ERROR if an error occurs, e.g. the frequency is out of range.
 *  \sa get_rf_freq()
 */
receiver::status receiver::set_rf_freq(float freq_hz)
{
    d_rf_freq = freq_hz;
    /* FIXME: check frequency? */
    fcd_src->set_freq(d_rf_freq);

    return STATUS_OK;
}

/*! \brief Get RF frequency.
 *  \return The current RF frequency.
 *  \sa set_rf_freq()
 */
float receiver::get_rf_freq()
{
    return d_rf_freq;
}

/*! \brief Set RF gain.
 *  \param gain_db The desired gain in dB.
 *  \return RX_STATUS_ERROR if an error occurs, e.g. the gain is out of valid range.
 */
receiver::status receiver::set_rf_gain(float gain_db)
{
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
    filter->set_offset(d_filter_offset);
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

    filter->set_param(low, high, trans_width);

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


receiver::status receiver::set_dc_corr(double dci, double dcq)
{
    fcd_src->set_dc_corr(dci, dcq);

    return STATUS_OK;
}

receiver::status receiver::set_iq_corr(double gain, double phase)
{
    fcd_src->set_iq_corr(gain, phase);

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
    if (dbfs)
        return meter->get_level_db();
    else
        return meter->get_level();
}

/*! \brief Get latest FFT data. */
void receiver::get_fft_data(std::complex<float>* fftPoints, int &fftsize)
{
    fft->get_fft_data(fftPoints, fftsize);
}



/*! \brief Set squelch level.
 *  \param level_db The new level in dBFS.
 */
receiver::status receiver::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
}


/*! \brief Set squelch alpha */
receiver::status receiver::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
}


/*! \brief Set baseband gain.
 *  \param gain_db The new gain in dB.
 *
 * This is a manually controlled gain setting that is placed in between
 * the squelch and the AGC. This gain is very useful for AM-type modulations
 * where the amplitude of the incoming signal (often less than -60 dBFS)
 * is converted directly to audio amplitude.
 */
receiver::status receiver::set_bb_gain(float gain_db)
{
    float k;

    /* convert dB to factor */
    k = pow(10.0, gain_db / 20.0);
    //std::cout << "G:" << gain_db << "dB / K:" << k << std::endl;
    bb_gain->set_k(k);

    return STATUS_OK;
}



receiver::status receiver::set_demod(demod rx_demod)
{
    status ret = STATUS_OK;
    demod current_demod = d_demod;

    /* check if new demodulator selection is valid */
    if ((rx_demod < DEMOD_SSB) || (rx_demod >= DEMOD_NUM))
        return STATUS_ERROR;

    if (rx_demod == current_demod) {
        /* nothing to do */
        return STATUS_OK;
    }

    /* lock graph while we reconfigure */
    tb->lock();

    /* disconnect current demodulator */
    switch (current_demod) {

    case DEMOD_SSB:
        tb->disconnect(agc, 0, demod_ssb, 0);
        tb->disconnect(demod_ssb, 0, audio_gain, 0);
        tb->disconnect(demod_ssb, 0, wav_sink, 0);
        break;

    case DEMOD_AM:
        tb->disconnect(agc, 0, demod_am, 0);
        tb->disconnect(demod_am, 0, audio_gain, 0);
        tb->disconnect(demod_am, 0, wav_sink, 0);
        break;

    case DEMOD_FM:
        tb->disconnect(agc, 0, demod_fm, 0);
        tb->disconnect(demod_fm, 0, audio_gain, 0);
        tb->disconnect(demod_fm, 0, wav_sink, 0);
        break;


    }


    switch (rx_demod) {

    case DEMOD_SSB:
        d_demod = rx_demod;
        tb->connect(agc, 0, demod_ssb, 0);
        tb->connect(demod_ssb, 0, audio_gain, 0);
        tb->connect(demod_ssb, 0, wav_sink, 0);
        break;

    case DEMOD_AM:
        d_demod = rx_demod;
        tb->connect(agc, 0, demod_am, 0);
        tb->connect(demod_am, 0, audio_gain, 0);
        tb->connect(demod_am, 0, wav_sink, 0);
        break;

    case DEMOD_FM:
        d_demod = DEMOD_FM;
        tb->connect(agc, 0, demod_fm, 0);
        tb->connect(demod_fm, 0, audio_gain, 0);
        tb->connect(demod_fm, 0, wav_sink, 0);
        break;

    default:
        /* use FMN */
        d_demod = DEMOD_FM;
        tb->connect(agc, 0, demod_fm, 0);
        tb->connect(demod_fm, 0, audio_gain, 0);
        break;
    }

    /* continue processing */
    tb->unlock();

    return ret;
}


/*! \brief Set maximum deviation of the FM demodulator.
 *  \param maxdev_hz The new maximum deviation in Hz.
 */
receiver::status receiver::set_fm_maxdev(float maxdev_hz)
{
    demod_fm->set_max_dev(maxdev_hz);

    return STATUS_OK;
}


receiver::status receiver::set_fm_deemph(double tau)
{
    demod_fm->set_tau(tau);

    return STATUS_OK;
}


/*! \brief Set AM DCR status.
 *  \param enabled Flag indicating whether DCR should be enabled or disabled.
 */
receiver::status receiver::set_am_dcr(bool enabled)
{
    demod_am->set_dcr(enabled);

    return STATUS_OK;
}


receiver::status receiver::set_af_gain(float gain_db)
{
    float k;

    /* convert dB to factor */
    k = pow(10.0, gain_db / 20.0);
    //std::cout << "G:" << gain_db << "dB / K:" << k << std::endl;
    audio_gain->set_k(k);

    return STATUS_OK;
}


/*! \brief Start WAV file recorder.
 *  \param filename The filename where to record.
 */
receiver::status receiver::start_recording(const std::string filename)
{
    if (d_recording_wav) {
        /* error - we are already recording */
        return STATUS_ERROR;
    }

    /* wav_sink was created in the constructor */
    if (wav_sink) {
        //std::cout << "WAV sink exists" << std::endl;

        // not strictly necessary to lock but I think it is safer
        tb->lock();
        wav_sink->open(filename.c_str());
        tb->unlock();
        d_recording_wav = true;
    }
    else {
        std::cout << "WAV sink does not exist" << std::endl;
    }

    return STATUS_OK;
}


/*! \brief Stop WAV file recorder. */
receiver::status receiver::stop_recording()
{
    if (!d_recording_wav) {
        /* error: we are not recording */
        return STATUS_ERROR;
    }

    // not strictly necessary to lock but I think it is safer
    tb->lock();
    wav_sink->close();
    tb->unlock();
    d_recording_wav = false;

    return STATUS_OK;
}

