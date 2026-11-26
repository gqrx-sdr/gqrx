/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Contributors.
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
#pragma once

#include <gnuradio/hier_block2.h>
#include <memory>
#include <string>

/* -----------------------------------------------------------------------
 * Abstract base for all TX modulators.
 *
 * Signal convention:
 *   Input port 0:  float  — microphone audio (at d_audio_rate Hz)
 *   Output port 0: gr_complex — baseband IQ  (at d_rf_rate Hz)
 * ----------------------------------------------------------------------- */
class tx_modulator_base;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<tx_modulator_base> tx_modulator_base_sptr;
#else
typedef std::shared_ptr<tx_modulator_base>   tx_modulator_base_sptr;
#endif

class tx_modulator_base : public gr::hier_block2
{
public:
    tx_modulator_base(const std::string &name,
                      unsigned int       audio_rate,
                      double             rf_rate);
    virtual ~tx_modulator_base() = default;

    virtual void set_audio_rate(unsigned int rate);
    virtual void set_rf_rate(double rate);

    unsigned int audio_rate() const { return d_audio_rate; }
    double       rf_rate()    const { return d_rf_rate; }

protected:
    unsigned int d_audio_rate;
    double       d_rf_rate;
};

