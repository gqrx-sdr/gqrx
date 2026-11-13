/*
 * Checks for tx_devstr_names_sink_driver(), which decides whether the TX
 * probe may hand a device string to osmosdr::sink::make(). Needs no GNU
 * Radio, Qt or SDR hardware.
 */
#undef NDEBUG
#include <cassert>
#include <iostream>
#include <string>

#include "applications/gqrx/tx_device_args.h"

static void expect(const std::string &devstr, bool expected)
{
    const bool got = tx_devstr_names_sink_driver(devstr);
    if (got != expected)
        std::cerr << "FAIL: \"" << devstr << "\" -> " << got << ", expected " << expected << std::endl;
    assert(got == expected);
}

int main()
{
    // Empty: nothing to probe.
    expect("", false);
    expect("   ", false);

    // Receive-only gr-osmosdr drivers. osmosdr::sink::make() would fall back
    // to the first transmit-capable device it finds for any of these.
    expect("rtl=0", false);
    expect("rtl=00000001", false);
    expect("rtl=0,bias=1", false);
    expect("rtl_tcp=127.0.0.1:1234", false);
    expect("airspy=0", false);
    expect("airspyhf=0", false);
    expect("sdrplay=0", false);
    expect("fcd,type=2,device='FUNcube Dongle V2.0'", false);
    expect("rfspace=192.168.1.10:50000", false);
    expect("netsdr=192.168.1.10", false);

    // Driver names inside values or labels do not count; only keys do.
    expect("rtl=0,label='HackRF One'", false);
    expect("label=hackrf", false);
    expect("rtl=0,serial=uhd", false);

    // Transmit-capable gr-osmosdr sink drivers.
    expect("hackrf=0", true);
    expect("hackrf=a1b2c3", true);
    expect("hackrf", true);
    expect("uhd", true);
    expect("uhd,serial=30F1234,type=b200", true);
    expect("bladerf=0", true);
    expect("redpitaya=192.168.1.100:1001", true);
    expect("freesrp=0", true);
    expect("xtrx=0", true);
    expect("soapy=0,driver=plutosdr", true);
    expect("soapy=0,driver=lime", true);

    // CIoConfig::escapeDevstr() wraps strings that contain spaces in quotes.
    expect("'soapy=0,driver=plutosdr,label=PlutoSDR #0 usb:1.2.5'", true);
    expect("'rtl=0,label=Generic RTL2832U OEM'", false);

    // file= devices: gr-osmosdr's file sink would open and truncate the file.
    expect("file=/tmp/rec.raw,rate=1e6,freq=100e6,throttle=true,repeat=false", false);
    expect("hackrf=0,file=/tmp/rec.raw", false);

    // Several devices: every one must name a sink driver.
    expect("hackrf=0 bladerf=0", true);
    expect("rtl=0 hackrf=0", false);
    expect("numchan=1 hackrf=0", true);
    expect("numchan=1 rtl=0", false);
    expect("hackrf=0  ", true);

    // Strings gr-osmosdr's own parser would reject.
    expect("hackrf=0\\q", false);
    expect("hackrf=0\\", false);

    std::cout << "test_tx_device_args passed." << std::endl;
    return 0;
}
