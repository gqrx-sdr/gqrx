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

#include <functional>
#include <optional>
#include <string>

/* TX telemetry that needs neither Qt nor GNU Radio. */

/*! \brief Meters of the transmitter. An empty value means not exposed. */
struct tx_meters {
    std::optional<double> fwd_power;  /*!< Forward power in W. */
    std::optional<double> alc;        /*!< ALC in dB. */
    std::optional<double> swr;        /*!< SWR as a ratio, e.g. 1.2. */
};

/*! \brief Source of the TX meters. */
class tx_telemetry_provider
{
public:
    virtual ~tx_telemetry_provider() {}
    virtual tx_meters read_meters() const = 0;
};

/*! \brief Provider for backends without meters (gr-osmosdr has none). */
class null_tx_telemetry_provider : public tx_telemetry_provider
{
public:
    tx_meters read_meters() const override { return tx_meters(); }
};

/*! \brief Provider that returns fixed values, for tests. */
class fake_tx_telemetry_provider : public tx_telemetry_provider
{
public:
    explicit fake_tx_telemetry_provider(const tx_meters &meters) : m_meters(meters) {}
    tx_meters read_meters() const override { return m_meters; }

private:
    tx_meters m_meters;
};

/*! \brief Read the meters of a rig through hamlib level names.
 *  \param get_level  Returns false if the level is not available.
 *
 * Queries RFPOWER_METER_WATTS (then RFPOWER), ALC and SWR.
 */
tx_meters read_rigctl_meters(
    const std::function<bool(const char *level, double &out)> &get_level);

/* Text shown in the TX dock. */
std::string tx_telemetry_dash();
std::string tx_telemetry_rate_text(bool valid, double rate_hz);
std::string tx_telemetry_freq_text(bool valid, double freq_hz);
std::string tx_telemetry_gain_text(bool valid, double gain, double gmin, double gmax);
std::string tx_telemetry_fwd_text(std::optional<double> watts);
std::string tx_telemetry_alc_text(std::optional<double> db);
std::string tx_telemetry_swr_text(std::optional<double> ratio);
