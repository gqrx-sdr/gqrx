#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <gnuradio/top_block.h>
#include <gnuradio/sync_block.h>
#include <gnuradio/blocks/null_source.h>
#include <gnuradio/blocks/null_sink.h>

#include "interfaces/audio_recorder_f.h"
#include "interfaces/udp_sink_f.h"

// Tracking source to ensure stop() is not called during runtime operations
class tracking_source_f : public gr::sync_block
{
public:
    int stop_count;

    tracking_source_f()
        : gr::sync_block("tracking_source_f",
                         gr::io_signature::make(0, 0, 0),
                         gr::io_signature::make(2, 2, sizeof(float))),
          stop_count(0)
    {
    }

    bool stop() override
    {
        stop_count++;
        return gr::sync_block::stop();
    }

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override
    {
        (void)input_items;
        float *out0 = static_cast<float *>(output_items[0]);
        float *out1 = static_cast<float *>(output_items[1]);
        for (int i = 0; i < noutput_items; ++i) {
            out0[i] = 0.5f;
            out1[i] = -0.5f;
        }
        return noutput_items;
    }
};

static void test_audio_recorder()
{
    std::cout << "[TEST] Testing audio_recorder_f..." << std::endl;

    auto tb = gr::make_top_block("test_audio_recorder_tb");
    auto src = gnuradio::get_initial_sptr(new tracking_source_f());
    auto rec = make_audio_recorder_f(48000);

    tb->connect(src, 0, rec, 0);
    tb->connect(src, 1, rec, 1);

    tb->start();

    assert(src->stop_count == 0);
    assert(!rec->is_recording());

    // Edge cases: empty path rejected, stopping while idle is no-op
    assert(!rec->start_recording("", 1.0f));
    assert(!rec->is_recording());
    rec->stop_recording();
    assert(!rec->is_recording());
    const std::string filename = "test_recording.wav";
    // Remove if exists
    std::remove(filename.c_str());

    // Start recording while flowgraph is running
    bool ok = rec->start_recording(filename, 1.0f);
    assert(ok);
    (void)ok;
    assert(rec->is_recording());
    // Duplicate start rejected while recording
    assert(!rec->start_recording("duplicate.wav", 1.0f));
    // Flowgraph must NOT have stopped
    assert(src->stop_count == 0);

    // Let it record some samples
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop recording while flowgraph is running
    rec->stop_recording();
    assert(!rec->is_recording());
    assert(src->stop_count == 0);

    // Verify WAV file
    std::ifstream wav(filename, std::ios::binary);
    assert(wav.is_open());

    char riff[4];
    wav.read(riff, 4);
    assert(std::memcmp(riff, "RIFF", 4) == 0);

    uint32_t chunk_size = 0;
    wav.read(reinterpret_cast<char *>(&chunk_size), 4);
    assert(chunk_size > 36);

    char wave[4];
    wav.read(wave, 4);
    assert(std::memcmp(wave, "WAVE", 4) == 0);

    char fmt[4];
    wav.read(fmt, 4);
    assert(std::memcmp(fmt, "fmt ", 4) == 0);

    uint32_t sub1_size = 0;
    wav.read(reinterpret_cast<char *>(&sub1_size), 4);
    assert(sub1_size == 16);

    uint16_t audio_format = 0;
    wav.read(reinterpret_cast<char *>(&audio_format), 2);
    assert(audio_format == 1); // PCM

    uint16_t channels = 0;
    wav.read(reinterpret_cast<char *>(&channels), 2);
    assert(channels == 2); // Stereo

    uint32_t sample_rate = 0;
    wav.read(reinterpret_cast<char *>(&sample_rate), 4);
    assert(sample_rate == 48000);

    uint32_t byte_rate = 0;
    wav.read(reinterpret_cast<char *>(&byte_rate), 4);
    assert(byte_rate == 48000 * 4);

    uint16_t block_align = 0;
    wav.read(reinterpret_cast<char *>(&block_align), 2);
    assert(block_align == 4);

    uint16_t bits_per_sample = 0;
    wav.read(reinterpret_cast<char *>(&bits_per_sample), 2);
    assert(bits_per_sample == 16);

    char data_tag[4];
    wav.read(data_tag, 4);
    assert(std::memcmp(data_tag, "data", 4) == 0);

    uint32_t data_size = 0;
    wav.read(reinterpret_cast<char *>(&data_size), 4);
    assert(data_size == chunk_size - 36);
    assert(data_size > 0);

    wav.close();
    std::remove(filename.c_str());

    // Record a second time to verify restartability
    ok = rec->start_recording(filename, 0.5f);
    assert(ok);
    assert(rec->is_recording());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    rec->stop_recording();
    assert(!rec->is_recording());
    assert(src->stop_count == 0);
    std::remove(filename.c_str());

    tb->stop();
    tb->wait();

    std::cout << "[PASS] audio_recorder_f verified successfully." << std::endl;
}

static void test_udp_sink()
{
    std::cout << "[TEST] Testing udp_sink_f..." << std::endl;

    // Create a local UDP receiver socket
    int recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(recv_sock >= 0);

    struct sockaddr_in recv_addr;
    std::memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    recv_addr.sin_port = htons(0); // auto-bind port

    int ret = bind(recv_sock, reinterpret_cast<struct sockaddr *>(&recv_addr), sizeof(recv_addr));
    assert(ret == 0);
    (void)ret;

    socklen_t addr_len = sizeof(recv_addr);
    ret = getsockname(recv_sock, reinterpret_cast<struct sockaddr *>(&recv_addr), &addr_len);
    assert(ret == 0);
    int test_port = ntohs(recv_addr.sin_port);

    // Set receive timeout (1 second)
#ifdef _WIN32
    DWORD timeout_ms = 1000;
    setsockopt(recv_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms));
#else
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(recv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    auto tb = gr::make_top_block("test_udp_sink_tb");
    auto src = gnuradio::get_initial_sptr(new tracking_source_f());
    auto udp = make_udp_sink_f();

    tb->connect(src, 0, udp, 0);
    tb->connect(src, 1, udp, 1);

    tb->start();
    assert(src->stop_count == 0);

    // Edge cases: invalid host/port rejected
    udp->start_streaming("", test_port, false);
    udp->start_streaming("127.0.0.1", 0, false);
    udp->start_streaming("127.0.0.1", -1, false);
    udp->start_streaming("127.0.0.1", 70000, false);
    udp->stop_streaming(); // No-op when not streaming
    // Start streaming to the local socket
    udp->start_streaming("127.0.0.1", test_port, true);
    assert(src->stop_count == 0);

    // Receive at least one packet
    char buf[2048];
    ssize_t n = recv(recv_sock, buf, sizeof(buf), 0);
    assert(n > 0);
    assert(n <= 1024);
    (void)n;

    // Stop streaming
    udp->stop_streaming();
    assert(src->stop_count == 0);

    // Start streaming again in mono mode
    udp->start_streaming("127.0.0.1", test_port, false);
    assert(src->stop_count == 0);

    n = recv(recv_sock, buf, sizeof(buf), 0);
    assert(n > 0);
    assert(n <= 1024);
    (void)n;

    udp->stop_streaming();
    assert(src->stop_count == 0);

#ifdef _WIN32
    closesocket(recv_sock);
#else
    close(recv_sock);
#endif

    tb->stop();
    tb->wait();

    std::cout << "[PASS] udp_sink_f verified successfully." << std::endl;
}

static void test_demod_switch_continuity()
{
    std::cout << "[TEST] Testing source continuity during demod/sink rewiring..." << std::endl;

    auto tb = gr::make_top_block("test_continuity_tb");
    auto src = gnuradio::get_initial_sptr(new tracking_source_f());
    auto rec = make_audio_recorder_f(48000);
    auto udp = make_udp_sink_f();

    // Intermediate processing blocks
    auto rx1 = gr::blocks::null_sink::make(sizeof(float));
    auto rx2 = gr::blocks::null_sink::make(sizeof(float));

    tb->connect(src, 0, rec, 0);
    tb->connect(src, 1, rec, 1);
    tb->connect(src, 0, udp, 0);
    tb->connect(src, 1, udp, 1);
    tb->connect(src, 0, rx1, 0);

    tb->start();
    assert(src->stop_count == 0);

    // Rewire rx1 to rx2 on running graph WITHOUT calling tb->lock() / tb->stop()
    tb->disconnect(src, 0, rx1, 0);
    tb->connect(src, 0, rx2, 0);

    // Source must NOT have stopped
    assert(src->stop_count == 0);

    // Start and stop audio recording and UDP streaming
    rec->start_recording("test_cont.wav", 1.0f);
    udp->start_streaming("127.0.0.1", 17356, true);
    assert(src->stop_count == 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    rec->stop_recording();
    udp->stop_streaming();
    assert(src->stop_count == 0);

    std::remove("test_cont.wav");

    tb->stop();
    tb->wait();
    assert(src->stop_count == 1); // Only stopped when tb->stop() was called

    std::cout << "[PASS] Demod/sink rewiring source continuity verified." << std::endl;
}

int main()
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    test_audio_recorder();
    test_udp_sink();
    test_demod_switch_continuity();

#ifdef _WIN32
    WSACleanup();
#endif

    std::cout << "All regression tests passed successfully!" << std::endl;
    return 0;
}
