/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx Authors
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
#ifndef TX_DEVICE_ARGS_H
#define TX_DEVICE_ARGS_H

#include <map>
#include <string>
#include <vector>

/*! Parameters of one device in a gr-osmosdr device string (key -> value). */
typedef std::map<std::string, std::string> tx_device_params;

/*! \brief Split a gr-osmosdr device string into per-device parameters.
 *
 * Mirrors gr-osmosdr's args_to_vector() and params_to_dict() (arg_helpers.h):
 * devices are separated by spaces, parameters by commas, single quotes group
 * text, and a backslash escapes the next character. Global "numchan=" tokens
 * and empty tokens are skipped. Returns false where gr-osmosdr's parser would
 * throw.
 */
bool tx_devstr_split(const std::string &devstr, std::vector<tx_device_params> &devices);

/*! \brief Check whether a gr-osmosdr device string names a transmit driver.
 *
 * osmosdr::sink::make() only opens the device named in its argument string
 * when that string contains one of the sink driver keys. Otherwise it
 * enumerates every transmit-capable device it can find and opens the first
 * one (gr-osmosdr lib/sink_impl.cc). Probing with an RX-only string such as
 * "rtl=0" can therefore return a working sink for a different device that
 * the user never selected, e.g. a HackRF or USRP that is also plugged in.
 *
 * Returns true only if every device in \p devstr names one of gr-osmosdr's
 * sink drivers: uhd, hackrf, bladerf, soapy, redpitaya, freesrp or xtrx.
 * Returns false for an empty string, for source-only drivers (rtl, rtl_tcp,
 * airspy, airspyhf, sdrplay, fcd, rfspace, ...), for any string with a
 * file= device, and for strings gr-osmosdr itself would fail to parse.
 *
 * Parsing follows gr-osmosdr's arg_helpers.h: devices are separated by
 * spaces, parameters by commas, single quotes group text, and a backslash
 * escapes the next character.
 *
 * \note This only stops gr-osmosdr from choosing a device on its own. A
 * soapy= device can still be receive-only: gr-osmosdr's soapy sink does not
 * check the driver's TX channel count, and some Soapy modules accept a TX
 * stream setup anyway. The transmitter checks that separately when gqrx is
 * built with SoapySDR.
 */
bool tx_devstr_names_sink_driver(const std::string &devstr);

#endif // TX_DEVICE_ARGS_H
