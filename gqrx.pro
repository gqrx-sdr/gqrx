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
    ICON = resources/icons/gqrx.icns
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

RESOURCES += \
    resources/icons.qrc \
    resources/textfiles.qrc

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
    src/applications/gqrx/main.cpp \
    src/applications/gqrx/mainwindow.cpp \
    src/applications/gqrx/receiver.cpp \
    src/applications/gqrx/remote_control.cpp \
    src/applications/gqrx/remote_control_settings.cpp \
    src/dsp/afsk1200/cafsk12.cpp \
    src/dsp/afsk1200/costabf.c \
    src/dsp/agc_impl.cpp \
    src/dsp/correct_iq_cc.cpp \
    src/dsp/filter/decimator.cpp \
    src/dsp/hbf_decim.cpp \
    src/dsp/lpf.cpp \
    src/dsp/rds/decoder_impl.cc \
    src/dsp/rds/parser_impl.cc \
    src/dsp/resampler_xx.cpp \
    src/dsp/rx_agc_xx.cpp \
    src/dsp/rx_demod_am.cpp \
    src/dsp/rx_demod_fm.cpp \
    src/dsp/rx_fft.cpp \
    src/dsp/rx_filter.cpp \
    src/dsp/rx_meter.cpp \
    src/dsp/rx_noise_blanker_cc.cpp \
    src/dsp/rx_rds.cpp \
    src/dsp/sniffer_f.cpp \
    src/dsp/stereo_demod.cpp \
    src/interfaces/udp_sink_f.cpp \
    src/qtgui/afsk1200win.cpp \
    src/qtgui/agc_options.cpp \
    src/qtgui/audio_options.cpp \
    src/qtgui/bookmarks.cpp \
    src/qtgui/bookmarkstablemodel.cpp \
    src/qtgui/bookmarkstaglist.cpp \
    src/qtgui/demod_options.cpp \
    src/qtgui/dockaudio.cpp \
    src/qtgui/dockbookmarks.cpp \
    src/qtgui/dockinputctl.cpp \
    src/qtgui/dockrds.cpp \
    src/qtgui/dockrxopt.cpp \
    src/qtgui/dockfft.cpp \
    src/qtgui/freqctrl.cpp \
    src/qtgui/ioconfig.cpp \
    src/qtgui/iq_tool.cpp \
    src/qtgui/meter.cpp \
    src/qtgui/nb_options.cpp \
    src/qtgui/plotter.cpp \
    src/qtgui/qtcolorpicker.cpp \
    src/receivers/nbrx.cpp \
    src/receivers/receiver_base.cpp \
    src/receivers/wfmrx.cpp

HEADERS += \
    src/applications/gqrx/gqrx.h \
    src/applications/gqrx/mainwindow.h \
    src/applications/gqrx/receiver.h \
    src/applications/gqrx/remote_control.h \
    src/applications/gqrx/remote_control_settings.h \
    src/dsp/afsk1200/cafsk12.h \
    src/dsp/afsk1200/filter.h \
    src/dsp/afsk1200/filter-i386.h \
    src/dsp/agc_impl.h \
    src/dsp/correct_iq_cc.h \
    src/dsp/filter/decimator.h \
    src/dsp/filter/filtercoef_hbf_70.h \
    src/dsp/filter/filtercoef_hbf_100.h \
    src/dsp/filter/filtercoef_hbf_140.h \
    src/dsp/hbf_decim.h \
    src/dsp/lpf.h \
    src/dsp/rds/api.h \
    src/dsp/rds/parser.h \
    src/dsp/rds/decoder.h \
    src/dsp/rds/decoder_impl.h \
    src/dsp/rds/parser_impl.h \
    src/dsp/rds/constants.h \
    src/dsp/resampler_xx.h \
    src/dsp/rx_agc_xx.h \
    src/dsp/rx_demod_am.h \
    src/dsp/rx_demod_fm.h \
    src/dsp/rx_fft.h \
    src/dsp/rx_filter.h \
    src/dsp/rx_meter.h \
    src/dsp/rx_noise_blanker_cc.h \
    src/dsp/rx_rds.h \
    src/dsp/sniffer_f.h \
    src/dsp/stereo_demod.h \
    src/interfaces/udp_sink_f.h \
    src/qtgui/afsk1200win.h \
    src/qtgui/agc_options.h \
    src/qtgui/audio_options.h \
    src/qtgui/bookmarks.h \
    src/qtgui/bookmarkstablemodel.h \
    src/qtgui/bookmarkstaglist.h \
    src/qtgui/demod_options.h \
    src/qtgui/dockaudio.h \
    src/qtgui/dockbookmarks.h \
    src/qtgui/dockfft.h \
    src/qtgui/dockinputctl.h \
    src/qtgui/dockrds.h \
    src/qtgui/dockrxopt.h \
    src/qtgui/freqctrl.h \
    src/qtgui/ioconfig.h \
    src/qtgui/iq_tool.h \
    src/qtgui/meter.h \
    src/qtgui/nb_options.h \
    src/qtgui/plotter.h \
    src/qtgui/qtcolorpicker.h \
    src/receivers/nbrx.h \
    src/receivers/receiver_base.h \
    src/receivers/wfmrx.h

FORMS += \
    src/applications/gqrx/mainwindow.ui \
    src/applications/gqrx/remote_control_settings.ui \
    src/qtgui/afsk1200win.ui \
    src/qtgui/agc_options.ui \
    src/qtgui/audio_options.ui \
    src/qtgui/demod_options.ui \
    src/qtgui/dockaudio.ui \
    src/qtgui/dockbookmarks.ui \
    src/qtgui/dockfft.ui \
    src/qtgui/dockinputctl.ui \
    src/qtgui/dockrds.ui \
    src/qtgui/iq_tool.ui \
    src/qtgui/dockrxopt.ui \
    src/qtgui/ioconfig.ui \
    src/qtgui/nb_options.ui

# Use pulseaudio (ps: could use equals? undocumented)
contains(AUDIO_BACKEND, pulse): {
    HEADERS += \
        src/pulseaudio/pa_device_list.h \
        src/pulseaudio/pa_sink.h \
        src/pulseaudio/pa_source.h
    SOURCES += \
        src/pulseaudio/pa_device_list.cc \
        src/pulseaudio/pa_sink.cc \
        src/pulseaudio/pa_source.cc
    DEFINES += WITH_PULSEAUDIO
}

macx {
    HEADERS += src/osxaudio/device_list.h
    SOURCES += src/osxaudio/device_list.cpp
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
             gnuradio-runtime \
             gnuradio-osmosdr

INCPATH += src/

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
