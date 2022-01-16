/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2013 Alexandru Csete OZ9AEC.
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
#if GNURADIO_VERSION < 0x031000
#include <gnuradio/blocks/udp_sink.h>
#else
#include <gnuradio/network/udp_header_types.h>
#include <gnuradio/network/udp_sink.h>
#endif

#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/float_to_short.h>
#include <gnuradio/io_signature.h>

#include <iostream>

#include "udp_sink_f.h"


/*
 * Create a new instance of udp_sink_f and return an
 * upcasted shared_ptr. This is effectively the public
 * constructor.
 */
udp_sink_f_sptr make_udp_sink_f()
{
    return gnuradio::get_initial_sptr(new udp_sink_f());
}

static const int MIN_IN = 2;  /*!< Minimum number of input streams. */
static const int MAX_IN = 2;  /*!< Maximum number of input streams. */
static const int MIN_OUT = 0; /*!< Minimum number of output streams. */
static const int MAX_OUT = 0; /*!< Maximum number of output streams. */

udp_sink_f::udp_sink_f()
    : gr::hier_block2("udp_sink_f",
                      gr::io_signature::make(MIN_IN, MAX_IN, sizeof(float)),
                      gr::io_signature::make(MIN_OUT, MAX_OUT, sizeof(float)))
{

    d_f2s = gr::blocks::float_to_short::make(1, 32767);
#if GNURADIO_VERSION < 0x031000
#ifdef GQRX_OS_MACX
    // There seems to be excessive packet loss (even to localhost) on OS X
    // unless the buffer size is limited.
    d_sink = gr::blocks::udp_sink::make(sizeof(short), "localhost", 7355, 512);
#else
    d_sink = gr::blocks::udp_sink::make(sizeof(short), "localhost", 7355);
#endif
    d_sink->disconnect();
#endif

    d_inter = gr::blocks::interleave::make(sizeof(float));
    d_null0 = gr::blocks::null_sink::make(sizeof(float));
    d_null1 = gr::blocks::null_sink::make(sizeof(float));

    connect(self(), 0, d_null0, 0);
    connect(self(), 1, d_null1, 0);
}

udp_sink_f::~udp_sink_f()
{
}

/*! \brief Start streaming through the UDP sink
 *  \param host The hostname or IP address of the client.
 *  \param port The port used for the UDP stream
 *  \param stereo Select mono or stereo streaming
 */
void udp_sink_f::start_streaming(const std::string host, int port, bool stereo)
{
    lock();
    disconnect_all();

    std::cout << "Starting UDP streaming, Host: " << host;
    std::cout << ", Port: " << std::to_string(port) << ", ";
    std::cout << (stereo ? "Stereo" : "Mono") << std::endl;

#if GNURADIO_VERSION >= 0x031000
    d_sink = gr::network::udp_sink::make(sizeof(short), 1, host, port, HEADERTYPE_NONE, 1448, true);
#endif

    if (stereo)
    {
        connect(self(), 0, d_inter, 0);
        connect(self(), 1, d_inter, 1);
        connect(d_inter, 0, d_f2s, 0);
        connect(d_f2s, 0, d_sink, 0);
    }
    else
    {
        connect(self(), 0, d_f2s, 0);
        connect(d_f2s, 0, d_sink, 0);
        connect(self(), 1, d_null0, 0);
    }
    unlock();

#if GNURADIO_VERSION < 0x031000
    d_sink->connect(host, port);
#endif
}


void udp_sink_f::stop_streaming(void)
{
    lock();
    disconnect_all();
    connect(self(), 0, d_null0, 0);
    connect(self(), 1, d_null1, 0);
    unlock();

    std::cout << "Disconnected UDP streaming" << std::endl;

#if GNURADIO_VERSION < 0x031000
    d_sink->disconnect();
#else
    d_sink = nullptr;
#endif
}
