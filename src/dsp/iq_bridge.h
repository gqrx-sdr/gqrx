/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#ifndef IQ_BRIDGE_H
#define IQ_BRIDGE_H

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>
#include <gnuradio/gr_complex.h>
#include <gnuradio/sync_block.h>

/*! \brief Single producer / single consumer I/Q FIFO shared by
 *         iq_bridge_sink_c and iq_bridge_source_c.
 *
 * If the consumer falls behind, the oldest samples are discarded so that
 * the latency stays bounded.
 */
class iq_bridge_fifo
{
public:
    explicit iq_bridge_fifo(size_t capacity);

    void   set_capacity(size_t capacity);
    size_t capacity();
    void   clear();

    void   write(const gr_complex *in, size_t num);
    size_t read(gr_complex *out, size_t max, int timeout_ms);

    uint64_t dropped();

private:
    std::mutex              d_mutex;
    std::condition_variable d_cond;
    std::vector<gr_complex> d_buf;
    size_t                  d_rd;
    size_t                  d_count;
    uint64_t                d_dropped;
};

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<iq_bridge_fifo> iq_bridge_fifo_sptr;
#else
typedef std::shared_ptr<iq_bridge_fifo> iq_bridge_fifo_sptr;
#endif

class iq_bridge_sink_c;
class iq_bridge_source_c;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<iq_bridge_sink_c> iq_bridge_sink_c_sptr;
typedef boost::shared_ptr<iq_bridge_source_c> iq_bridge_source_c_sptr;
#else
typedef std::shared_ptr<iq_bridge_sink_c> iq_bridge_sink_c_sptr;
typedef std::shared_ptr<iq_bridge_source_c> iq_bridge_source_c_sptr;
#endif

iq_bridge_sink_c_sptr make_iq_bridge_sink_c(iq_bridge_fifo_sptr fifo);
iq_bridge_source_c_sptr make_iq_bridge_source_c(iq_bridge_fifo_sptr fifo);

/*! \brief Sink that pushes I/Q samples into an iq_bridge_fifo.
 *  \ingroup DSP
 *
 * Used together with iq_bridge_source_c to pass samples from one top block
 * to another. This lets the hardware source run in its own flow graph so
 * that it keeps streaming while the DSP flow graph is locked or stopped for
 * reconfiguration.
 */
class iq_bridge_sink_c : public gr::sync_block
{
    friend iq_bridge_sink_c_sptr make_iq_bridge_sink_c(iq_bridge_fifo_sptr fifo);

protected:
    explicit iq_bridge_sink_c(iq_bridge_fifo_sptr fifo);

public:
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

private:
    iq_bridge_fifo_sptr d_fifo;
};

/*! \brief Source that pulls I/Q samples from an iq_bridge_fifo.
 *  \ingroup DSP
 */
class iq_bridge_source_c : public gr::sync_block
{
    friend iq_bridge_source_c_sptr make_iq_bridge_source_c(iq_bridge_fifo_sptr fifo);

protected:
    explicit iq_bridge_source_c(iq_bridge_fifo_sptr fifo);

public:
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

private:
    iq_bridge_fifo_sptr d_fifo;
};

#endif /* IQ_BRIDGE_H */
