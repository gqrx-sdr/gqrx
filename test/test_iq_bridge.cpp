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

/*
 * Tests for the I/Q bridge used to run the input device in its own flow
 * graph (https://github.com/gqrx-sdr/gqrx/issues/1445).
 *
 * The flow graph test uses a source whose stop() throws, like
 * SoapyRemoteDevice::deactivateStream() does when the control link is
 * reset. If locking or restarting the DSP flow graph ever stops the source
 * again, the test aborts the same way Gqrx did.
 */
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gnuradio/blocks/throttle.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/top_block.h>

#include "dsp/iq_bridge.h"

static int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,    \
                         __LINE__, #cond);                                 \
            failures++;                                                    \
        }                                                                  \
    } while (0)

/* Emits 0, 1, 2, ... and counts start()/stop() calls. */
class counting_source : public gr::sync_block
{
public:
    std::atomic<int> starts{0};
    std::atomic<int> stops{0};
    std::atomic<bool> throw_on_stop{false};

    counting_source()
        : gr::sync_block("counting_source",
                         gr::io_signature::make(0, 0, 0),
                         gr::io_signature::make(1, 1, sizeof(gr_complex))),
          d_next(0)
    {
    }

    bool start() override { starts++; return true; }

    bool stop() override
    {
        stops++;
        if (throw_on_stop)
            throw std::runtime_error("SoapyRPCUnpacker::recv(header) FAIL: recv() [104: Connection reset by peer]");
        return true;
    }

    int work(int noutput_items, gr_vector_const_void_star &,
             gr_vector_void_star &output_items) override
    {
        gr_complex *out = (gr_complex *)output_items[0];
        for (int i = 0; i < noutput_items; i++)
            out[i] = gr_complex((float)(d_next++ % 1000000), 0.0f);
        return noutput_items;
    }

private:
    uint64_t d_next;
};

/* Checks that samples arrive in order without gaps. */
class sequence_sink : public gr::sync_block
{
public:
    std::atomic<uint64_t> received{0};
    std::atomic<uint64_t> gaps{0};   // number of discontinuities

    sequence_sink()
        : gr::sync_block("sequence_sink",
                         gr::io_signature::make(1, 1, sizeof(gr_complex)),
                         gr::io_signature::make(0, 0, 0)),
          d_expected(-1)
    {
    }

    int work(int noutput_items, gr_vector_const_void_star &input_items,
             gr_vector_void_star &) override
    {
        const gr_complex *in = (const gr_complex *)input_items[0];
        for (int i = 0; i < noutput_items; i++)
        {
            long v = (long)in[i].real();
            if (d_expected >= 0 && v != d_expected)
                gaps++;
            d_expected = (v + 1) % 1000000;
        }
        received += noutput_items;
        return noutput_items;
    }

private:
    long d_expected;
};

static void test_fifo()
{
    iq_bridge_fifo fifo(8);
    std::vector<gr_complex> in(6), out(16);
    for (int i = 0; i < 6; i++)
        in[i] = gr_complex((float)i, 0.0f);

    // empty FIFO times out with zero items
    CHECK(fifo.read(out.data(), out.size(), 1) == 0);

    // in-order delivery with wrap-around
    fifo.write(in.data(), 6);
    CHECK(fifo.read(out.data(), 4, 0) == 4);
    CHECK(out[0].real() == 0.0f && out[3].real() == 3.0f);
    fifo.write(in.data(), 6);       // wraps
    CHECK(fifo.read(out.data(), out.size(), 0) == 8);
    CHECK(out[0].real() == 4.0f && out[1].real() == 5.0f);
    CHECK(out[2].real() == 0.0f && out[7].real() == 5.0f);
    CHECK(fifo.dropped() == 0);

    // overflow keeps the newest samples
    fifo.write(in.data(), 6);
    fifo.write(in.data(), 6);
    CHECK(fifo.dropped() == 4);
    CHECK(fifo.read(out.data(), out.size(), 0) == 8);
    CHECK(out[0].real() == 4.0f && out[2].real() == 0.0f && out[7].real() == 5.0f);

    // writing more than the capacity at once
    std::vector<gr_complex> big(20);
    for (int i = 0; i < 20; i++)
        big[i] = gr_complex((float)i, 0.0f);
    fifo.write(big.data(), big.size());
    CHECK(fifo.read(out.data(), out.size(), 0) == 8);
    CHECK(out[0].real() == 12.0f && out[7].real() == 19.0f);

    // clear() and set_capacity() discard buffered data
    fifo.write(in.data(), 3);
    fifo.clear();
    CHECK(fifo.read(out.data(), out.size(), 0) == 0);
    fifo.write(in.data(), 3);
    fifo.set_capacity(4);
    CHECK(fifo.capacity() == 4);
    CHECK(fifo.read(out.data(), out.size(), 0) == 0);
}

static void test_dsp_reconfig_keeps_source_running()
{
    const double rate = 1e6;

    gr::top_block_sptr src_tb = gr::make_top_block("test_input");
    gr::top_block_sptr dsp_tb = gr::make_top_block("test_dsp");

    iq_bridge_fifo_sptr fifo(new iq_bridge_fifo((size_t)rate));
    auto bridge_snk = make_iq_bridge_sink_c(fifo);
    auto bridge_src = make_iq_bridge_source_c(fifo);

    auto src = gnuradio::get_initial_sptr(new counting_source());
    auto throttle = gr::blocks::throttle::make(sizeof(gr_complex), rate);
    auto snk = gnuradio::get_initial_sptr(new sequence_sink());

    src_tb->connect(src, 0, throttle, 0);
    src_tb->connect(throttle, 0, bridge_snk, 0);
    dsp_tb->connect(bridge_src, 0, snk, 0);

    src_tb->start();
    dsp_tb->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // From here on, stopping the source would abort the test
    src->throw_on_stop = true;

    // What Gqrx does on demod change, recording, UDP, filter changes, ...
    for (int i = 0; i < 20; i++)
    {
        dsp_tb->lock();
        dsp_tb->unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    for (int i = 0; i < 5; i++)
    {
        dsp_tb->stop();
        dsp_tb->wait();
        dsp_tb->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(src->starts == 1);
    CHECK(src->stops == 0);
    CHECK(snk->received > 0);
    // Samples that were in flight inside the DSP flow graph are lost on each
    // restart, but the device stream itself must be continuous.
    CHECK(snk->gaps <= 25);
    CHECK(fifo->dropped() == 0);

    uint64_t before = snk->received;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(snk->received > before);   // still streaming

    src->throw_on_stop = false;
    dsp_tb->stop();
    dsp_tb->wait();
    src_tb->stop();
    src_tb->wait();
    CHECK(src->stops == 1);
}

int main()
{
    test_fifo();
    test_dsp_reconfig_keeps_source_running();

    if (failures)
    {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("All iq_bridge tests passed\n");
    return EXIT_SUCCESS;
}
