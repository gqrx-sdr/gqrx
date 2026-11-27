/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <cstdio>

#include "dsp/tx_telemetry.h"

static std::string format(const char *fmt, double a)
{
    char buf[64];
    snprintf(buf, sizeof(buf), fmt, a);
    return buf;
}

tx_meters read_rigctl_meters(
    const std::function<bool(const char *level, double &out)> &get_level)
{
    tx_meters m;
    double v;

    if (get_level("RFPOWER_METER_WATTS", v) || get_level("RFPOWER", v))
        m.fwd_power = v;
    if (get_level("ALC", v))
        m.alc = v;
    if (get_level("SWR", v))
        m.swr = v;

    return m;
}

std::string tx_telemetry_dash()
{
    return "\xe2\x80\x93";
}

std::string tx_telemetry_rate_text(bool valid, double rate_hz)
{
    return valid ? format("%1.3f Msps", rate_hz / 1.0e6) : tx_telemetry_dash();
}

std::string tx_telemetry_freq_text(bool valid, double freq_hz)
{
    return valid ? format("%1.6f MHz", freq_hz / 1.0e6) : tx_telemetry_dash();
}

std::string tx_telemetry_gain_text(bool valid, double gain, double gmin, double gmax)
{
    if (!valid)
        return tx_telemetry_dash();

    char buf[96];
    snprintf(buf, sizeof(buf), "%1.1f dB (%g%s%g)", gain, gmin,
             tx_telemetry_dash().c_str(), gmax);
    return buf;
}

std::string tx_telemetry_fwd_text(std::optional<double> watts)
{
    return watts ? format("%1.1f W", *watts) : tx_telemetry_dash();
}

std::string tx_telemetry_alc_text(std::optional<double> db)
{
    return db ? format("%1.1f dB", *db) : tx_telemetry_dash();
}

std::string tx_telemetry_swr_text(std::optional<double> ratio)
{
    return ratio ? format("%1.2f", *ratio) : tx_telemetry_dash();
}
