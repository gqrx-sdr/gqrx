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

SOURCES +=\
    receiver.cc \
    main.cc \
    mainwindow.cc \
    rx_filter.cc \
    rx_demod_fm.cc \
    qtgui/freqctrl.cpp \
    rx_meter.cc \
    qtgui/meter.cpp \
    qtgui/plotter.cpp \
    dsp/rx_fft.cc


HEADERS  += mainwindow.h \
    receiver.h \
    rx_filter.h \
    rx_demod_fm.h \
    qtgui/freqctrl.h \
    rx_meter.h \
    qtgui/meter.h \
    qtgui/plotter.h \
    dsp/rx_fft.h

FORMS    += mainwindow.ui


# dependencies via pkg-config
# FIXME: check for version?
linux-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio gnuradio-fcd
}
