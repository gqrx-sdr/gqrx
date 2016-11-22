Gqrx
====

Gqrx is an open source software defined radio (SDR) receiver implemented using
[GNU Radio](http://gnuradio.org) and the [Qt GUI toolkit](https://www.qt.io/).
Currently it works on Linux and Mac with hardware supported by gr-osmosdr,
including Funcube Dongle, RTL-SDR, Airspy, HackRF, BladeRF, RFSpace, USRP and
SoapySDR.

Gqrx can operate as an AM/FM/SSB receiver with audio output or as an FFT-only
instrument. There are also various hooks for interacting with external
application using nertwork sockets.


Download
--------

Gqrx is distributed as source code package and binaries for Linux and Mac.
Alternate Mac support is available through macports and homebrew.

Please see http://gqrx.dk/download for a list of download resources.


Usage
-----

It is strongly recommended to run the "volk_profile" gnuradio utility before
running gqrx. This will detect and enable processor specific optimisations and
will in many cases give a significant performance boost.

The first time you start gqrx it will open a device configuration dialog.
Supported devices that are connected to the computer are discovered
automatically and you can select any of them in the drop-down list.

If you don't see your device listed in the drop-down list it could be because:
- The driver has not been included in a binary distribution
- The udev rule has not been properly configured
- Linux kernel driver is blocking access to the device

You can test your device using device specific tools, such as rtl_test,
airspy_rx, hackrf_transfer, qthid, etc.

Gqrx supports multiple configurations and sessions if you have several devices
or if you want to use the same device under different configurations. You can
load a configuration from the GUI or using the -c command line argument. See
"gqrx --help" for a complete list of command line arguments.

Tutorials and howtos are being written and published on the website
http://gqrx.dk/


Known problems
--------------

See the bug tracker on Github: https://github.com/csete/gqrx/issues


Getting help and reporting bugs
-------------------------------

There is a Google group for discussing anything related to Gqrx:
https://groups.google.com/forum/#!forum/gqrx
This includes getting help with installation and troubleshooting. Please
remember to provide detailed description of your problem, your setup, what
steps you followed, etc.

Please stick around and help others with their problems. Otherwise, if only
developers provide user support there will be no more time for further
development.


Installation from source
------------------------

Gqrx can be compiled using qmake or cmake.

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
    - gnuradio-pmt
- The gr-iqbalance library (optional)
- Drivers for the hardware you want to have support for:
    - Funcube Dongle Pro driver via gr-fcd
    - UHD driver via gr-uhd
    - Funcube Dongle Pro+ driver from https://github.com/dl1ksv/gr-fcdproplus
    - RTL-SDR driver from http://cgit.osmocom.org/cgit/rtl-sdr/
    - OsmoSDR driver from http://cgit.osmocom.org/cgit/osmo-sdr/
    - HackRF Jawbreaker driver from http://greatscottgadgets.com/hackrf/
    - Airspy driver from https://github.com/airspy/host
    - SoapySDR from https://github.com/pothosware/SoapySDR
    - RFSpace driver is bult in
- gnuradio-osmosdr from http://cgit.osmocom.org/cgit/gr-osmosdr/
- pulseaudio or portaudio (Linux only and optional)
- Qt 5 with the following components:
    - Core
    - GUI
    - Network
    - Widgets
    - Svg (runtime only)
- cmake version >= 3.2.0 if you wish to build using cmake.

To build using qmake, you can either open the gqrx.pro file in Qt Creator and
build, or on the command line:
<pre>
$ git clone https://github.com/csete/gqrx.git gqrx.git
$ cd gqrx.git
$ mkdir build
$ cd build
$ qmake ..
$ make
</pre>

Using cmake, gqrx can be compiled from within Qt Creator or in a terminal:

For command line builds:
<pre>
$ git clone https://github.com/csete/gqrx.git gqrx.git
$ cd gqrx.git
$ mkdir build
$ cd build
$ cmake ..
$ make
</pre>
On some systems, the default cmake release builds are "over optimized" and
perform poorly. In that case try forcing -O2 using
<pre>
export CXXFLAGS=-O2
</pre>
before the cmake step.

For Qt Creator builds:
<pre>
$ git clone https://github.com/csete/gqrx.git gqrx.git
$ cd gqrx.git
$ mkdir build
Start Qt Creator
Open gqrx.git/CMakeLists.txt file
At the dialog asking for build location, select gqrx.git/build
click continue
If asked to choose cmake executable, do so
click continue
click the run cmake button
click done
optionally, on the Projects page, under Build Steps/Make/Additional arguments,
	enter -j4 (replacing 4 with the number of cores in your CPU).
Use Qt Creator as before
</pre>

Credits and License
-------------------

Gqrx is designed and written by Alexandru Csete OZ9AEC, and it is licensed
under the GNU General Public License.

Some of the source files were adopted from Cutesdr by Moe Weatley and these
come with a Simplified BSD license.

Following people and organisations have contributed to gqrx:

Alex Grinkov:
- FM stereo demodulator.

Alexander Fasching:
- Bug fixes.

Andy Sloane:
- Bug fixes and improvements.

Andrea Merello:
- Cmake build option to build using gr-audio.

Anthony Willard:
- Various fixes and improvements

Bastian Bloessl:
Pavel Stano:
- RDS support via gr-rds.

Chris Kuethe:
- Fractional PPM correction.

Christian Lindner DL2VCL:
charlylima:
Stefano Leucci:
- Bookmarks implementation.

Daniil Cherednik:
- FM OIRT stereo.

Dominic Chen:
- Bug fixes.

Elias Önal:
- Building Gqrx on Mac OS X.
- Crash recovery dialog.

Frank Brickle, AB2KT:
Bob McGwier, N4HY:
- Noise blanker (from dttsp).

Göran Weinholt, SA6CJK:
- Various GUI improvements.

Grigory Shipunov:
- Initial .desktop file.

Jiří Pinkava:
- Port to gnuradio 3.7 API.

Josh Blum
- Windows build and MSVC tweaks.

Kate Adams:
- Auto squelch.

Kitware Inc.:
- Widgets from the CTK library (http://commontk.org/).

Michael Dickens:
- Bugfixes and audio on OSX.

Michael Lass:
- Improved tuning ranges at hardware limits.

Michael Tatarinov:
- Documentation and bugfixes.

Moe Weatley:
- FFT plotter and waterfall.
- Frequency selector.
- Signal strength indicator.
- AGC

Nadeem Hasan:
- Bug fixes.

Nokia:
- QtColorPicker widget.

Phil Vachon:
- Bug fixes.

Rob Frohne:
- Initial Qt5 support.

Stefano Leucci:
- Peak detection and hold for the FFT plot.

Timothy Reaves:
- UI layout fixes for Mac.
- cmake build files

Valentin Ochs:
- ALSA support improvement.
- Various bugfixes.

Vesa Solonen:
- DC removal in AM demodulator.

Vincent Pelletier
- Initial work on the horizontal zooming / scrolling.

Will Scales
- Bug fixes.

Wolfgang Fritz DK7OB
- SDRPlay integration.
- 1-2-5 scaling on FFT plot.
- Various UI improvements.

Youssef Touil
- FIRCalc design tool for optimizing input decimator.

Some of the icons are from:
- The GNOME icon theme CC-SA 3.0 by GNOME icon artists 
- Tango icon theme, Public Domain by The people from the Tango! project
- Mint-X icon theme, GPL by Clement Lefebvre

Also thanks to Volker Schroer and Alexey Bazhin for bringing Funcube Dongle
Pro+ support to GNU Radio and Gqrx.

Let me know if somebody or someting is missing from the list!

Alex OZ9AEC
