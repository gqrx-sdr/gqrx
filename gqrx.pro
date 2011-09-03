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
    # Define version string (see below for releases)
    VER = $$system(git describe --abbrev=8)
} else {
    DEFINES += QT_NO_DEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
    VER = 2.0
}

# Tip from: http://www.qtcentre.org/wiki/index.php?title=Version_numbering_using_QMake
VERSTR = '\\"$${VER}\\"'          # place quotes around the version string
DEFINES += VERSION=\"$${VERSTR}\" # create a VERSION macro containing the version string

# QMAKE_RPATH & co won't work with origin
QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/lib\''

# Use for valgrind
#QMAKE_CFLAGS_DEBUG += '-g -O0'

SOURCES +=\
    receiver.cc \
    main.cc \
    mainwindow.cc \
    qtgui/freqctrl.cpp \
    qtgui/meter.cpp \
    qtgui/plotter.cpp \
    dsp/rx_fft.cc \
    dsp/rx_filter.cc \
    dsp/rx_demod_fm.cc \
    dsp/rx_meter.cc \
    qtgui/dockrxopt.cc \
    dsp/rx_demod_am.cc \
    qtgui/ioconfig.cc \
    qtgui/dockfcdctl.cpp \
    qtgui/dockaudio.cpp \
    dsp/resampler_ff.cc \
    qtgui/dockfft.cpp \
    dsp/sniffer_f.cc \
    dsp/afsk1200/costabf.c \
    dsp/afsk1200/cafsk12.cpp \
    qtgui/dockiqplayer.cpp \
    qtgui/afsk1200win.cc \
    qtgui/bpsk1000win.cc \
    qtgui/arissattlm.cc \
    tlm/arissat/scale_therm.c \
    tlm/arissat/scale_psu.c \
    tlm/arissat/scale_ppt.c


HEADERS  += mainwindow.h \
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
    tlm/arissat/scale_ppt.h

FORMS    += \
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
linux-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio gnuradio-fcd
}

macx-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio gnuradio-fcd
    INCLUDEPATH += /opt/local/include
    INCLUDEPATH += /opt/local/include/gnuradio
}

OTHER_FILES += \
    README \
    COPYING

RESOURCES += \
    icons.qrc
