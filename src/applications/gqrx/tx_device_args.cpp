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
#include "applications/gqrx/tx_device_args.h"

#include <vector>

namespace {

/* Sink driver keys known to gr-osmosdr (lib/sink_impl.cc), minus "file". */
const char *const sink_drivers[] = {
    "uhd", "hackrf", "bladerf", "soapy", "redpitaya", "freesrp", "xtrx"
};

/*
 * Split the way boost::escaped_list_separator<char>("\\", sep, "'") does,
 * which is what gr-osmosdr's args_to_vector() and params_to_vector() use.
 * Returns false where boost would throw (bad or trailing escape).
 */
bool split_escaped(const std::string &in, char sep, std::vector<std::string> &out)
{
    out.clear();
    if (in.empty())
        return true;

    std::string token;
    bool in_quote = false;

    for (size_t i = 0; i < in.size(); i++)
    {
        const char c = in[i];
        if (c == '\\')
        {
            if (++i == in.size())
                return false;
            const char n = in[i];
            if (n == 'n')
                token += '\n';
            else if (n == '\'' || n == sep || n == '\\')
                token += n;
            else
                return false;
        }
        else if (c == sep && !in_quote)
        {
            out.push_back(token);
            token.clear();
        }
        else if (c == '\'')
        {
            in_quote = !in_quote;
        }
        else
        {
            token += c;
        }
    }
    out.push_back(token);
    return true;
}

bool is_sink_driver(const std::string &key)
{
    for (const char *driver : sink_drivers)
        if (key == driver)
            return true;
    return false;
}

} // namespace

bool tx_devstr_names_sink_driver(const std::string &devstr)
{
    std::vector<std::string> devices;
    if (!split_escaped(devstr, ' ', devices))
        return false;

    int named = 0;
    for (const std::string &device : devices)
    {
        // Global "numchan=N" tokens are not devices (see args_to_io_signature()).
        if (device.rfind("numchan=", 0) == 0)
            continue;

        std::vector<std::string> params;
        if (!split_escaped(device, ',', params))
            return false;

        // gr-osmosdr ignores tokens without parameters (e.g. repeated spaces).
        if (params.empty())
            continue;

        bool has_sink_driver = false;
        for (const std::string &param : params)
        {
            const std::string key = param.substr(0, param.find('='));
            // A file= key makes gr-osmosdr open (and truncate) a file sink.
            if (key == "file")
                return false;
            if (is_sink_driver(key))
                has_sink_driver = true;
        }

        // Every device must name a sink driver. Otherwise TX would either be
        // auto-selected or land on a different device than the one meant.
        if (!has_sink_driver)
            return false;
        named++;
    }

    return named > 0;
}
