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
#ifndef UDP_SINK_F_H
#define UDP_SINK_F_H

#include <gnuradio/sync_block.h>
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>

class udp_sink_f;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<udp_sink_f> udp_sink_f_sptr;
#else
typedef std::shared_ptr<udp_sink_f> udp_sink_f_sptr;
#endif

udp_sink_f_sptr make_udp_sink_f(void);

class udp_sink_f : public gr::sync_block
{
public:
    udp_sink_f(void);
    ~udp_sink_f() override;

    void start_streaming(const std::string host, int port, bool stereo);
    void stop_streaming(void);

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

private:
    std::mutex d_mutex;
    bool d_streaming;
    bool d_stereo;
    int d_sock;
    uint8_t d_sockaddr[128];
    size_t d_sockaddr_len;
    std::vector<int16_t> d_buffer;
};


#endif // UDP_SINK_F_H
