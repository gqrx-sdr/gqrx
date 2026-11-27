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
#define DCS_BAUD        134.4

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

const std::vector<int>& dcs_codes()
{
    static const std::vector<int> codes = {
        023, 025, 026, 031, 032, 036, 043, 047, 051, 053, 054, 065, 071, 072,
        073, 074, 0114, 0115, 0116, 0122, 0125, 0131, 0132, 0134, 0143, 0145,
        0152, 0155, 0156, 0162, 0165, 0172, 0174, 0205, 0212, 0223, 0225, 0226,
        0243, 0244, 0245, 0246, 0251, 0252, 0255, 0261, 0263, 0265, 0266, 0271,
        0274, 0306, 0311, 0315, 0325, 0331, 0332, 0343, 0346, 0351, 0356, 0364,
        0365, 0371, 0411, 0412, 0413, 0423, 0431, 0432, 0445, 0446, 0452, 0454,
        0455, 0462, 0464, 0465, 0466, 0503, 0506, 0516, 0523, 0526, 0532, 0546,
        0565, 0606, 0612, 0624, 0627, 0631, 0632, 0654, 0662, 0664, 0703, 0712,
        0723, 0731, 0732, 0734, 0743, 0754
    };
    return codes;
}

bool dcs_valid(int code)
{
    const std::vector<int> &c = dcs_codes();
    return std::find(c.begin(), c.end(), code) != c.end();
}

/* Golay (23,12) encoder: [11 check bits][12 data bits] */
static uint32_t golay2312(uint32_t data)
{
    data &= 0xFFF;
    uint32_t cw = data;
    for (int i = 0; i < 12; i++)
    {
        if (cw & 1)
            cw ^= 0xC75;
        cw >>= 1;
    }
    return (cw << 12) | data;
}

uint32_t dcs_word(int code, bool inverted)
{
    uint32_t word = golay2312(0x800 | (code & 0x1FF));
    return inverted ? (~word) & 0x7FFFFF : word;
}

std::vector<float> dcs_samples(int code, bool inverted, float audio_rate,
                               float amplitude)
{
    const uint32_t word = dcs_word(code, inverted);
    const double bit_len = audio_rate / DCS_BAUD;
    std::vector<float> samples;

    samples.reserve((size_t)(bit_len * 23) + 1);
    double edge = 0.0;
    for (int bit = 0; bit < 23; bit++)
    {
        edge += bit_len;
        float v = ((word >> bit) & 1) ? amplitude : -amplitude;
        while ((double)(samples.size() + 1) <= edge + 1e-9)
            samples.push_back(v);
    }
    return samples;
}
