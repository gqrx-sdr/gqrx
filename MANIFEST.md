title: gqrx
brief: SDR receiver implemented using GNU Radio and the Qt GUI toolkit
tags:
  - AM
  - FM
  - SSB
  - FFT
author:
  - Alexandru Csete
copyright_owner:
  - Alexandru Csete
dependencies:
  - gnuradio
  - gnuradio-osmosdr
  - Qt5
repo: https://github.com/csete/gqrx.git
stable_release: HEAD
icon:
---

Gqrx is an open source software defined radio (SDR) receiver implemented using GNU Radio and the Qt GUI toolkit. 
Currently it works on Linux and Mac with hardware supported by gr-osmosdr, including Funcube Dongle, RTL-SDR, Airspy, HackRF, BladeRF, RFSpace, USRP and SoapySDR.
Gqrx can operate as an AM/FM/SSB receiver with audio output or as an FFT-only instrument. 
There are also various hooks for interacting with external application using nertwork sockets.

Download:

Gqrx is distributed as source code package and binaries for Linux and Mac. 
Alternate Mac support is available through macports and homebrew.
Please see http://gqrx.dk/download for a list of download resources.

Usage:

It is strongly recommended to run the "volk_profile" gnuradio utility before running gqrx. 
This will detect and enable processor specific optimisations and will in many cases give a significant performance boost.
The first time you start gqrx it will open a device configuration dialog. 
Supported devices that are connected to the computer are discovered automatically and you can select any of them in the drop-down list.
If you don't see your device listed in the drop-down list it could be because:
1. The driver has not been included in a binary distribution
2. The udev rule has not been properly configured
3. Linux kernel driver is blocking access to the device
You can test your device using device specific tools, such as rtl_test, airspy_rx, hackrf_transfer, qthid, etc.
Gqrx supports multiple configurations and sessions if you have several devices or if you want to use the same device under different configurations. 
You can load a configuration from the GUI or using the -c command line argument. 
See "gqrx --help" for a complete list of command line arguments.
Tutorials and howtos are being written and published on the website http://gqrx.dk/
