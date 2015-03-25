Gqrx
====

Gqrx is an experimental software defined radio receiver implemented using GNU Radio and the Qt GUI toolkit. Currently it works on Linux and Mac and supports the following devices:
- Funcube Dongle Pro and Pro+
- RTL2832U-based DVB-T dongles (rtlsdr via USB and TCP)
- OsmoSDR
- USRP
- HackRF Jawbreaker
- Nuand bladeRF
- RFspace SDR-IQ, SDR-IP and NetSDR
- Airspy
- any other device supported by the gr-osmosdr library

Gqrx can operate as a traditional AM/FM/SSB receiver with audio output or as an FFT-only instrument.


Download
--------

Gqrx is distributed as source code package and binaries for Linux and Mac. Please see http://gqrx.dk/download for a list of official and third party download resources.


Usage
-----

The first time you start gqrx it will open a device configuration dialog. Supported devices that are connected to the computer are discovered automatically and you can select any of them in the drop-down list.

If you don't see your device listed in the drop-down list it could be because:
- The driver has not included in a binary distribution
- The udev rule has not been properly configured

You can test your device first with rtl_test, qthid, or uhd_usrp_probe that come with the respective packages.

Gqrx supports multiple configurations and sessions if you have several devices or if you want to use the same device under different configurations. You can load a configuration from the GUI or using the -c command line argument. See "gqrx --help" for a complete list of command line arguments.

Tutorials and howtos are being written and published on the website
http://gqrx.dk/


Known problems
--------------

See the bug tracker on Github: https://github.com/csete/gqrx/issues


Getting help and reporting bugs
-------------------------------

There is now a Google group for discussing anything related to Gqrx: https://groups.google.com/forum/#!forum/gqrx
This includes getting help with installation and troubleshooting. Please remember to provide detailed description of your problem, your setup, what steps you followed, etc.


Installation from source
------------------------

The source code is hosted on Github: https://github.com/csete/gqrx

To compile gqrx from source you need the following dependencies:
- GNU Radio 3.7 with the following components:
    - gnuradio-runtime
    - gnuradio-analog
    - gnuradio-digital
    - gnuradio-blocks
    - gnuradio-filter
    - gnuradio-fft
    - gnuradio-audio
- The gr-iqbalance library (optional)
- At least one of:
    - Funcube Dongle Pro driver via gnuradio-fcd
    - UHD driver via gnuradio-uhd
    - Funcube Dongle Pro+ driver from https://github.com/dl1ksv/gr-fcdproplus
    - RTL-SDR driver from http://cgit.osmocom.org/cgit/rtl-sdr/
    - OsmoSDR driver from http://cgit.osmocom.org/cgit/osmo-sdr/
    - HackRF Jawbreaker driver from http://greatscottgadgets.com/hackrf/
- gnuradio-osmosdr from http://cgit.osmocom.org/cgit/gr-osmosdr/
- pulseaudio (Linux only and optional)
- Qt 4.7 or later with the following components:
    - Core
    - GUI
    - Network
    - Widgets (Qt 5 only)
    - Svg (runtime only)

Gqrx comes with a simple qmake build setup. It can be compiled from within Qt Creator or in a terminal:

<pre>
$ git clone https://github.com/csete/gqrx.git gqrx.git
$ cd gqrx.git
$ mkdir build
$ cd build
$ qmake ..
$ make
</pre>

To build in debug mode add "CONFIG+=debug" to the qmake step above. There are also some other qmake options, see the gqrx.pro file.


Credits and License
-------------------

Gqrx is designed and written by Alexandru Csete OZ9AEC, and it is licensed under the GNU General Public License.
Some of the source files were adopted from Cutesdr by Moe Weatley and these come with a BSD license.
Following people and organisations have contributed:


TODO: Credits for bookmarks


Alex Grinkov:
- FM stereo demodulator.

Anthony Willard:
- Various fixes and improvements

Bastian Bloessl:
Pavel Stano:
- RDS support via gr-rds.

Chris Kuethe:
- Fractional PPM correction.

Christian Lindner DL2VCL:
Stefano Leucci:
- Bookmarks implementation.

Elias Önal:
- Building Gqrx on Mac OS X.
- Crash recovery dialog.

Frank Brickle, AB2KT:
Bob McGwier, N4HY:
- Noise blanker (from dttsp).

Göran Weinholt, SA6CJK:
- Various GUI improvements.

Jiří Pinkava:
- Port to gnuradio 3.7 API.

Kobra @ Xiatek:
- Auto squelch.

Michael Dickens:
- Bugfixes on OSX.

Moe Weatley:
- FFT plotter and waterfall.
- Frequency selector.
- Signal strength indicator.
- AGC

Nadeem Hasan:
- Bug fixes.

Nokia:
- QtColorPicker widget.

Stefano Leucci:
- Peak detection and hold for the FFT plot.

Vesa Solonen:
- DC removal in AM demodulator.

Vincent Pelletier
- Initial work on the horizontal zooming / scrolling.

Will Scales
- Bug fixes.

Also thanks to Volker Schroer and Alexey Bazhin for bringing Funcube Dongle Pro+ support to GNU Radio and Gqrx.

Some of the icons are from the GNOME and Tango icon themes. The scope.svg icon is based on the utilities-system-monitor.svg icon from the Mint-X icon theme licensed under GNU GPL.

Let me know if somebody or someting is missing from the list!

Alex OZ9AEC
