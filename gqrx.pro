#-------------------------------------------------
#
# Qmake project file for gqrx
#
#-------------------------------------------------

QT       += core gui

TARGET = gqrx
TEMPLATE = app

# disable debug messages in release
CONFIG(debug, debug|release) {
    # Use for valgrind
    #QMAKE_CFLAGS_DEBUG += '-g -O0'

    # Define version string (see below for releases)
    VER = $$system(git describe --abbrev=8)
} else {
    DEFINES += QT_NO_DEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
    VER = 0.0

    # Release binaries with gr bundled
    # QMAKE_RPATH & co won't work with origin
    QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/lib\''
}

# Tip from: http://www.qtcentre.org/wiki/index.php?title=Version_numbering_using_QMake
VERSTR = '\\"$${VER}\\"'          # place quotes around the version string
DEFINES += VERSION=\"$${VERSTR}\" # create a VERSION macro containing the version string


SOURCES += \
    receiver.cpp \
    main.cpp \
    mainwindow.cpp \
    qtgui/freqctrl.cpp \
    qtgui/meter.cpp \
    qtgui/plotter.cpp \
    dsp/rx_fft.cpp \
    dsp/rx_filter.cpp \
    dsp/rx_demod_fm.cpp \
    dsp/rx_meter.cpp \
    qtgui/dockrxopt.cpp \
    dsp/rx_demod_am.cpp \
    qtgui/ioconfig.cpp \
    qtgui/dockfcdctl.cpp \
    qtgui/dockaudio.cpp \
    dsp/resampler_ff.cpp \
    qtgui/dockfft.cpp \
    dsp/sniffer_f.cpp \
    dsp/afsk1200/costabf.c \
    dsp/afsk1200/cafsk12.cpp \
    qtgui/dockiqplayer.cpp \
    qtgui/afsk1200win.cpp \
    qtgui/bpsk1000win.cpp \
    qtgui/arissattlm.cpp \
    tlm/arissat/scale_therm.c \
    tlm/arissat/scale_psu.c \
    tlm/arissat/scale_ppt.c \
    dsp/rx_source_base.cpp \
    dsp/rx_source_fcd.cpp \
    dsp/rx_agc_xx.cpp \
    dsp/agc_impl.cpp \
    dsp/correct_iq_cc.cpp \
    pulseaudio/pa_device_list.cc \  # FIXME: Linux only
    pulseaudio/pa_sink.cc \
    pulseaudio/pa_source.cc \
    fcdctl/fcd.c \
    fcdctl/hid-libusb.c  # FIXME: Linux only
#    fcdctl/hidwin.c \
#    fcdctl/hidmac.c \

HEADERS += \
    mainwindow.h \
    receiver.h \
    qtgui/freqctrl.h \
    qtgui/meter.h \
    qtgui/plotter.h \
    dsp/rx_fft.h \
    dsp/rx_filter.h \
    dsp/rx_demod_fm.h \
    dsp/rx_meter.h \
    qtgui/dockrxopt.h \
    dsp/rx_demod_am.h \
    qtgui/ioconfig.h \
    qtgui/dockfcdctl.h \
    qtgui/dockaudio.h \
    dsp/resampler_ff.h \
    qtgui/dockfft.h \
    dsp/sniffer_f.h \
    dsp/afsk1200/filter-i386.h \
    dsp/afsk1200/filter.h \
    dsp/afsk1200/cafsk12.h \
    qtgui/dockiqplayer.h \
    qtgui/afsk1200win.h \
    qtgui/bpsk1000win.h \
    tlm/arissat/ss_types_common.h \
    tlm/arissat/ss_stdint.h \
    qtgui/arissattlm.h \
    tlm/arissat/scale_therm.h \
    tlm/arissat/scale_psu.h \
    tlm/arissat/scale_ppt.h \
    dsp/rx_source_base.h \
    dsp/rx_source_fcd.h \
    dsp/rx_agc_xx.h \
    dsp/agc_impl.h \
    dsp/correct_iq_cc.h \
    pulseaudio/pa_device_list.h \  # FIXME: Linux only
    pulseaudio/pa_sink.h \
    pulseaudio/pa_source.h \
    fcdctl/hidapi.h \
    fcdctl/fcdhidcmd.h \
    fcdctl/fcd.h

FORMS += \
    qtgui/dockrxopt.ui \
    mainwindow.ui \
    qtgui/ioconfig.ui \
    qtgui/dockfcdctl.ui \
    qtgui/dockaudio.ui \
    qtgui/dockfft.ui \
    qtgui/dockiqplayer.ui \
    qtgui/afsk1200win.ui \
    qtgui/bpsk1000win.ui \
    qtgui/arissattlm.ui


# dependencies via pkg-config
# FIXME: check for version?
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio
    PKGCONFIG += libpulse libpulse-simple
    PKGCONFIG += libusb-1.0
}

macx-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio
    INCLUDEPATH += /opt/local/include
    INCLUDEPATH += /opt/local/include/gnuradio
}

OTHER_FILES += \
    README \
    COPYING

RESOURCES += \
    icons.qrc
