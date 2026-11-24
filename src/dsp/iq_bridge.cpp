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
#include <algorithm>
#include <chrono>

#include <gnuradio/io_signature.h>

#include "dsp/iq_bridge.h"

/*
 * How long the bridge source waits for new samples before returning zero
 * items. Keeping this short lets the scheduler stop the DSP flow graph
 * promptly, while still avoiding a busy loop.
 */
static const unsigned int READ_TIMEOUT_MS = 20;

iq_bridge_fifo::iq_bridge_fifo(size_t capacity)
    : d_buf(std::max<size_t>(capacity, 1)),
      d_read(0),
      d_count(0),
      d_dropped(0)
{
}

iq_bridge_fifo_sptr make_iq_bridge_fifo(size_t capacity)
{
    return std::make_shared<iq_bridge_fifo>(capacity);
}

void iq_bridge_fifo::set_capacity(size_t capacity)
{
    std::lock_guard<std::mutex> lock(d_mutex);

    capacity = std::max<size_t>(capacity, 1);
    if (capacity == d_buf.size())
        return;

    d_buf.assign(capacity, gr_complex(0.0f, 0.0f));
    d_read = 0;
    d_count = 0;
}

void iq_bridge_fifo::clear()
{
    std::lock_guard<std::mutex> lock(d_mutex);

    d_read = 0;
    d_count = 0;
}

void iq_bridge_fifo::write(const gr_complex *in, size_t num)
{
    {
        std::lock_guard<std::mutex> lock(d_mutex);
        const size_t capacity = d_buf.size();

        /* Keep the newest samples if the chunk itself does not fit. */
        if (num > capacity)
        {
            d_dropped += num - capacity;
            in += num - capacity;
            num = capacity;
        }

        /* Make room by discarding the oldest samples. */
        const size_t free_space = capacity - d_count;
        if (num > free_space)
        {
            const size_t discard = num - free_space;
            d_read = (d_read + discard) % capacity;
            d_count -= discard;
            d_dropped += discard;
        }

        const size_t write = (d_read + d_count) % capacity;
        const size_t first = std::min(num, capacity - write);
        std::copy(in, in + first, d_buf.begin() + write);
        if (num > first)
            std::copy(in + first, in + num, d_buf.begin());

        d_count += num;
    }

    d_not_empty.notify_one();
}

size_t iq_bridge_fifo::read(gr_complex *out, size_t max, unsigned int timeout_ms)
{
    std::unique_lock<std::mutex> lock(d_mutex);

    if (d_count == 0)
    {
        d_not_empty.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                             [this] { return d_count > 0; });
    }

    const size_t capacity = d_buf.size();
    const size_t num = std::min(max, d_count);
    const size_t first = std::min(num, capacity - d_read);

    std::copy(d_buf.begin() + d_read, d_buf.begin() + d_read + first, out);
    if (num > first)
        std::copy(d_buf.begin(), d_buf.begin() + (num - first), out + first);

    d_read = (d_read + num) % capacity;
    d_count -= num;

    return num;
}

uint64_t iq_bridge_fifo::dropped() const
{
    std::lock_guard<std::mutex> lock(d_mutex);

    return d_dropped;
}


/*** iq_bridge_sink_c *******************************************************/

iq_bridge_sink_c_sptr make_iq_bridge_sink_c(iq_bridge_fifo_sptr fifo)
{
    return gnuradio::get_initial_sptr(new iq_bridge_sink_c(fifo));
}

iq_bridge_sink_c::iq_bridge_sink_c(iq_bridge_fifo_sptr fifo)
    : gr::sync_block("iq_bridge_sink_c",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(0, 0, 0)),
      d_fifo(fifo)
{
}

int iq_bridge_sink_c::work(int noutput_items,
                           gr_vector_const_void_star &input_items,
                           gr_vector_void_star &output_items)
{
    (void) output_items;

    d_fifo->write(static_cast<const gr_complex *>(input_items[0]), noutput_items);

    return noutput_items;
}


/*** iq_bridge_source_c *****************************************************/

iq_bridge_source_c_sptr make_iq_bridge_source_c(iq_bridge_fifo_sptr fifo)
{
    return gnuradio::get_initial_sptr(new iq_bridge_source_c(fifo));
}

iq_bridge_source_c::iq_bridge_source_c(iq_bridge_fifo_sptr fifo)
    : gr::sync_block("iq_bridge_source_c",
                     gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_fifo(fifo)
{
}

int iq_bridge_source_c::work(int noutput_items,
                             gr_vector_const_void_star &input_items,
                             gr_vector_void_star &output_items)
{
    (void) input_items;

    gr_complex *out = static_cast<gr_complex *>(output_items[0]);

    return static_cast<int>(d_fifo->read(out, noutput_items, READ_TIMEOUT_MS));
}
