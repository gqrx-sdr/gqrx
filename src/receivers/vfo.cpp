/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2022 vladisslav2011@gmail.com.
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
#include "vfo.h"

void vfo_s::set_offset(int offset)
{
    d_offset = offset;
}

void vfo_s::set_filter_low(int low)
{
    d_filter_low = low;
}

void vfo_s::set_filter_high(int high)
{
    d_filter_high = high;
}

void vfo_s::set_filter_tw(int tw)
{
    d_filter_tw = tw;
}

void vfo_s::set_filter(int low, int high, int tw)
{
    d_filter_low = low;
    d_filter_high = high;
    d_filter_tw = tw;
}

void vfo_s::filter_adjust()
{
    Modulations::UpdateFilterRange(d_demod, d_filter_low, d_filter_high);
    Modulations::UpdateTw(d_filter_low, d_filter_high, d_filter_tw);
}

void vfo_s::set_demod(Modulations::idx demod)
{
    d_demod = demod;
}

void vfo_s::set_index(int index)
{
    d_index = index;
}

void vfo_s::set_freq_lock(bool on)
{
    d_locked = on;
}

void vfo_s::set_sql_level(double level_db)
{
    d_level_db = level_db;
}

void vfo_s::set_sql_alpha(double alpha)
{
    d_alpha = alpha;
}

void vfo_s::set_agc_on(bool agc_on)
{
    d_agc_on = agc_on;
}

void vfo_s::set_agc_target_level(int target_level)
{
    d_agc_target_level = target_level;
}

void vfo_s::set_agc_manual_gain(float gain)
{
    d_agc_manual_gain = gain;
}

void vfo_s::set_agc_max_gain(int gain)
{
    d_agc_max_gain = gain;
}

void vfo_s::set_agc_attack(int attack_ms)
{
    d_agc_attack_ms = attack_ms;
}

void vfo_s::set_agc_decay(int decay_ms)
{
    d_agc_decay_ms = decay_ms;
}

void vfo_s::set_agc_hang(int hang_ms)
{
    d_agc_hang_ms = hang_ms;
}

void vfo_s::set_agc_panning(int panning)
{
    d_agc_panning = panning;
}

void vfo_s::set_agc_panning_auto(bool mode)
{
    d_agc_panning_auto = mode;
}

void vfo_s::set_agc_mute(bool agc_mute)
{
    d_agc_mute = agc_mute;
}

void vfo_s::set_cw_offset(int offset)
{
    d_cw_offset = offset;
}

void vfo_s::set_fm_maxdev(float maxdev_hz)
{
    d_fm_maxdev = maxdev_hz;
}

void vfo_s::set_fm_deemph(double tau)
{
    d_fm_deemph = tau;
}

void vfo_s::set_am_dcr(bool enabled)
{
    d_am_dcr = enabled;
}

void vfo_s::set_amsync_dcr(bool enabled)
{
    d_amsync_dcr = enabled;
}

void vfo_s::set_amsync_pll_bw(float pll_bw)
{
    d_amsync_pll_bw = pll_bw;
}

void vfo_s::set_nb_on(int nbid, bool on)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_on[nbid - 1] = on;
}

bool vfo_s::get_nb_on(int nbid)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        return d_nb_on[nbid - 1];
    return false;
}

void vfo_s::set_nb_threshold(int nbid, float threshold)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_threshold[nbid - 1] = threshold;
}

float vfo_s::get_nb_threshold(int nbid)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        return d_nb_threshold[nbid - 1];
    return 0.0;
}

void vfo_s::set_audio_rec_dir(const std::string& dir)
{
    d_rec_dir = dir;
}

void vfo_s::set_audio_rec_sql_triggered(bool enabled)
{
    d_rec_sql_triggered = enabled;
}

void vfo_s::set_audio_rec_min_time(const int time_ms)
{
    d_rec_min_time = time_ms;
}

void vfo_s::set_audio_rec_max_gap(const int time_ms)
{
    d_rec_max_gap = time_ms;
}

/* UDP streaming */
bool vfo_s::set_udp_host(const std::string &host)
{
    d_udp_host = host;
    return true;
}

bool vfo_s::set_udp_port(int port)
{
    d_udp_port = port;
    return true;
}

bool vfo_s::set_udp_stereo(bool stereo)
{
    d_udp_stereo = stereo;
    return true;
}

void vfo_s::restore_settings(vfo_s& from, bool force)
{
    set_offset(from.get_offset());
    set_filter(from.get_filter_low(), from.get_filter_high(), from.get_filter_tw());
    set_freq_lock(from.get_freq_lock());
    set_demod(from.get_demod());
    set_sql_level(from.get_sql_level());
    set_sql_alpha(from.get_sql_alpha());

    set_agc_on(from.get_agc_on());
    set_agc_target_level(from.get_agc_target_level());
    set_agc_manual_gain(from.get_agc_manual_gain());
    set_agc_max_gain(from.get_agc_max_gain());
    set_agc_attack(from.get_agc_attack());
    set_agc_decay(from.get_agc_decay());
    set_agc_hang(from.get_agc_hang());
    set_agc_panning(from.get_agc_panning());
    set_agc_panning_auto(from.get_agc_panning_auto());

    set_cw_offset(from.get_cw_offset());

    set_fm_maxdev(from.get_fm_maxdev());
    set_fm_deemph(from.get_fm_deemph());

    set_am_dcr(from.get_am_dcr());
    set_amsync_dcr(from.get_amsync_dcr());
    set_amsync_pll_bw(from.get_amsync_pll_bw());

    for (int k = 0; k < RECEIVER_NB_COUNT; k++)
    {
        set_nb_on(k + 1, from.get_nb_on(k + 1));
        set_nb_threshold(k + 1, from.get_nb_threshold(k + 1));
    }
    if (force || (from.get_audio_rec_dir() != ""))
        set_audio_rec_dir(from.get_audio_rec_dir());
    if (force || (from.get_audio_rec_min_time() > 0))
        set_audio_rec_min_time(from.get_audio_rec_min_time());
    if (force || (from.get_audio_rec_max_gap() > 0))
        set_audio_rec_max_gap(from.get_audio_rec_max_gap());
    set_audio_rec_sql_triggered(from.get_audio_rec_sql_triggered());
    set_udp_host(from.get_udp_host());
    set_udp_port(from.get_udp_port());
    set_udp_stereo(from.get_udp_stereo());
}
