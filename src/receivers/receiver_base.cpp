/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <functional>

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name, float pref_quad_rate, double quad_rate, int audio_rate)
    : gr::hier_block2 (src_name,
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(float))),
      vfo_s(),
      d_connected(false),
      d_decim_rate(quad_rate),
      d_quad_rate(0),
      d_ddc_decim(1),
      d_audio_rate(audio_rate),
      d_center_freq(145500000.0),
      d_pref_quad_rate(pref_quad_rate)
{
    d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
    d_quad_rate = d_decim_rate / d_ddc_decim;
    ddc = make_downconverter_cc(d_ddc_decim, 0.0, d_decim_rate);
    connect(self(), 0, ddc, 0);

    iq_resamp = make_resampler_cc(d_pref_quad_rate/float(d_quad_rate));
    agc = make_rx_agc_2f(d_audio_rate, d_agc_on, d_agc_target_level,
                         d_agc_manual_gain, d_agc_max_gain, d_agc_attack_ms,
                         d_agc_decay_ms, d_agc_hang_ms, d_agc_panning);
    sql = make_rx_sql_cc(d_level_db, d_alpha);
    meter = make_rx_meter_c(d_pref_quad_rate);
    wav_sink = wavfile_sink_gqrx::make(0, 2, (unsigned int) d_audio_rate,
                                       wavfile_sink_gqrx::FORMAT_WAV,
                                       wavfile_sink_gqrx::FORMAT_PCM_16);
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    wav_sink->set_rec_event_handler(std::bind(rec_event, this, std::placeholders::_1,
                                    std::placeholders::_2));
}

receiver_base_cf::~receiver_base_cf()
{
    //Prevent segfault
    if (wav_sink)
        wav_sink->set_rec_event_handler(nullptr);
}

void receiver_base_cf::set_demod(Modulations::idx demod)
{
    if ((get_demod() == Modulations::MODE_OFF) && (demod != Modulations::MODE_OFF))
    {
        qDebug() << "Changing RX quad rate:"  << d_decim_rate << "->" << d_quad_rate;
        lock();
        ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
        iq_resamp->set_rate(d_pref_quad_rate/float(d_quad_rate));
        unlock();
    }
    vfo_s::set_demod(demod);
}

void receiver_base_cf::set_quad_rate(double quad_rate)
{
    if (std::abs(d_decim_rate-quad_rate) > 0.5)
    {
        d_decim_rate = quad_rate;
        d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
        d_quad_rate = d_decim_rate / d_ddc_decim;
        //avoid triggering https://github.com/gnuradio/gnuradio/issues/5436
        if (get_demod() != Modulations::MODE_OFF)
        {
            qDebug() << "Changing RX quad rate:"  << d_decim_rate << "->" << d_quad_rate;
            lock();
            ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
            iq_resamp->set_rate(d_pref_quad_rate/float(d_quad_rate));
            unlock();
        }
    }
}

void receiver_base_cf::set_center_freq(double center_freq)
{
    d_center_freq = center_freq;
    wav_sink->set_center_freq(center_freq);
}

void receiver_base_cf::set_offset(int offset)
{
    vfo_s::set_offset(offset);
    ddc->set_center_freq(offset);
    wav_sink->set_offset(offset);
}

void receiver_base_cf::set_cw_offset(int offset)
{
    vfo_s::set_cw_offset(offset);
}

void receiver_base_cf::set_audio_rec_dir(const std::string& dir)
{
    vfo_s::set_audio_rec_dir(dir);
    wav_sink->set_rec_dir(dir);
}

void receiver_base_cf::set_audio_rec_sql_triggered(bool enabled)
{
    vfo_s::set_audio_rec_sql_triggered(enabled);
    sql->set_impl(enabled ? rx_sql_cc::SQL_PWR : rx_sql_cc::SQL_SIMPLE);
    wav_sink->set_sql_triggered(enabled);
}

void receiver_base_cf::set_audio_rec_min_time(const int time_ms)
{
    vfo_s::set_audio_rec_min_time(time_ms);
    wav_sink->set_rec_min_time(time_ms);
}

void receiver_base_cf::set_audio_rec_max_gap(const int time_ms)
{
    vfo_s::set_audio_rec_max_gap(time_ms);
    wav_sink->set_rec_max_gap(time_ms);
}

float receiver_base_cf::get_signal_level()
{
    return meter->get_level_db();
}

bool receiver_base_cf::has_nb()
{
    return false;
}

void receiver_base_cf::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
    vfo_s::set_sql_level(level_db);
}

void receiver_base_cf::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
    vfo_s::set_sql_alpha(alpha);
}

void receiver_base_cf::set_agc_on(bool agc_on)
{
    agc->set_agc_on(agc_on);
    vfo_s::set_agc_on(agc_on);
}

void receiver_base_cf::set_agc_target_level(int target_level)
{
    agc->set_target_level(target_level);
    vfo_s::set_agc_target_level(target_level);
}

void receiver_base_cf::set_agc_manual_gain(float gain)
{
    agc->set_manual_gain(gain);
    vfo_s::set_agc_manual_gain(gain);
}

void receiver_base_cf::set_agc_max_gain(int gain)
{
    agc->set_max_gain(gain);
    vfo_s::set_agc_max_gain(gain);
}

void receiver_base_cf::set_agc_attack(int attack_ms)
{
    agc->set_attack(attack_ms);
    vfo_s::set_agc_attack(attack_ms);
}

void receiver_base_cf::set_agc_decay(int decay_ms)
{
    agc->set_decay(decay_ms);
    vfo_s::set_agc_decay(decay_ms);
}

void receiver_base_cf::set_agc_hang(int hang_ms)
{
    agc->set_hang(hang_ms);
    vfo_s::set_agc_hang(hang_ms);
}

void receiver_base_cf::set_agc_panning(int panning)
{
    agc->set_panning(panning);
    vfo_s::set_agc_panning(panning);
}

float receiver_base_cf::get_agc_gain()
{
    return agc->get_current_gain();
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

int receiver_base_cf::start_audio_recording()
{
    return wav_sink->open_new();
}

bool receiver_base_cf::get_audio_recording()
{
    return wav_sink->is_active();
}

void receiver_base_cf::stop_audio_recording()
{
    wav_sink->close();
}

//FIXME Reimplement wavfile_sink correctly to make this work as expected
void receiver_base_cf::continue_audio_recording(receiver_base_cf_sptr from)
{
    if (from.get() == this)
        return;
    from->disconnect(from->agc, 0, from->wav_sink, 0);
    from->disconnect(from->agc, 1, from->wav_sink, 1);
    wav_sink = from->wav_sink;
    wav_sink->set_rec_event_handler(std::bind(rec_event, this, std::placeholders::_1,
                                    std::placeholders::_2));
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    from->wav_sink.reset();
}

std::string receiver_base_cf::get_last_audio_filename()
{
    return d_audio_filename;
}

void receiver_base_cf::rec_event(receiver_base_cf * self, std::string filename, bool is_running)
{
    self->d_audio_filename = filename;
    if (self->d_rec_event)
    {
        self->d_rec_event(self->get_index(), filename, is_running);
        std::cerr<<"d_rec_event("<<self->get_index()<<","<<filename<<","<<is_running<<")\n";
    }
}

void receiver_base_cf::restore_settings(receiver_base_cf& from)
{
    vfo_s::restore_settings(from);

    set_center_freq(from.d_center_freq);
}