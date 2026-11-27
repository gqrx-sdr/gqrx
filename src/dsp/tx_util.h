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
#pragma once

#include <string>
#include <vector>

/* Helpers for the transmitter that need neither Qt nor GNU Radio. */

/*! \brief Select the device string used for transmitting.
 *  \param tx_dev     The configured TX device (may be empty).
 *  \param input_dev  The input device string of the receiver.
 *  \return The TX device, else the input device, else an empty string.
 *
 * Whitespace is trimmed. A "file=" input device cannot transmit and is not
 * used as a fallback.
 */
std::string tx_device_string(const std::string &tx_dev,
                             const std::string &input_dev);

/*! \brief Transmit frequency for a receive frequency and a split offset.
 *  \return rx_hz + split_hz, or 0 if that is not a positive frequency.
 */
double tx_frequency(double rx_hz, double split_hz);

/*! \brief The standard CTCSS tones in Hz, in increasing order. */
const std::vector<float>& ctcss_tones();

/*! \brief Check whether hz is one of the standard CTCSS tones. */
bool ctcss_valid(float hz);

/*! \brief Mixing levels of voice and CTCSS tone.
 *  \param tone_level  Tone amplitude relative to full deviation (clamped).
 *  \param voice_gain  Gain for the voice audio.
 *  \param tone_amp    Amplitude of the tone.
 *
 * The sum of both is 1, so the peak deviation never exceeds the maximum.
 */
void ctcss_levels(float tone_level, float &voice_gain, float &tone_amp);
