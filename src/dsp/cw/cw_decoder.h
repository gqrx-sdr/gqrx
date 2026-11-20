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
#ifndef CW_DECODER_H
#define CW_DECODER_H

#include <complex>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

/*! \brief Streaming Morse code (CW) decoder for demodulated audio.
 *
 * The decoder detects a single audio tone, measures key-down and key-up
 * durations in samples and converts them to text. The dot length is either
 * derived from a manual speed setting or, in automatic mode, estimated from
 * recent mark and gap durations. All detector and timing state is kept between
 * calls to process(), so the input may be split at arbitrary sample boundaries.
 *
 * A character is emitted once the following silence exceeds two dot lengths,
 * and a single word separator (space) once it exceeds five dot lengths.
 */
class CwDecoder
{
public:
    CwDecoder(double sample_rate, double tone_hz = 700.0, double wpm = 20.0);

    /*! \brief Process samples and return text decoded from them. */
    std::string process(const float *samples, std::size_t count);

    /*! \brief Start a new stream.
     *
     * Discards all detector state, any partially received character and the
     * speed evidence. Tone, manual speed and automatic mode are kept.
     */
    void reset();

    /*! \brief Set the audio tone to detect. Resets the decoder. */
    void set_tone(double tone_hz);

    /*! \brief Set the manual speed in words per minute. Resets the decoder. */
    void set_wpm(double wpm);

    /*! \brief Use the estimated speed instead of the manual one.
     *
     * Until an estimate is available the manual speed is used. Switching does
     * not reset the decoder or discard collected timing evidence.
     */
    void set_auto_wpm(bool enabled);

    double tone() const { return d_tone_hz; }
    double wpm() const { return d_wpm; } /*!< Manual speed. */
    bool auto_wpm() const { return d_auto_wpm; }

    /*! \brief Estimated speed in WPM, or 0 while there is not enough evidence. */
    double estimated_wpm() const;

private:
    void configure();

    void select_timing();

    void apply_timing(double dot, double min_len);

    bool detect(float sample);

    void update_timing(bool key_down, std::string &out);

    void classify_mark();

    void update_estimate();

    double estimate_dot() const;

    double d_sample_rate;
    double d_tone_hz;
    double d_wpm;
    bool d_auto_wpm;

    /* tone detector */
    double d_phase;
    double d_phase_inc;
    std::vector<std::complex<double> > d_ring; /*!< Moving average window. */
    std::size_t d_ring_pos;
    std::size_t d_ring_fill;
    std::complex<double> d_sum;
    double d_noise; /*!< Envelope level while key is up. */
    double d_peak; /*!< Envelope level while key is down. */
    std::uint64_t d_noise_count;
    double d_noise_alpha;
    double d_peak_attack;
    double d_peak_decay;
    bool d_key_down;

    /* Morse timing, all in samples */
    std::uint64_t d_min_len; /*!< Shorter marks/dropouts are ignored. */
    std::uint64_t d_dash_len; /*!< Dot/dash threshold. */
    std::uint64_t d_max_mark_len; /*!< Longer marks are not Morse. */
    std::uint64_t d_char_gap; /*!< Silence that completes a character. */
    std::uint64_t d_word_gap; /*!< Silence that completes a word. */
    std::uint64_t d_mark_len;
    std::uint64_t d_space_len; /*!< Silence since the last key-up. */
    std::uint64_t d_gap_len; /*!< Silence since the last accepted element. */
    bool d_in_mark;
    bool d_mark_pending; /*!< Mark ended but not yet classified. */
    bool d_word_pending; /*!< A character was emitted since the last space. */
    std::string d_symbols; /*!< Dots and dashes of the current character. */

    /* speed estimation, all in samples */
    bool d_have_element; /*!< An element was accepted since reset. */
    std::deque<double> d_marks; /*!< Recent element durations. */
    std::deque<double> d_gaps; /*!< Recent silences between elements. */
    double d_est_dot; /*!< Smoothed dot estimate, 0 if none. */
};

#endif // CW_DECODER_H
