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
#include <algorithm>
#include <cmath>

#include "dsp/tx_util.h"

#define CTCSS_LEVEL_MIN 0.05f
#define CTCSS_LEVEL_MAX 0.3f

static std::string trim(const std::string &s)
{
    const char *ws = " \t\r\n";
    size_t first = s.find_first_not_of(ws);
    if (first == std::string::npos)
        return "";
    size_t last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

std::string tx_device_string(const std::string &tx_dev,
                             const std::string &input_dev)
{
    std::string tx = trim(tx_dev);
    if (!tx.empty())
        return tx;

    std::string in = trim(input_dev);
    if (in.compare(0, 5, "file=") == 0)
        return "";

    return in;
}

double tx_frequency(double rx_hz, double split_hz)
{
    double hz = rx_hz + split_hz;
    return (hz > 0.0) ? hz : 0.0;
}

const std::vector<float>& ctcss_tones()
{
    static const std::vector<float> tones = {
        67.0f,  69.3f,  71.9f,  74.4f,  77.0f,  79.7f,  82.5f,  85.4f,
        88.5f,  91.5f,  94.8f,  97.4f,  100.0f, 103.5f, 107.2f, 110.9f,
        114.8f, 118.8f, 123.0f, 127.3f, 131.8f, 136.5f, 141.3f, 146.2f,
        151.4f, 156.7f, 162.2f, 167.9f, 173.8f, 179.9f, 186.2f, 192.8f,
        203.5f, 210.7f, 218.1f, 225.7f, 233.6f, 241.8f, 250.3f, 254.1f
    };
    return tones;
}

bool ctcss_valid(float hz)
{
    const std::vector<float> &t = ctcss_tones();
    return std::find(t.begin(), t.end(), hz) != t.end();
}

void ctcss_levels(float tone_level, float &voice_gain, float &tone_amp)
{
    tone_level = std::min(std::max(tone_level, CTCSS_LEVEL_MIN), CTCSS_LEVEL_MAX);
    tone_amp = tone_level;
    voice_gain = 1.0f - tone_level;
}
