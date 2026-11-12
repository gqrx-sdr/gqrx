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
#include <memory>
#include <mutex>
#include <vector>

#include <gnuradio/gr_complex.h>
#include <gnuradio/sync_block.h>

/*! \brief Lock protected sample queue between two flow graphs.
 *  \ingroup DSP
 *
 * GNU Radio buffers cannot be shared between two top blocks, so the raw I/Q
 * source and the DSP chain (which Gqrx stops and rewires at run time) each run
 * in their own flow graph and are connected through this FIFO.
 *
 * The queue is bounded. If the consumer falls behind, the oldest samples are
 * dropped so that tuning/mode changes do not build up an ever increasing
 * latency.
 */
class iq_bridge_fifo
{
public:
    iq_bridge_fifo(size_t capacity);

    /*! \brief Resize the queue and discard its contents.
     *
     * Both ends access the FIFO through this object, so resizing is safe while
     * the flow graphs are running. It is normally only done when the input
     * sample rate changes.
     */
    void set_capacity(size_t capacity);

    /*! \brief Discard all queued samples. */
    void clear();

    /*! \brief Append samples, dropping the oldest ones if the queue is full. */
    void write(const gr_complex *in, size_t num);

    /*! \brief Read up to \p max samples, waiting up to \p timeout_ms for data.
     *  \return The number of samples written to \p out (may be zero).
     */
    size_t read(gr_complex *out, size_t max, unsigned int timeout_ms);

    /*! \brief Number of samples dropped because the queue was full. */
    uint64_t dropped() const;

private:
    mutable std::mutex      d_mutex;
    std::condition_variable d_not_empty;
    std::vector<gr_complex> d_buf;
    size_t                  d_read;    /*!< Index of the oldest sample. */
    size_t                  d_count;   /*!< Number of queued samples. */
    uint64_t                d_dropped; /*!< Number of samples discarded. */
};

typedef std::shared_ptr<iq_bridge_fifo> iq_bridge_fifo_sptr;

iq_bridge_fifo_sptr make_iq_bridge_fifo(size_t capacity);


class iq_bridge_sink_c;
class iq_bridge_source_c;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<iq_bridge_sink_c> iq_bridge_sink_c_sptr;
typedef boost::shared_ptr<iq_bridge_source_c> iq_bridge_source_c_sptr;
#else
typedef std::shared_ptr<iq_bridge_sink_c> iq_bridge_sink_c_sptr;
typedef std::shared_ptr<iq_bridge_source_c> iq_bridge_source_c_sptr;
#endif

/*! \brief Push samples from the source flow graph into an iq_bridge_fifo. */
iq_bridge_sink_c_sptr make_iq_bridge_sink_c(iq_bridge_fifo_sptr fifo);

/*! \brief Pull samples out of an iq_bridge_fifo into the DSP flow graph. */
iq_bridge_source_c_sptr make_iq_bridge_source_c(iq_bridge_fifo_sptr fifo);


class iq_bridge_sink_c : public gr::sync_block
{
public:
    iq_bridge_sink_c(iq_bridge_fifo_sptr fifo);

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

private:
    iq_bridge_fifo_sptr d_fifo;
};


class iq_bridge_source_c : public gr::sync_block
{
public:
    iq_bridge_source_c(iq_bridge_fifo_sptr fifo);

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

private:
    iq_bridge_fifo_sptr d_fifo;
};

#endif /* IQ_BRIDGE_H */
