#-------------------------------------------------
#
# Qmake project file for gqrx
#
#-------------------------------------------------

QT       += core gui

TARGET = gqrx
TEMPLATE = app


SOURCES +=\
    receiver.cc \
    main.cc \
    mainwindow.cc \
    rx_filter.cc \
    rx_demod_fm.cc \
    qtgui/freqctrl.cpp \
    rx_meter.cc \
    qtgui/meter.cpp


HEADERS  += mainwindow.h \
    receiver.h \
    rx_filter.h \
    rx_demod_fm.h \
    qtgui/freqctrl.h \
    rx_meter.h \
    qtgui/meter.h

FORMS    += mainwindow.ui


# dependencies via pkg-config
# FIXME: check for version?
linux-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio gnuradio-fcd
}
