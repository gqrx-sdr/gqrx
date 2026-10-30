#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include "dsp/tx_modulator.h"
#include "applications/gqrx/transmitter.h"
#include "applications/gqrx/remote_control.h"

static void test_tx_mod_am()
{
    std::cout << "Running test_tx_mod_am..." << std::endl;

    tx_mod_am_sptr mod = make_tx_mod_am(0.9f);
    assert(std::abs(mod->mod_index() - 0.9f) < 1e-4f);

    mod->set_mod_index(0.8f);
    assert(std::abs(mod->mod_index() - 0.8f) < 1e-4f);

    // Test clipping of mod_index
    mod->set_mod_index(-0.5f);
    assert(mod->mod_index() >= 0.0f);
    mod->set_mod_index(1.5f);
    assert(mod->mod_index() <= 1.0f);

    mod->set_mod_index(0.9f);

    // Test work function
    const int N = 5;
    float in_data[N] = {0.0f, 1.0f, -1.0f, 2.5f, -3.0f};
    gr_complex out_data[N];

    const void *input_items[1] = { in_data };
    void *output_items[1] = { out_data };
    gr_vector_const_void_star inputs(input_items, input_items + 1);
    gr_vector_void_star outputs(output_items, output_items + 1);

    int ret = mod->work(N, inputs, outputs);
    assert(ret == N);
    (void)ret;

    // At in=0: env = 0.5 * (1 + 0) = 0.5
    assert(std::abs(out_data[0].real() - 0.5f) < 1e-4f);
    assert(std::abs(out_data[0].imag()) < 1e-4f);

    // At in=1: env = 0.5 * (1 + 0.9) = 0.95
    assert(std::abs(out_data[1].real() - 0.95f) < 1e-4f);

    // At in=-1: env = 0.5 * (1 - 0.9) = 0.05
    assert(std::abs(out_data[2].real() - 0.05f) < 1e-4f);

    // Clamped inputs: 2.5 should clamp to 1.0 -> 0.95
    assert(std::abs(out_data[3].real() - 0.95f) < 1e-4f);

    // Clamped inputs: -3.0 should clamp to -1.0 -> 0.05
    assert(std::abs(out_data[4].real() - 0.05f) < 1e-4f);

    std::cout << "test_tx_mod_am passed." << std::endl;
}

static void test_tx_modulator_modes()
{
    std::cout << "Running test_tx_modulator_modes..." << std::endl;

    double audio_rate = 48000.0;
    tx_modulator_sptr mod = make_tx_modulator(audio_rate, TX_MOD_NFM);
    assert(mod->mode() == TX_MOD_NFM);

    // Test mic gain
    mod->set_mic_gain(6.0f);
    assert(std::abs(mod->mic_gain() - 6.0f) < 1e-4f);

    // Test FM deviation
    assert(std::abs(mod->fm_maxdev() - 5000.0f) < 1e-4f);
    mod->set_fm_maxdev(2500.0f);
    assert(std::abs(mod->fm_maxdev() - 2500.0f) < 1e-4f);

    // Test CTCSS tone
    mod->set_ctcss_freq(88.5f);
    assert(std::abs(mod->ctcss_freq() - 88.5f) < 1e-4f);
    mod->set_ctcss_freq(0.0f);
    assert(std::abs(mod->ctcss_freq()) < 1e-4f);

    // Test dynamic mode switching
    mod->set_mode(TX_MOD_AM);
    assert(mod->mode() == TX_MOD_AM);

    mod->set_mode(TX_MOD_USB);
    assert(mod->mode() == TX_MOD_USB);

    mod->set_mode(TX_MOD_LSB);
    assert(mod->mode() == TX_MOD_LSB);

    mod->set_mode(TX_MOD_NFM);
    assert(mod->mode() == TX_MOD_NFM);

    std::cout << "test_tx_modulator_modes passed." << std::endl;
}

static void test_remote_control_rigctl()
{
    std::cout << "Running test_remote_control_rigctl..." << std::endl;

    RemoteControl rc;

    // Test initial PTT state
    QString res = rc.processCommand("t");
    assert(res == "0\n");

    res = rc.processCommand("\\get_ptt");
    assert(res == "0\n");

    // Test setting PTT ON
    res = rc.processCommand("T 1");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("t");
    assert(res == "1\n");

    // Test setting PTT OFF
    res = rc.processCommand("\\set_ptt 0");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("t");
    assert(res == "0\n");

    // Test PTT invalid arguments
    res = rc.processCommand("T");
    assert(res == "RPRT 1\n");

    res = rc.processCommand("T invalid");
    assert(res == "RPRT 1\n");

    res = rc.processCommand("T ?");
    assert(res == "0 1\n");

    // Test func PTT
    res = rc.processCommand("u PTT");
    assert(res == "0\n");

    res = rc.processCommand("U PTT 1");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("u PTT");
    assert(res == "1\n");

    res = rc.processCommand("U PTT 0");
    assert(res == "RPRT 0\n");

    // Test Split VFO
    res = rc.processCommand("s");
    assert(res == "0\nVFOA\n");

    res = rc.processCommand("S 1 VFOB");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("s");
    assert(res == "1\nVFOB\n");

    res = rc.processCommand("S 0 VFOA");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("s");
    assert(res == "0\nVFOA\n");

    res = rc.processCommand("S invalid");
    assert(res == "RPRT 1\n");

    res = rc.processCommand("S ?");
    assert(res == "0 1\n");

    // Test func SPLIT
    res = rc.processCommand("u SPLIT");
    assert(res == "0\n");

    res = rc.processCommand("U SPLIT 1");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("u SPLIT");
    assert(res == "1\n");

    res = rc.processCommand("U SPLIT 0");
    assert(res == "RPRT 0\n");

    // Test Split Frequency
    res = rc.processCommand("I 145600000");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("i");
    assert(res == "145600000\n");

    res = rc.processCommand("\\get_split_freq");
    assert(res == "145600000\n");

    res = rc.processCommand("I -100");
    assert(res == "RPRT 1\n");

    res = rc.processCommand("I notanumber");
    assert(res == "RPRT 1\n");

    // Test Levels: RFPOWER and MICGAIN
    res = rc.processCommand("l RFPOWER");
    assert(res == "0.0\n");

    res = rc.processCommand("L RFPOWER 25.0");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("l RFPOWER");
    assert(res == "25.0\n");

    res = rc.processCommand("l MICGAIN");
    assert(res == "0.0\n");

    res = rc.processCommand("L MICGAIN 4.5");
    assert(res == "RPRT 0\n");

    res = rc.processCommand("l MICGAIN");
    assert(res == "4.5\n");

    res = rc.processCommand("L RFPOWER invalid");
    assert(res == "RPRT 1\n");

    res = rc.processCommand("L MICGAIN invalid");
    assert(res == "RPRT 1\n");

    // Test level and func query lists
    res = rc.processCommand("l ?");
    assert(res.contains("RFPOWER"));
    assert(res.contains("MICGAIN"));

    res = rc.processCommand("L ?");
    assert(res.contains("RFPOWER"));
    assert(res.contains("MICGAIN"));

    res = rc.processCommand("u ?");
    assert(res.contains("PTT"));
    assert(res.contains("SPLIT"));

    res = rc.processCommand("U ?");
    assert(res.contains("PTT"));
    assert(res.contains("SPLIT"));

    // Test dump_state
    res = rc.processCommand("\\dump_state");
    assert(res.contains("10000000000.000000"));

    std::cout << "test_remote_control_rigctl passed." << std::endl;
}

static void test_transmitter_lifecycle()
{
    std::cout << "Running test_transmitter_lifecycle..." << std::endl;

    transmitter tx("", "", 2000000.0);

    // Initial state
    assert(!tx.is_transmitting());

    // Frequency
    tx.set_tx_freq(145500000.0);
    assert(std::abs(tx.get_tx_freq() - 145500000.0) < 1e-4);

    // Mode
    tx.set_tx_mode(TX_MOD_AM);
    assert(tx.get_tx_mode() == TX_MOD_AM);
    assert(tx.get_tx_mode_str() == "AM");

    tx.set_tx_mode(TX_MOD_USB);
    assert(tx.get_tx_mode_str() == "USB");

    tx.set_tx_mode(TX_MOD_LSB);
    assert(tx.get_tx_mode_str() == "LSB");

    tx.set_tx_mode(TX_MOD_NFM);
    assert(tx.get_tx_mode_str() == "NFM");

    // Power & Mic gain
    tx.set_tx_power(18.0);
    assert(std::abs(tx.get_tx_power() - 18.0) < 1e-4);

    tx.set_mic_gain(3.5f);
    assert(std::abs(tx.get_mic_gain() - 3.5f) < 1e-4f);

    // Split
    tx.set_split_mode(true);
    assert(tx.get_split_mode());
    tx.set_split_offset(600000.0);
    assert(std::abs(tx.get_split_offset() - 600000.0) < 1e-4);

    // CTCSS
    tx.set_ctcss_freq(123.0f);
    assert(std::abs(tx.get_ctcss_freq() - 123.0f) < 1e-4f);

    // Status details formatting
    std::string details = tx.get_status_details();
    assert(details.find("146.1000 MHz") != std::string::npos);
    assert(details.find("NFM") != std::string::npos);
    assert(details.find("Pwr: 18.0 dB") != std::string::npos);
    assert(details.find("Mic: 3.5 dB") != std::string::npos);
    assert(details.find("Split: 600.0 kHz") != std::string::npos);
    assert(details.find("CTCSS: 123.0 Hz") != std::string::npos);

    // Start & Stop (with fallback null sink)
    transmitter::status s = tx.start();
    assert(s == transmitter::STATUS_OK);
    (void)s;
    assert(tx.is_transmitting());

    s = tx.stop();
    assert(s == transmitter::STATUS_OK);
    assert(!tx.is_transmitting());

    std::cout << "test_transmitter_lifecycle passed." << std::endl;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    test_tx_mod_am();
    test_tx_modulator_modes();
    test_remote_control_rigctl();
    test_transmitter_lifecycle();

    std::cout << "\nALL TRANSMITTER AND RIGCTL TESTS PASSED!" << std::endl;
    return 0;
}
