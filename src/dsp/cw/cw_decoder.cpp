/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Lukas Kohlfürst.
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
#include "cw_decoder.h"

namespace
{
    const double TWO_PI = 6.28318530717958647692;

    /* Envelope levels below this are treated as silence. */
    const double ENV_FLOOR = 1.0e-5;

    /* Required ratio between tone and noise envelope (about 12 dB). */
    const double MIN_SNR = 4.0;

    /* Key-down / key-up thresholds as fraction of the noise-to-peak span. */
    const double KEY_DOWN_LEVEL = 0.6;
    const double KEY_UP_LEVEL = 0.4;

    const std::size_t MAX_SYMBOLS = 10;

    /* Automatic speed estimation */
    const double AUTO_MIN_WPM = 5.0;
    const double AUTO_MAX_WPM = 60.0;
    const std::size_t EST_HISTORY = 16; /* Recent marks and gaps kept. */
    const std::size_t EST_MIN_MARKS = 6;
    const std::size_t EST_MIN_PER_CLUSTER = 2;
    const std::size_t EST_MIN_GAPS = 3;
    const double EST_SPLIT_RATIO = 1.6; /* Neighbour ratio that splits dots from dashes. */
    const double EST_DASH_RATIO_MIN = 2.0; /* Accepted dash/dot length ratio. */
    const double EST_DASH_RATIO_MAX = 4.5;
    const double EST_SMOOTHING = 0.25;
    const double EST_SEED_MAX_GLITCH = 0.010; /* Glitch length limit (s) before an estimate. */

    struct MorseCode
    {
        const char *code;
        const char *text;
    };

    const MorseCode MORSE_TABLE[] = {
        {".-", "A"}, {"-...", "B"}, {"-.-.", "C"}, {"-..", "D"}, {".", "E"},
        {"..-.", "F"}, {"--.", "G"}, {"....", "H"}, {"..", "I"}, {".---", "J"},
        {"-.-", "K"}, {".-..", "L"}, {"--", "M"}, {"-.", "N"}, {"---", "O"},
        {".--.", "P"}, {"--.-", "Q"}, {".-.", "R"}, {"...", "S"}, {"-", "T"},
        {"..-", "U"}, {"...-", "V"}, {".--", "W"}, {"-..-", "X"}, {"-.--", "Y"},
        {"--..", "Z"},
        {"-----", "0"}, {".----", "1"}, {"..---", "2"}, {"...--", "3"},
        {"....-", "4"}, {".....", "5"}, {"-....", "6"}, {"--...", "7"},
        {"---..", "8"}, {"----.", "9"},
        {".-.-.-", "."}, {"--..--", ","}, {"..--..", "?"}, {".----.", "'"},
        {"-.-.--", "!"}, {"-..-.", "/"}, {"-.--.", "("}, {"-.--.-", ")"},
        {".-...", "&"}, {"---...", ":"}, {"-.-.-.", ";"}, {"-...-", "="},
        {".-.-.", "+"}, {"-....-", "-"}, {"..--.-", "_"}, {".-..-.", "\""},
        {"...-..-", "$"}, {".--.-.", "@"}, {"...-.-", "<SK>"},
    };

    const char *lookup(const std::string &symbols)
    {
        for (const auto &entry: MORSE_TABLE)
        {
            if (symbols == entry.code)
            {
                return entry.text;
            }
        }
        return "*";
    }

    std::uint64_t to_samples(double samples)
    {
        if (samples < 1.0)
        {
            return 1;
        }
        return static_cast<std::uint64_t>(std::llround(samples));
    }

    /* Longest mark or gap used as speed evidence: a slow dash with some margin.
     * Longer key-down is a carrier, longer key-up is a pause. */
    double max_evidence_len(double sample_rate)
    {
        return 1.5 * 3.0 * sample_rate * 1.2 / AUTO_MIN_WPM;
    }

    double mean(const std::vector<double> &v, std::size_t begin, std::size_t end)
    {
        double sum = 0.0;
        for (std::size_t i = begin; i < end; i++)
            sum += v[i];
        return sum / static_cast<double>(end - begin);
    }
} // namespace


CwDecoder::CwDecoder(double sample_rate, double tone_hz, double wpm)
    : d_sample_rate(sample_rate),
      d_tone_hz(tone_hz),
      d_wpm(wpm),
      d_auto_wpm(false)
{
    configure();
}

void CwDecoder::set_tone(double tone_hz)
{
    d_tone_hz = tone_hz;
    configure();
}

void CwDecoder::set_wpm(double wpm)
{
    d_wpm = wpm;
    configure();
}

void CwDecoder::set_auto_wpm(bool enabled)
{
    d_auto_wpm = enabled;
    select_timing();
}

double CwDecoder::estimated_wpm() const
{
    if (d_est_dot <= 0.0)
    {
        return 0.0;
    }
    return d_sample_rate * 1.2 / d_est_dot;
}

/*! \brief Derive detector parameters, then reset. */
void CwDecoder::configure()
{
    d_tone_hz = std::max(0.0, std::min(d_tone_hz, d_sample_rate / 2.0));
    d_wpm = std::max(1.0, std::min(d_wpm, 100.0));

    /* PARIS timing: one dot is 1.2 / WPM seconds */
    const double dot = d_sample_rate * 1.2 / d_wpm;

    /* 10 ms (100 Hz wide) moving average, shorter only for very fast Morse */
    d_ring.assign(to_samples(std::min(dot / 2.0, d_sample_rate / 100.0)),
                  std::complex<double>(0.0, 0.0));
    d_phase_inc = TWO_PI * d_tone_hz / d_sample_rate;
    d_noise_alpha = 1.0 / (0.25 * d_sample_rate);
    d_peak_decay = 1.0 / (2.0 * d_sample_rate);

    reset();
}

/*! \brief Use the manual speed, or the estimated one in automatic mode. */
void CwDecoder::select_timing()
{
    if (d_auto_wpm && d_est_dot > 0.0)
    {
        apply_timing(d_est_dot, d_est_dot / 4.0);
        return;
    }

    const double dot = d_sample_rate * 1.2 / d_wpm;
    double min_len = dot / 4.0;

    /* don't let a slow manual setting merge the elements we need to measure */
    if (d_auto_wpm)
    {
        min_len = std::min(min_len, EST_SEED_MAX_GLITCH * d_sample_rate);
    }

    apply_timing(dot, min_len);
}

/*! \brief Set timing thresholds without touching detector or timing state. */
void CwDecoder::apply_timing(double dot, double min_len)
{
    d_peak_attack = 1.0 / dot;

    d_min_len = to_samples(min_len);
    d_dash_len = to_samples(2.0 * dot);
    d_max_mark_len = to_samples(10.0 * dot);
    d_char_gap = to_samples(2.0 * dot);
    d_word_gap = to_samples(5.0 * dot);
}

void CwDecoder::reset()
{
    d_phase = 0.0;
    std::fill(d_ring.begin(), d_ring.end(), std::complex<double>(0.0, 0.0));
    d_ring_pos = 0;
    d_ring_fill = 0;
    d_sum = std::complex<double>(0.0, 0.0);
    d_noise = 0.0;
    d_peak = 0.0;
    d_noise_count = 0;
    d_key_down = false;

    d_mark_len = 0;
    d_space_len = 0;
    d_gap_len = 0;
    d_in_mark = false;
    d_mark_pending = false;
    d_word_pending = false;
    d_symbols.clear();

    d_have_element = false;
    d_marks.clear();
    d_gaps.clear();
    d_est_dot = 0.0;
    select_timing();
}

std::string CwDecoder::process(const float *samples, std::size_t count)
{
    std::string out;

    for (std::size_t i = 0; i < count; i++)
        update_timing(detect(samples[i]), out);

    return out;
}

/*! \brief Tone detector: mix to DC, moving average, adaptive threshold. */
bool CwDecoder::detect(float sample)
{
    const double x = static_cast<double>(sample);
    const std::complex<double> v(x * std::cos(d_phase), -x * std::sin(d_phase));

    d_phase += d_phase_inc;
    if (d_phase >= TWO_PI)
    {
        d_phase -= TWO_PI;
    }

    d_sum += v - d_ring[d_ring_pos];
    d_ring[d_ring_pos] = v;
    if (++d_ring_pos == d_ring.size())
    {
        /* recalculate once per window so rounding errors can't accumulate */
        d_ring_pos = 0;
        d_sum = std::complex<double>(0.0, 0.0);
        for (const auto &r: d_ring)
            d_sum += r;
    }

    if (d_ring_fill < d_ring.size())
    {
        d_ring_fill++;
        return false;
    }

    const double env = std::abs(d_sum) / static_cast<double>(d_ring.size());

    if (env > d_peak)
    {
        d_peak += (env - d_peak) * d_peak_attack;
    }
    else
    {
        d_peak += (d_noise - d_peak) * d_peak_decay;
    }

    if (!d_key_down)
    {
        /* plain average until the time constant is reached, then EMA */
        d_noise_count++;
        const double alpha = std::max(d_noise_alpha,
                                      1.0 / static_cast<double>(d_noise_count));
        d_noise += (env - d_noise) * alpha;
    }

    /* The noise level follows the envelope until key-down, while the peak
     * rises only over about one dot. Checking the SNR on the peak alone lets
     * the noise level absorb a fast first element so the key never goes down;
     * the current envelope may open the gate on its own. */
    const double level = std::max(env, d_peak);
    const double span = d_peak - d_noise;
    if (d_key_down)
    {
        d_key_down = env > d_noise + KEY_UP_LEVEL * span;
    }
    else
    {
        d_key_down = level > ENV_FLOOR && level > MIN_SNR * d_noise &&
                     env > d_noise + KEY_DOWN_LEVEL * span;
    }

    return d_key_down;
}

/*! \brief Advance Morse timing by one sample and append completed text. */
void CwDecoder::update_timing(bool key_down, std::string &out)
{
    if (key_down)
    {
        if (!d_in_mark)
        {
            d_in_mark = true;
            if (d_mark_pending)
            {
                /* dropout too short to be a gap; continue the previous mark */
                d_mark_len += d_space_len;
                d_gap_len -= d_space_len;
                d_mark_pending = false;
            } else
            {
                d_mark_len = 0;
            }
            d_space_len = 0;
        }
        d_mark_len++;
        return;
    }

    if (d_in_mark)
    {
        d_in_mark = false;
        d_mark_pending = true;
        d_space_len = 0;
    }
    d_space_len++;
    d_gap_len++;

    if (d_mark_pending)
    {
        if (d_space_len < d_min_len)
        {
            return;
        }
        d_mark_pending = false;
        classify_mark();
    }

    if (!d_symbols.empty() && d_gap_len >= d_char_gap)
    {
        out += lookup(d_symbols);
        d_symbols.clear();
        d_word_pending = true;
    }

    if (d_word_pending && d_gap_len >= d_word_gap)
    {
        out += ' ';
        d_word_pending = false;
    }
}

void CwDecoder::classify_mark()
{
    if (d_mark_len < d_min_len)
    {
        /* noise spike; count it as part of the surrounding silence */
        d_gap_len += d_mark_len;
        return;
    }

    /* collect speed evidence; the gap is the silence before this mark */
    const double max_len = max_evidence_len(d_sample_rate);
    const double gap = static_cast<double>(d_gap_len - d_space_len);
    const double len = static_cast<double>(d_mark_len);

    if (d_have_element && gap <= max_len)
    {
        d_gaps.push_back(gap);
        if (d_gaps.size() > EST_HISTORY)
        {
            d_gaps.pop_front();
        }
    }
    d_have_element = len <= max_len;
    if (d_have_element && (d_est_dot <= 0.0 || len <= 10.0 * d_est_dot))
    {
        d_marks.push_back(len);
        if (d_marks.size() > EST_HISTORY)
        {
            d_marks.pop_front();
        }
    }

    if (d_mark_len > d_max_mark_len)
    {
        d_symbols.clear(); // steady carrier, not Morse
    }
    else if (d_symbols.size() <= MAX_SYMBOLS) // longer than any code: "*"
    {
        d_symbols += (d_mark_len < d_dash_len) ? '.' : '-';
    }

    d_gap_len = d_space_len;

    if (d_have_element)
    {
        update_estimate();
    }
}

/*! \brief Update the smoothed dot estimate after a new mark. */
void CwDecoder::update_estimate()
{
    const double dot = estimate_dot();
    if (dot <= 0.0)
    {
        return;
    }

    if (d_est_dot <= 0.0)
    {
        d_est_dot = dot;
    }
    else
    {
        d_est_dot += (dot - d_est_dot) * EST_SMOOTHING;
    }

    if (d_auto_wpm)
    {
        select_timing();
    }
}

/*! \brief Estimate the dot length from recent marks and gaps.
 *
 * Marks are split into a short (dot) and a long (dash) cluster at the largest
 * ratio between neighbouring sorted lengths. If both clusters are present and
 * about 3:1 apart, the dot is fitted to both (dash = 3 dots). If all marks are
 * alike they could be dots or dashes, so the silences between elements decide:
 * one-unit gaps about a third of the mark length mean the marks are dashes,
 * gaps similar to the mark length mean they are dots.
 *
 * Returns 0 when the evidence is insufficient or inconsistent.
 */
double CwDecoder::estimate_dot() const
{
    if (d_marks.size() < EST_MIN_MARKS)
    {
        return 0.0;
    }

    std::vector<double> marks(d_marks.begin(), d_marks.end());
    std::sort(marks.begin(), marks.end());

    std::size_t split = 0;
    double split_ratio = 1.0;
    for (std::size_t i = 1; i < marks.size(); i++)
    {
        const double ratio = marks[i] / marks[i - 1];
        if (ratio > split_ratio)
        {
            split_ratio = ratio;
            split = i;
        }
    }

    double dot = 0.0;
    if (split_ratio >= EST_SPLIT_RATIO)
    {
        const std::size_t n_dots = split;
        const std::size_t n_dashes = marks.size() - split;
        if (n_dots < EST_MIN_PER_CLUSTER || n_dashes < EST_MIN_PER_CLUSTER)
        {
            return 0.0;
        }

        const double dots = mean(marks, 0, split);
        const double dashes = mean(marks, split, marks.size());
        const double ratio = dashes / dots;
        if (ratio < EST_DASH_RATIO_MIN || ratio > EST_DASH_RATIO_MAX)
        {
            return 0.0;
        }

        dot = (dots * static_cast<double>(n_dots) + dashes * static_cast<double>(n_dashes)) /
              static_cast<double>(n_dots + 3 * n_dashes);
    } else
    {
        const double mark = mean(marks, 0, marks.size());
        std::size_t dot_like = 0;
        std::size_t dash_like = 0;

        for (const double gap: d_gaps)
        {
            const double ratio = gap / mark;
            if (ratio >= 0.2 && ratio <= 0.5)
            {
                dash_like++;
            }
            else if (ratio >= 0.6 && ratio <= 1.6)
            {
                dot_like++;
            }
        }

        if (dash_like >= EST_MIN_GAPS)
        {
            dot = mark / 3.0;
        }
        else if (dot_like >= EST_MIN_GAPS)
        {
            dot = mark;
        }
        else
        {
            return 0.0;
        }
    }

    const double wpm = d_sample_rate * 1.2 / dot;
    if (wpm < AUTO_MIN_WPM || wpm > AUTO_MAX_WPM)
    {
        return 0.0;
    }

    return dot;
}
