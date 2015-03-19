#--------------------------------------------------------------------------------
#
# Qmake project file for gqrx - http://gqrx.dk
#
# Common options you may want to passs to qmake:
#
#    CONFIG+=debug          Enable debug mode
#    PREFIX=/some/prefix    Installation prefix
#    BOOST_SUFFIX=-mt       To link against libboost-xyz-mt (needed for pybombs)
#--------------------------------------------------------------------------------

QT       += core gui network
contains(QT_MAJOR_VERSION,5) {
    QT += widgets
}

TEMPLATE = app

macx {
    TARGET = Gqrx
    ICON = icons/gqrx.icns
    DEFINES += GQRX_OS_MACX
} else {
    TARGET = gqrx
}

# enable pkg-config to find dependencies
CONFIG += link_pkgconfig

unix:!macx {
    packagesExist(libpulse libpulse-simple) {
        # Comment out to use gr-audio (not recommended with ALSA and Funcube Dongle Pro)
        AUDIO_BACKEND = pulse
        message("Gqrx configured with pulseaudio backend")
    }
}

RESOURCES += icons.qrc \
    textfiles.qrc

# make clean target
QMAKE_CLEAN += gqrx

# make install target
isEmpty(PREFIX) {
    message(No prefix given. Using /usr/local)
    PREFIX=/usr/local
}

target.path  = $$PREFIX/bin
INSTALLS    += target 

#CONFIG += debug

# disable debug messages in release
CONFIG(debug, debug|release) {
    # Use for valgrind
    #QMAKE_CFLAGS_DEBUG += '-g -O0'

    # Define version string (see below for releases)
    VER = $$system(git describe --abbrev=8)

} else {
    DEFINES += QT_NO_DEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
    VER = $$system(git describe --abbrev=1)
    ## VER = v2.3.1-37-g78ea9469

    # Release binaries with gr bundled
    # QMAKE_RPATH & co won't work with origin
    ## QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/lib\''
}


# Tip from: http://www.qtcentre.org/wiki/index.php?title=Version_numbering_using_QMake
VERSTR = '\\"$${VER}\\"'          # place quotes around the version string
DEFINES += VERSION=\"$${VERSTR}\" # create a VERSION macro containing the version string

SOURCES += \
    applications/gqrx/main.cpp \
    applications/gqrx/mainwindow.cpp \
    applications/gqrx/receiver.cpp \
    applications/gqrx/remote_control.cpp \
    applications/gqrx/remote_control_settings.cpp \
    dsp/afsk1200/cafsk12.cpp \
    dsp/afsk1200/costabf.c \
    dsp/agc_impl.cpp \
    dsp/correct_iq_cc.cpp \
    dsp/lpf.cpp \
    dsp/rds/decoder_impl.cc \
    dsp/rds/parser_impl.cc \
    dsp/resampler_xx.cpp \
    dsp/rx_agc_xx.cpp \
    dsp/rx_demod_am.cpp \
    dsp/rx_demod_fm.cpp \
    dsp/rx_fft.cpp \
    dsp/rx_filter.cpp \
    dsp/rx_meter.cpp \
    dsp/rx_noise_blanker_cc.cpp \
    dsp/rx_rds.cpp \
    dsp/sniffer_f.cpp \
    dsp/stereo_demod.cpp \
    interfaces/udp_sink_f.cpp \
    qtgui/afsk1200win.cpp \
    qtgui/agc_options.cpp \
    qtgui/audio_options.cpp \
    qtgui/bookmarks.cpp \
    qtgui/bookmarkstablemodel.cpp \
    qtgui/bookmarkstaglist.cpp \
    qtgui/demod_options.cpp \
    qtgui/dockaudio.cpp \
    qtgui/dockbookmarks.cpp \
    qtgui/dockinputctl.cpp \
    qtgui/dockrds.cpp \
    qtgui/dockrxopt.cpp \
    qtgui/dockfft.cpp \
    qtgui/freqctrl.cpp \
    qtgui/ioconfig.cpp \
    qtgui/iq_tool.cpp \
    qtgui/meter.cpp \
    qtgui/nb_options.cpp \
    qtgui/plotter.cpp \
    qtgui/qtcolorpicker.cpp \
    receivers/nbrx.cpp \
    receivers/receiver_base.cpp \
    receivers/wfmrx.cpp

HEADERS += \
    applications/gqrx/gqrx.h \
    applications/gqrx/mainwindow.h \
    applications/gqrx/receiver.h \
    applications/gqrx/remote_control.h \
    applications/gqrx/remote_control_settings.h \
    dsp/afsk1200/cafsk12.h \
    dsp/afsk1200/filter.h \
    dsp/afsk1200/filter-i386.h \
    dsp/agc_impl.h \
    dsp/correct_iq_cc.h \
    dsp/lpf.h \
    dsp/rds/api.h \
    dsp/rds/parser.h \
    dsp/rds/decoder.h \
    dsp/rds/decoder_impl.h \
    dsp/rds/parser_impl.h \
    dsp/rds/constants.h \
    dsp/resampler_xx.h \
    dsp/rx_agc_xx.h \
    dsp/rx_demod_am.h \
    dsp/rx_demod_fm.h \
    dsp/rx_fft.h \
    dsp/rx_filter.h \
    dsp/rx_meter.h \
    dsp/rx_noise_blanker_cc.h \
    dsp/rx_rds.h \
    dsp/sniffer_f.h \
    dsp/stereo_demod.h \
    interfaces/udp_sink_f.h \
    qtgui/afsk1200win.h \
    qtgui/agc_options.h \
    qtgui/audio_options.h \
    qtgui/bookmarks.h \
    qtgui/bookmarkstablemodel.h \
    qtgui/bookmarkstaglist.h \
    qtgui/demod_options.h \
    qtgui/dockaudio.h \
    qtgui/dockbookmarks.h \
    qtgui/dockfft.h \
    qtgui/dockinputctl.h \
    qtgui/dockrds.h \
    qtgui/dockrxopt.h \
    qtgui/freqctrl.h \
    qtgui/ioconfig.h \
    qtgui/iq_tool.h \
    qtgui/meter.h \
    qtgui/nb_options.h \
    qtgui/plotter.h \
    qtgui/qtcolorpicker.h \
    receivers/nbrx.h \
    receivers/receiver_base.h \
    receivers/wfmrx.h

FORMS += \
    applications/gqrx/mainwindow.ui \
    applications/gqrx/remote_control_settings.ui \
    qtgui/afsk1200win.ui \
    qtgui/agc_options.ui \
    qtgui/audio_options.ui \
    qtgui/demod_options.ui \
    qtgui/dockaudio.ui \
    qtgui/dockbookmarks.ui \
    qtgui/dockfft.ui \
    qtgui/dockinputctl.ui \
    qtgui/dockrds.ui \
    qtgui/iq_tool.ui \
    qtgui/dockrxopt.ui \
    qtgui/ioconfig.ui \
    qtgui/nb_options.ui

# Use pulseaudio (ps: could use equals? undocumented)
contains(AUDIO_BACKEND, pulse): {
    HEADERS += \
        pulseaudio/pa_device_list.h \
        pulseaudio/pa_sink.h \
        pulseaudio/pa_source.h
    SOURCES += \
        pulseaudio/pa_device_list.cc \
        pulseaudio/pa_sink.cc \
        pulseaudio/pa_source.cc
    DEFINES += WITH_PULSEAUDIO
}

macx {
    HEADERS += osxaudio/device_list.h
    SOURCES += osxaudio/device_list.cpp
}

contains(AUDIO_BACKEND, pulse): {
    PKGCONFIG += libpulse libpulse-simple
} else {
    PKGCONFIG += gnuradio-audio
}

PKGCONFIG += gnuradio-analog \
             gnuradio-blocks \
             gnuradio-digital \
             gnuradio-filter \
             gnuradio-fft \
             gnuradio-osmosdr

unix:!macx {
    LIBS += -lboost_system$$BOOST_SUFFIX -lboost_program_options$$BOOST_SUFFIX
    LIBS += -lrt  # need to include on some distros
}

macx {
    LIBS += -lboost_system-mt -lboost_program_options-mt
}

OTHER_FILES += \
    bookmarks.csv \
    gqrx.desktop \
    README.md \
    COPYING \
    news.txt
