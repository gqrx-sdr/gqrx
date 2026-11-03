/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2013 Alexandru Csete OZ9AEC.
 * Copyright 2026 Gqrx contributors.
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

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <gnuradio/io_signature.h>
#include <algorithm>
#include <cmath>
#include <cstring>
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

// nc is used widely for receiving UDP streams and some versions of nc,
// notably on MacOS, use a 1024 byte buffer so we need to make sure we
// don't send packets that are larger than that, otherwise data will be
// lost.
static const int PAYLOAD_SIZE = 1024;

udp_sink_f::udp_sink_f()
    : gr::sync_block("udp_sink_f",
                     gr::io_signature::make(MIN_IN, MAX_IN, sizeof(float)),
                     gr::io_signature::make(MIN_OUT, MAX_OUT, 0)),
      d_streaming(false),
      d_stereo(false),
      d_sock(-1),
      d_sockaddr_len(0)
{
    std::memset(d_sockaddr, 0, sizeof(d_sockaddr));
}

udp_sink_f::~udp_sink_f()
{
    stop_streaming();
}

/*! \brief Start streaming through the UDP sink
 *  \param host The hostname or IP address of the client.
 *  \param port The port used for the UDP stream
 *  \param stereo Select mono or stereo streaming
 */
void udp_sink_f::start_streaming(const std::string host, int port, bool stereo)
{
    if (host.empty() || port <= 0 || port > 65535)
    {
        std::cerr << "UDP sink: Invalid host or port: " << host << ":" << port << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(d_mutex);

    if (d_sock >= 0)
    {
#ifdef _WIN32
        closesocket(d_sock);
#else
        close(d_sock);
#endif
        d_sock = -1;
    }
    d_buffer.clear();

    struct addrinfo hints;
    struct addrinfo *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    std::string port_str = std::to_string(port);
    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0 || !res)
    {
        std::cerr << "UDP sink: Failed to resolve " << host << ":" << port << std::endl;
        return;
    }

    d_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (d_sock < 0)
    {
        std::cerr << "UDP sink: Failed to create socket" << std::endl;
        freeaddrinfo(res);
        return;
    }

    if (res->ai_addrlen <= sizeof(d_sockaddr))
    {
        std::memcpy(d_sockaddr, res->ai_addr, res->ai_addrlen);
        d_sockaddr_len = res->ai_addrlen;
    }
    freeaddrinfo(res);

    d_stereo = stereo;
    d_streaming = true;

    std::cout << "Starting UDP streaming, Host: " << host;
    std::cout << ", Port: " << port_str << ", ";
    std::cout << (stereo ? "Stereo" : "Mono") << std::endl;
}

void udp_sink_f::stop_streaming(void)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    if (!d_streaming && d_sock < 0)
        return;

    d_streaming = false;

    if (d_sock >= 0)
    {
        if (!d_buffer.empty())
        {
            sendto(d_sock, reinterpret_cast<const char *>(d_buffer.data()),
                   static_cast<int>(d_buffer.size() * sizeof(int16_t)), 0,
                   reinterpret_cast<const struct sockaddr *>(d_sockaddr),
                   static_cast<socklen_t>(d_sockaddr_len));
            d_buffer.clear();
        }
#ifdef _WIN32
        closesocket(d_sock);
#else
        close(d_sock);
#endif
        d_sock = -1;
    }

    std::cout << "Disconnected UDP streaming" << std::endl;
}

int udp_sink_f::work(int noutput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items)
{
    (void)output_items;
    const float *in0 = static_cast<const float *>(input_items[0]);
    const float *in1 = static_cast<const float *>(input_items[1]);

    std::lock_guard<std::mutex> lock(d_mutex);
    if (!d_streaming || d_sock < 0)
        return noutput_items;

    const size_t max_samples = PAYLOAD_SIZE / sizeof(int16_t);

    for (int i = 0; i < noutput_items; ++i)
    {
        float s0 = in0[i];
        if (std::isnan(s0) || std::isinf(s0))
            s0 = 0.0f;
        s0 = std::clamp(s0 * 32767.0f, -32768.0f, 32767.0f);
        d_buffer.push_back(static_cast<int16_t>(std::round(s0)));

        if (d_stereo)
        {
            float s1 = in1[i];
            if (std::isnan(s1) || std::isinf(s1))
                s1 = 0.0f;
            s1 = std::clamp(s1 * 32767.0f, -32768.0f, 32767.0f);
            d_buffer.push_back(static_cast<int16_t>(std::round(s1)));
        }

        if (d_buffer.size() >= max_samples)
        {
            sendto(d_sock, reinterpret_cast<const char *>(d_buffer.data()),
                   static_cast<int>(d_buffer.size() * sizeof(int16_t)), 0,
                   reinterpret_cast<const struct sockaddr *>(d_sockaddr),
                   static_cast<socklen_t>(d_sockaddr_len));
            d_buffer.clear();
        }
    }

    return noutput_items;
}
