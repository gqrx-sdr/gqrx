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
#ifndef VFO_H
#define VFO_H

#if GNURADIO_VERSION < 0x030900
    #include <boost/shared_ptr.hpp>
#endif

#include "receivers/defines.h"
#include "receivers/modulations.h"


class vfo_s;
typedef class vfo_s
{
    public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<vfo_s> sptr;
#else
    typedef std::shared_ptr<vfo_s> sptr;
#endif
    static sptr make()
    {
        return sptr(new vfo_s());
    }

    vfo_s():
        d_offset(0),
        d_filter_low(-5000),
        d_filter_high(5000),
        d_filter_tw(100),
        d_demod(Modulations::MODE_OFF),
        d_index(-1),
        d_locked(false),
        d_level_db(-150),
        d_alpha(0.001),
        d_agc_on(true),
        d_agc_target_level(0),
        d_agc_manual_gain(0),
        d_agc_max_gain(100),
        d_agc_attack_ms(20),
        d_agc_decay_ms(500),
        d_agc_hang_ms(0),
        d_agc_panning(0),
        d_agc_panning_auto(false),
        d_cw_offset(700),
        d_fm_maxdev(2500),
        d_fm_deemph(7.5e-5),
        d_am_dcr(true),
        d_amsync_dcr(true),
        d_amsync_pll_bw(0.01),
        d_rec_dir(""),
        d_rec_sql_triggered(false),
        d_rec_min_time(0),
        d_rec_max_gap(0)
    {
        for (int k = 0; k < RECEIVER_NB_COUNT; k++)
        {
            d_nb_on[k] = false;
            d_nb_threshold[k] = 2;
        }
    }

    virtual ~vfo_s()
    {
    }

    struct comp
    {
        inline bool operator()(const sptr lhs, const sptr rhs) const
        {
            const vfo_s *a = lhs.get();
            const vfo_s *b = rhs.get();
            return (a->d_offset == b->d_offset) ? (a->d_index < b->d_index) : (a->d_offset < b->d_offset);
        }
    };
    typedef std::set<sptr, comp> set;
    //getters
    inline int get_offset() const { return d_offset; }
    /* Filter parameter */
    inline int get_filter_low() const { return d_filter_low;}
    inline int get_filter_high() const { return d_filter_high;}
    inline int get_filter_tw() const { return d_filter_tw; }
    inline void get_filter(int &low, int &high, int &tw) const
    {
        low = d_filter_low;
        high = d_filter_high;
        tw = d_filter_tw;
    }
    inline Modulations::idx get_demod() const { return d_demod; }
    inline int get_index() const { return d_index; }
    inline bool get_freq_lock() const { return d_locked; }
    /* Squelch parameter */
    inline double get_sql_level() const { return d_level_db; }
    inline double get_sql_alpha() const { return d_alpha; }
    /* AGC */
    inline bool  get_agc_on() const { return d_agc_on; }
    inline int   get_agc_target_level() const { return d_agc_target_level; }
    inline float get_agc_manual_gain() const { return d_agc_manual_gain; }
    inline int   get_agc_max_gain() const { return d_agc_max_gain; }
    inline int   get_agc_attack() const { return d_agc_attack_ms; }
    inline int   get_agc_decay() const { return d_agc_decay_ms; }
    inline int   get_agc_hang() const { return d_agc_hang_ms; }
    inline int   get_agc_panning() const { return d_agc_panning; }
    inline bool  get_agc_panning_auto() const { return d_agc_panning_auto; }
    /* CW parameters */
    inline int   get_cw_offset() const { return d_cw_offset; }
    /* FM parameters */
    inline float get_fm_maxdev() const { return d_fm_maxdev; }
    inline double get_fm_deemph() const { return d_fm_deemph; }
    /* AM parameters */
    inline bool get_am_dcr() const { return d_am_dcr; }

    /* AM-Sync parameters */
    inline bool  get_amsync_dcr() const { return d_amsync_dcr; }
    inline float get_amsync_pll_bw() const { return d_amsync_pll_bw; }
    /* Noise blanker */
    bool get_nb_on(int nbid);
    float get_nb_threshold(int nbid);
    /* Audio recorder */
    inline const std::string& get_audio_rec_dir() const { return d_rec_dir; }
    inline bool get_audio_rec_sql_triggered() const { return d_rec_sql_triggered; }
    inline int get_audio_rec_min_time() const { return d_rec_min_time; }
    inline int get_audio_rec_max_gap() const { return d_rec_max_gap; }

    //setters
    virtual void set_offset(int offset);
    /* Filter parameter */
    virtual void set_filter_low(int low);
    virtual void set_filter_high(int high);
    virtual void set_filter_tw(int tw);
    virtual void set_filter(int low, int high, int tw);
    virtual void filter_adjust();

    virtual void set_demod(Modulations::idx demod);
    virtual void set_index(int index);
    virtual void set_freq_lock(bool on);
    /* Squelch parameter */
    virtual void set_sql_level(double level_db);
    virtual void set_sql_alpha(double alpha);
    /* AGC */
    virtual void  set_agc_on(bool agc_on);
    virtual void  set_agc_target_level(int target_level);
    virtual void  set_agc_manual_gain(float gain);
    virtual void  set_agc_max_gain(int gain);
    virtual void  set_agc_attack(int attack_ms);
    virtual void  set_agc_decay(int decay_ms);
    virtual void  set_agc_hang(int hang_ms);
    virtual void  set_agc_panning(int panning);
    virtual void  set_agc_panning_auto(bool mode);
    /* CW parameters */
    virtual void set_cw_offset(int offset);
    /* FM parameters */
    virtual void  set_fm_maxdev(float maxdev_hz);
    virtual void  set_fm_deemph(double tau);
    /* AM parameters */
    virtual void set_am_dcr(bool enabled);
    /* AM-Sync parameters */
    virtual void  set_amsync_dcr(bool enabled);
    virtual void  set_amsync_pll_bw(float pll_bw);
    /* Noise blanker */
    virtual void set_nb_on(int nbid, bool on);
    virtual void set_nb_threshold(int nbid, float threshold);
    /* Audio recorder */
    virtual void set_audio_rec_dir(const std::string& dir);
    virtual void set_audio_rec_sql_triggered(bool enabled);
    virtual void set_audio_rec_min_time(const int time_ms);
    virtual void set_audio_rec_max_gap(const int time_ms);

    virtual void restore_settings(vfo_s& from, bool force = true);

    protected:
    int              d_offset;
    int              d_filter_low;
    int              d_filter_high;
    int              d_filter_tw;
    Modulations::idx d_demod;
    int              d_index;
    bool             d_locked;

    double           d_level_db;
    double           d_alpha;

    bool             d_agc_on;
    int              d_agc_target_level;
    float            d_agc_manual_gain;
    int              d_agc_max_gain;
    int              d_agc_attack_ms;
    int              d_agc_decay_ms;
    int              d_agc_hang_ms;
    int              d_agc_panning;
    int              d_agc_panning_auto;

    int              d_cw_offset;        /*!< CW offset */

    float            d_fm_maxdev;
    double           d_fm_deemph;

    bool             d_am_dcr;
    bool             d_amsync_dcr;
    float            d_amsync_pll_bw;

    bool             d_nb_on[RECEIVER_NB_COUNT];
    float            d_nb_threshold[RECEIVER_NB_COUNT];

    std::string      d_rec_dir;
    bool             d_rec_sql_triggered;
    int              d_rec_min_time;
    int              d_rec_max_gap;


} vfo;


#endif //VFO_H