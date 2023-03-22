/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
 * Copyright (C) 2013 by Elias Oenal <EliasOenal@gmail.com>
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <string>
#include <vector>
#include <volk/volk.h>

#include <QSettings>
#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFile>
#include <QGroupBox>
#include <QKeySequence>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QResource>
#include <QShortcut>
#include <QString>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextStream>
#include <QtGlobal>
#include <QTimer>
#include <QVBoxLayout>
#include <QSvgWidget>
#include "qtgui/ioconfig.h"
#include "mainwindow.h"
#include "qtgui/dxc_options.h"
#include "qtgui/dxc_spots.h"

/* Qt Designer files */
#include "ui_mainwindow.h"

/* DSP */
#include "receiver.h"
#include "remote_control_settings.h"

#include "qtgui/bookmarkstaglist.h"
#include "qtgui/bandplan.h"

MainWindow::MainWindow(const QString& cfgfile, bool edit_conf, QWidget *parent) :
    QMainWindow(parent),
    configOk(true),
    ui(new Ui::MainWindow),
    d_lnb_lo(0),
    d_hw_freq(0),
    d_fftAvg(0.25),
    d_have_audio(true),
    dec_afsk1200(nullptr)
{
    ui->setupUi(this);
    BandPlan::create();
    Bookmarks::create();
    DXCSpots::create();

    /* Initialise default configuration directory */
    QByteArray xdg_dir = qgetenv("XDG_CONFIG_HOME");
    if (xdg_dir.isEmpty())
    {
        // Qt takes care of conversion to native separators
        m_cfg_dir = QString("%1/.config/gqrx").arg(QDir::homePath());
    }
    else
    {
        m_cfg_dir = QString("%1/gqrx").arg(xdg_dir.data());
    }

    setWindowTitle(QString("Gqrx %1").arg(VERSION));

    // Set fixed widths for labels so they don't move around when set
    QFontMetrics metrics(font);
    QRect markerRect = metrics.boundingRect("99,999,999.999 kHz");
    ui->markerLabelA->setFixedWidth(markerRect.width());
    ui->markerLabelB->setFixedWidth(markerRect.width());
    QRect deltaFreqRect = metrics.boundingRect("Δ99,999,999.999 kHz   ⨏99,999,999.999 kHz");
    ui->deltaFreqLabel->setFixedWidth(deltaFreqRect.width());
    setMarkerA(MARKER_OFF);
    setMarkerB(MARKER_OFF);
    d_show_markers = true;

    /* frequency control widget */
    ui->freqCtrl->setup(0, 0, 9999e6, 1, FCTL_UNIT_NONE);
    ui->freqCtrl->setFrequency(144500000);

    d_filter_shape = receiver::FILTER_SHAPE_NORMAL;

    /* create receiver object */
    rx = new receiver("", "", 1);
    rx->set_rf_freq(144500000.0f);

    // remote controller
    remote = new RemoteControl();

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* FFT timer & data */
    iq_fft_timer = new QTimer(this);
    connect(iq_fft_timer, SIGNAL(timeout()), this, SLOT(iqFftTimeout()));

    audio_fft_timer = new QTimer(this);
    connect(audio_fft_timer, SIGNAL(timeout()), this, SLOT(audioFftTimeout()));

    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new float[MAX_FFT_SIZE];

    /* timer for data decoders */
    dec_timer = new QTimer(this);
    connect(dec_timer, SIGNAL(timeout()), this, SLOT(decoderTimeout()));

    // create I/Q tool widget
    iq_tool = new CIqTool(this);

    // create DXC Objects
    dxc_options = new DXCOptions(this);

    /* create dock widgets */
    uiDockRxOpt = new DockRxOpt();
    uiDockRDS = new DockRDS();
    uiDockAudio = new DockAudio();
    uiDockInputCtl = new DockInputCtl();
    uiDockFft = new DockFft();
    BandPlan::Get().setConfigDir(m_cfg_dir);
    Bookmarks::Get().setConfigDir(m_cfg_dir);
    BandPlan::Get().load();
    uiDockBookmarks = new DockBookmarks(this);

    // setup some toggle view shortcuts
    uiDockInputCtl->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_J));
    uiDockRxOpt->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    uiDockFft->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    uiDockAudio->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_A));
    uiDockBookmarks->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    ui->mainToolBar->toggleViewAction()->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));

    /* frequency setting shortcut */
    auto *freq_shortcut = new QShortcut(QKeySequence(Qt::Key_F), this);
    QObject::connect(freq_shortcut, &QShortcut::activated, this, &MainWindow::frequencyFocusShortcut);

    // zero cursor (rx filter offset)
    auto *rx_offset_zero_shortcut = new QShortcut(QKeySequence(Qt::Key_Z), this);
    QObject::connect(rx_offset_zero_shortcut, &QShortcut::activated, this, &MainWindow::rxOffsetZeroShortcut);
    // freeze plotter on SPACE
    auto *toggle_freeze_shortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    QObject::connect(toggle_freeze_shortcut, &QShortcut::activated, this, &MainWindow::toggleFreezeShortcut);
    // toggle markers on/off
    auto *toggle_markers_shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this);
    QObject::connect(toggle_markers_shortcut, &QShortcut::activated, this, &MainWindow::toggleMarkers);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    /* Add dock widgets to main window. This should be done even for
       dock widgets that are going to be hidden, otherwise they will
       end up floating in their own top-level window and can not be
       docked to the mainwindow.
    */
    addDockWidget(Qt::RightDockWidgetArea, uiDockInputCtl);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRxOpt);
    addDockWidget(Qt::RightDockWidgetArea, uiDockFft);
    tabifyDockWidget(uiDockInputCtl, uiDockRxOpt);
    tabifyDockWidget(uiDockRxOpt, uiDockFft);
    uiDockRxOpt->raise();

    addDockWidget(Qt::RightDockWidgetArea, uiDockAudio);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRDS);
    tabifyDockWidget(uiDockAudio, uiDockRDS);
    uiDockAudio->raise();

    addDockWidget(Qt::BottomDockWidgetArea, uiDockBookmarks);

    /* hide docks that we don't want to show initially */
    uiDockBookmarks->hide();
    uiDockRDS->hide();

    /* Add dock widget actions to View menu. By doing it this way all signal/slot
       connections will be established automagially.
    */
    ui->menu_View->addAction(uiDockInputCtl->toggleViewAction());
    ui->menu_View->addAction(uiDockRxOpt->toggleViewAction());
    ui->menu_View->addAction(uiDockRDS->toggleViewAction());
    ui->menu_View->addAction(uiDockAudio->toggleViewAction());
    ui->menu_View->addAction(uiDockFft->toggleViewAction());
    ui->menu_View->addAction(uiDockBookmarks->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->mainToolBar->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->actionFullScreen);

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), remote, SLOT(setNewFrequency(qint64)));
    connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), uiDockAudio, SLOT(setRxFrequency(qint64)));
    connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), uiDockRxOpt, SLOT(setRxFreq(qint64)));
    connect(uiDockInputCtl, SIGNAL(lnbLoChanged(double)), this, SLOT(setLnbLo(double)));
    connect(uiDockInputCtl, SIGNAL(lnbLoChanged(double)), remote, SLOT(setLnbLo(double)));
    connect(uiDockInputCtl, SIGNAL(gainChanged(QString, double)), this, SLOT(setGain(QString,double)));
    connect(uiDockInputCtl, SIGNAL(gainChanged(QString, double)), remote, SLOT(setGain(QString,double)));
    connect(uiDockInputCtl, SIGNAL(autoGainChanged(bool)), this, SLOT(setAutoGain(bool)));
    connect(uiDockInputCtl, SIGNAL(freqCorrChanged(double)), this, SLOT(setFreqCorr(double)));
    connect(uiDockInputCtl, SIGNAL(iqSwapChanged(bool)), this, SLOT(setIqSwap(bool)));
    connect(uiDockInputCtl, SIGNAL(dcCancelChanged(bool)), this, SLOT(setDcCancel(bool)));
    connect(uiDockInputCtl, SIGNAL(iqBalanceChanged(bool)), this, SLOT(setIqBalance(bool)));
    connect(uiDockInputCtl, SIGNAL(ignoreLimitsChanged(bool)), this, SLOT(setIgnoreLimits(bool)));
    connect(uiDockInputCtl, SIGNAL(antennaSelected(QString)), this, SLOT(setAntenna(QString)));
    connect(uiDockInputCtl, SIGNAL(freqCtrlResetChanged(bool)), this, SLOT(setFreqCtrlReset(bool)));
    connect(uiDockInputCtl, SIGNAL(invertScrollingChanged(bool)), this, SLOT(setInvertScrolling(bool)));
    connect(uiDockRxOpt, SIGNAL(rxFreqChanged(qint64)), ui->freqCtrl, SLOT(setFrequency(qint64)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), remote, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), this, SLOT(selectDemod(int)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), remote, SLOT(setMode(int)));
    connect(uiDockRxOpt, SIGNAL(fmMaxdevSelected(float)), this, SLOT(setFmMaxdev(float)));
    connect(uiDockRxOpt, SIGNAL(fmEmphSelected(double)), this, SLOT(setFmEmph(double)));
    connect(uiDockRxOpt, SIGNAL(amDcrToggled(bool)), this, SLOT(setAmDcr(bool)));
    connect(uiDockRxOpt, SIGNAL(cwOffsetChanged(int)), this, SLOT(setCwOffset(int)));
    connect(uiDockRxOpt, SIGNAL(amSyncDcrToggled(bool)), this, SLOT(setAmSyncDcr(bool)));
    connect(uiDockRxOpt, SIGNAL(amSyncPllBwSelected(float)), this, SLOT(setAmSyncPllBw(float)));
    connect(uiDockRxOpt, SIGNAL(agcToggled(bool)), this, SLOT(setAgcOn(bool)));
    connect(uiDockRxOpt, SIGNAL(agcHangToggled(bool)), this, SLOT(setAgcHang(bool)));
    connect(uiDockRxOpt, SIGNAL(agcThresholdChanged(int)), this, SLOT(setAgcThreshold(int)));
    connect(uiDockRxOpt, SIGNAL(agcSlopeChanged(int)), this, SLOT(setAgcSlope(int)));
    connect(uiDockRxOpt, SIGNAL(agcGainChanged(int)), this, SLOT(setAgcGain(int)));
    connect(uiDockRxOpt, SIGNAL(agcDecayChanged(int)), this, SLOT(setAgcDecay(int)));
    connect(uiDockRxOpt, SIGNAL(noiseBlankerChanged(int,bool,float)), this, SLOT(setNoiseBlanker(int,bool,float)));
    connect(uiDockRxOpt, SIGNAL(sqlLevelChanged(double)), this, SLOT(setSqlLevel(double)));
    connect(uiDockRxOpt, SIGNAL(sqlAutoClicked()), this, SLOT(setSqlLevelAuto()));
    connect(uiDockAudio, SIGNAL(audioGainChanged(float)), this, SLOT(setAudioGain(float)));
    connect(uiDockAudio, SIGNAL(audioGainChanged(float)), remote, SLOT(setAudioGain(float)));
    connect(uiDockAudio, SIGNAL(audioStreamingStarted(QString,int,bool)), this, SLOT(startAudioStream(QString,int,bool)));
    connect(uiDockAudio, SIGNAL(audioStreamingStopped()), this, SLOT(stopAudioStreaming()));
    connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), this, SLOT(startAudioRec(QString)));
    connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), remote, SLOT(startAudioRecorder(QString)));
    connect(uiDockAudio, SIGNAL(audioRecStopped()), this, SLOT(stopAudioRec()));
    connect(uiDockAudio, SIGNAL(audioRecStopped()), remote, SLOT(stopAudioRecorder()));
    connect(uiDockAudio, SIGNAL(audioPlayStarted(QString)), this, SLOT(startAudioPlayback(QString)));
    connect(uiDockAudio, SIGNAL(audioPlayStopped()), this, SLOT(stopAudioPlayback()));
    connect(uiDockAudio, SIGNAL(fftRateChanged(int)), this, SLOT(setAudioFftRate(int)));

    // FFT Dock
    connect(uiDockFft, SIGNAL(fftSizeChanged(int)), this, SLOT(setIqFftSize(int)));
    connect(uiDockFft, SIGNAL(fftRateChanged(int)), this, SLOT(setIqFftRate(int)));
    connect(uiDockFft, SIGNAL(fftWindowChanged(int)), this, SLOT(setIqFftWindow(int)));
    connect(uiDockFft, SIGNAL(wfSpanChanged(quint64)), this, SLOT(setWfTimeSpan(quint64)));
    connect(uiDockFft, SIGNAL(fftSplitChanged(int)), this, SLOT(setIqFftSplit(int)));
    connect(uiDockFft, SIGNAL(fftAvgChanged(float)), ui->plotter, SLOT(setFftAvg(float)));
    connect(uiDockFft, SIGNAL(fftZoomChanged(float)), ui->plotter, SLOT(zoomOnXAxis(float)));
    connect(uiDockFft, SIGNAL(waterfallModeChanged(int)), ui->plotter, SLOT(setWaterfallMode(int)));
    connect(uiDockFft, SIGNAL(plotModeChanged(int)), ui->plotter, SLOT(setPlotMode(int)));
    connect(uiDockFft, SIGNAL(plotScaleChanged(int)), ui->plotter, SLOT(setPlotScale(int)));
    connect(uiDockFft, SIGNAL(plotPerChanged(int)), ui->plotter, SLOT(setPlotPer(int)));
    connect(uiDockFft, SIGNAL(resetFftZoom()), ui->plotter, SLOT(resetHorizontalZoom()));
    connect(uiDockFft, SIGNAL(gotoFftCenter()), ui->plotter, SLOT(moveToCenterFreq()));
    connect(uiDockFft, SIGNAL(gotoDemodFreq()), ui->plotter, SLOT(moveToDemodFreq()));
    connect(uiDockFft, SIGNAL(bandPlanChanged(bool)), ui->plotter, SLOT(enableBandPlan(bool)));
    connect(uiDockFft, SIGNAL(markersChanged(bool)), ui->plotter, SLOT(enableMarkers(bool)));
    connect(uiDockFft, SIGNAL(markersChanged(bool)), this, SLOT(enableMarkers(bool)));
    connect(uiDockFft, SIGNAL(wfColormapChanged(const QString)), ui->plotter, SLOT(setWfColormap(const QString)));
    connect(uiDockFft, SIGNAL(wfColormapChanged(const QString)), uiDockAudio, SLOT(setWfColormap(const QString)));
    connect(uiDockFft, SIGNAL(pandapterRangeChanged(float,float)),
            ui->plotter, SLOT(setPandapterRange(float,float)));
    connect(uiDockFft, SIGNAL(waterfallRangeChanged(float,float)),
            ui->plotter, SLOT(setWaterfallRange(float,float)));
    connect(uiDockFft, SIGNAL(fftColorChanged(QColor)), this, SLOT(setFftColor(QColor)));
    connect(uiDockFft, SIGNAL(fftFillToggled(bool)), this, SLOT(enableFftFill(bool)));
    connect(uiDockFft, SIGNAL(fftMaxHoldToggled(bool)), ui->plotter, SLOT(enableMaxHold(bool)));
    connect(uiDockFft, SIGNAL(fftMinHoldToggled(bool)), ui->plotter, SLOT(enableMinHold(bool)));
    connect(uiDockFft, SIGNAL(peakDetectToggled(bool)), ui->plotter, SLOT(enablePeakDetect(bool)));
    connect(uiDockRDS, SIGNAL(rdsDecoderToggled(bool)), this, SLOT(setRdsDecoder(bool)));

    // Plotter
    connect(ui->plotter, SIGNAL(pandapterRangeChanged(float,float)),
            uiDockFft, SLOT(setPandapterRange(float,float)));
    connect(ui->plotter, SIGNAL(newZoomLevel(float)),
            uiDockFft, SLOT(setZoomLevel(float)));
    connect(ui->plotter, SIGNAL(newSize()), this, SLOT(setWfSize()));
    connect(ui->plotter, SIGNAL(markerSelectA(qint64)), this, SLOT(setMarkerA(qint64)));
    connect(ui->plotter, SIGNAL(markerSelectB(qint64)), this, SLOT(setMarkerB(qint64)));

    // Bookmarks
    connect(uiDockBookmarks, SIGNAL(newBookmarkActivated(qint64, QString, int)), this, SLOT(onBookmarkActivated(qint64, QString, int)));
    connect(uiDockBookmarks->actionAddBookmark, SIGNAL(triggered()), this, SLOT(on_actionAddBookmark_triggered()));
    connect(&Bookmarks::Get(), SIGNAL(BookmarksChanged()), ui->plotter, SLOT(updateOverlay()));

    //DXC Spots
    connect(&DXCSpots::Get(), SIGNAL(dxcSpotsUpdated()), this, SLOT(updateClusterSpots()));

    // I/Q playback
    connect(iq_tool, SIGNAL(startRecording(QString)), this, SLOT(startIqRecording(QString)));
    connect(iq_tool, SIGNAL(stopRecording()), this, SLOT(stopIqRecording()));
    connect(iq_tool, SIGNAL(startPlayback(QString,float,qint64)), this, SLOT(startIqPlayback(QString,float,qint64)));
    connect(iq_tool, SIGNAL(stopPlayback()), this, SLOT(stopIqPlayback()));
    connect(iq_tool, SIGNAL(seek(qint64)), this,SLOT(seekIqFile(qint64)));

    // remote control
    connect(remote, SIGNAL(newRDSmode(bool)), uiDockRDS, SLOT(setRDSmode(bool)));
    connect(remote, SIGNAL(newFilterOffset(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(remote, SIGNAL(newFilterOffset(qint64)), uiDockRxOpt, SLOT(setFilterOffset(qint64)));
    connect(remote, SIGNAL(newFrequency(qint64)), ui->freqCtrl, SLOT(setFrequency(qint64)));
    connect(remote, SIGNAL(newLnbLo(double)), uiDockInputCtl, SLOT(setLnbLo(double)));
    connect(remote, SIGNAL(newLnbLo(double)), this, SLOT(setLnbLo(double)));
    connect(remote, SIGNAL(newMode(int)), this, SLOT(selectDemod(int)));
    connect(remote, SIGNAL(newMode(int)), uiDockRxOpt, SLOT(setCurrentDemod(int)));
    connect(remote, SIGNAL(newSquelchLevel(double)), this, SLOT(setSqlLevel(double)));
    connect(remote, SIGNAL(newSquelchLevel(double)), uiDockRxOpt, SLOT(setSquelchLevel(double)));
    connect(remote, SIGNAL(newAudioGain(float)), uiDockAudio, SLOT(setAudioGainDb(float)));
    connect(uiDockRxOpt, SIGNAL(sqlLevelChanged(double)), remote, SLOT(setSquelchLevel(double)));
    connect(remote, SIGNAL(startAudioRecorderEvent()), uiDockAudio, SLOT(startAudioRecorder()));
    connect(remote, SIGNAL(stopAudioRecorderEvent()), uiDockAudio, SLOT(stopAudioRecorder()));
    connect(ui->plotter, SIGNAL(newFilterFreq(int, int)), remote, SLOT(setPassband(int, int)));
    connect(remote, SIGNAL(newPassband(int)), this, SLOT(setPassband(int)));
    connect(remote, SIGNAL(gainChanged(QString, double)), uiDockInputCtl, SLOT(setGain(QString,double)));
    connect(remote, SIGNAL(dspChanged(bool)), this, SLOT(on_actionDSP_triggered(bool)));
    connect(uiDockRDS, SIGNAL(rdsPI(QString)), remote, SLOT(rdsPI(QString)));

    rds_timer = new QTimer(this);
    connect(rds_timer, SIGNAL(timeout()), this, SLOT(rdsTimeout()));

    // enable frequency tooltips on FFT plot
    ui->plotter->setTooltipsEnabled(true);

    // Create list of input devices. This must be done before the configuration is
    // restored because device probing might change the device configuration
    CIoConfig::getDeviceList(devList);

    m_recent_config = new RecentConfig(m_cfg_dir, ui->menu_RecentConfig);
    connect(m_recent_config, SIGNAL(loadConfig(const QString &)), this, SLOT(loadConfigSlot(const QString &)));

    // restore last session
    if (!loadConfig(cfgfile, true, true))
    {

      // first time config
        qDebug() << "Launching I/O device editor";
        if (firstTimeConfig() != QDialog::Accepted)
        {
            qDebug() << "I/O device configuration cancelled.";
            configOk = false;
        }
        else
        {
            configOk = true;
        }
    }
    else if (edit_conf)
    {
        qDebug() << "Launching I/O device editor";
        if (on_actionIoConfig_triggered() != QDialog::Accepted)
        {
            qDebug() << "I/O device configuration cancelled.";
            configOk = false;
        }
        else
        {
            configOk = true;
        }
    }

    qsvg_dummy = new QSvgWidget();
}

MainWindow::~MainWindow()
{
    on_actionDSP_triggered(false);

    /* stop and delete timers */
    dec_timer->stop();
    delete dec_timer;

    meter_timer->stop();
    delete meter_timer;

    iq_fft_timer->stop();
    delete iq_fft_timer;

    audio_fft_timer->stop();
    delete audio_fft_timer;

    if (m_settings)
    {
        m_settings->setValue("configversion", 3);
        m_settings->setValue("crashed", false);

        // hide toolbar (default=false)
        if (ui->mainToolBar->isHidden())
            m_settings->setValue("gui/hide_toolbar", true);
        else
            m_settings->remove("gui/hide_toolbar");

        m_settings->setValue("gui/geometry", saveGeometry());
        m_settings->setValue("gui/state", saveState());

        // save session
        storeSession();

        m_settings->sync();
        delete m_settings;
    }

    delete m_recent_config;

    delete iq_tool;
    delete dxc_options;
    delete ui;
    delete uiDockRxOpt;
    delete uiDockAudio;
    delete uiDockBookmarks;
    delete uiDockFft;
    delete uiDockInputCtl;
    delete uiDockRDS;
    delete rx;
    delete remote;
    delete [] d_fftData;
    delete [] d_realFftData;
    delete qsvg_dummy;
}

/**
 * Load new configuration.
 * @param cfgfile
 * @returns True if config is OK, False if not (e.g. no input device specified).
 *
 * If cfgfile is an absolute path it will be used as is, otherwise it is assumed
 * to be the name of a file under m_cfg_dir.
 *
 * If cfgfile does not exist it will be created.
 *
 * If no input device is specified, we return false to signal that the I/O
 * configuration dialog should be run.
 *
 * FIXME: Refactor.
 */
bool MainWindow::loadConfig(const QString& cfgfile, bool check_crash,
                            bool restore_mainwindow)
{
    double      actual_rate;
    qint64      int64_val;
    int         int_val;
    bool        bool_val;
    bool        conf_ok = false;
    bool        conv_ok;
    bool        skip_loading_cfg = false;

    qDebug() << "Loading configuration from:" << cfgfile;

    if (m_settings)
    {
        // set current config to not crashed before loading new config
        m_settings->setValue("crashed", false);
        m_settings->sync();
        delete m_settings;
    }

    if (QDir::isAbsolutePath(cfgfile))
        m_settings = new QSettings(cfgfile, QSettings::IniFormat);
    else
        m_settings = new QSettings(QString("%1/%2").arg(m_cfg_dir).arg(cfgfile),
                                   QSettings::IniFormat);

    qDebug() << "Configuration file:" << m_settings->fileName();

    if (check_crash)
    {
        if (m_settings->value("crashed", false).toBool())
        {
            qDebug() << "Crash guard triggered!";
            auto* askUserAboutConfig =
                    new QMessageBox(QMessageBox::Warning, tr("Crash Detected!"),
                                    tr("<p>Gqrx has detected problems with the current configuration. "
                                       "Loading the configuration again could cause the application to crash.</p>"
                                       "<p>Do you want to edit the settings?</p>"),
                                    QMessageBox::Yes | QMessageBox::No);
            askUserAboutConfig->setDefaultButton(QMessageBox::Yes);
            askUserAboutConfig->setTextFormat(Qt::RichText);
            askUserAboutConfig->exec();
            if (askUserAboutConfig->result() == QMessageBox::Yes)
                skip_loading_cfg = true;

            delete askUserAboutConfig;
        }
        else
        {
            m_settings->setValue("crashed", true); // clean exit will set this to FALSE
            m_settings->sync();
        }
    }

    if (skip_loading_cfg)
        return false;

    // manual reconf (FIXME: check status)
    conv_ok = false;

    // hide toolbar
    bool_val = m_settings->value("gui/hide_toolbar", false).toBool();
    if (bool_val)
        ui->mainToolBar->hide();

    // main window settings
    if (restore_mainwindow)
    {
        restoreGeometry(m_settings->value("gui/geometry",
                                          saveGeometry()).toByteArray());
        restoreState(m_settings->value("gui/state", saveState()).toByteArray());
    }

    QString indev = m_settings->value("input/device", "").toString();
    if (!indev.isEmpty())
    {
        try
        {
            rx->set_input_device(indev.toStdString());
            conf_ok = true;
        }
        catch (std::runtime_error &x)
        {
            QMessageBox::warning(nullptr,
                             QObject::tr("Failed to set input device"),
                             QObject::tr("<p><b>%1</b></p>"
                                         "Please select another device.")
                                     .arg(x.what()),
                             QMessageBox::Ok);
        }

        // Update window title
        setWindowTitle(QString("Gqrx %1 - %2").arg(VERSION).arg(indev));

        // Add available antenna connectors to the UI
        std::vector<std::string> antennas = rx->get_antennas();
        uiDockInputCtl->setAntennas(antennas);

        // Update gain stages.
        if (indev.contains("rtl", Qt::CaseInsensitive)
                && !m_settings->contains("input/gains"))
        {
            /* rtlsdr gain is 0 by default making users think their device is
             * deaf. Therefore, we don't read gain from the device, but initialize
             * it to the midpoint.
             */
            updateGainStages(false);
        }
        else
            updateGainStages(true);
    }

    QString outdev = m_settings->value("output/device", "").toString();

    try {
        rx->set_output_device(outdev.toStdString());
    } catch (std::exception &x) {
        QMessageBox::warning(nullptr,
                         QObject::tr("Failed to set output device"),
                         QObject::tr("<p><b>%1</b></p>"
                                     "Please select another device.")
                                 .arg(x.what()),
                         QMessageBox::Ok);
    }

    int_val = m_settings->value("input/sample_rate", 0).toInt(&conv_ok);
    if (conv_ok && (int_val > 0))
    {
        actual_rate = rx->set_input_rate(int_val);

        if (actual_rate == 0)
        {
            // There is an error with the device (perhaps not attached)
            // Warn user and use 100 ksps (rate used by gr-osmocom null_source)
            auto *dialog =
                    new QMessageBox(QMessageBox::Warning, tr("Device Error"),
                                    tr("There was an error configuring the input device.\n"
                                       "Please make sure that a supported device is attached "
                                       "to the computer and restart gqrx."),
                                    QMessageBox::Ok);
            dialog->setModal(true);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();

            actual_rate = int_val;
        }

        qDebug() << "Requested sample rate:" << int_val;
        qDebug() << "Actual sample rate   :" << QString("%1").arg(actual_rate, 0, 'f', 6);
    }
    else
        actual_rate = rx->get_input_rate();

    if (actual_rate > 0.)
    {
        int_val = m_settings->value("input/decimation", 1).toInt(&conv_ok);
        if (conv_ok && int_val >= 2)
        {
            if (rx->set_input_decim(int_val) != (unsigned int)int_val)
            {
                qDebug() << "Failed to set decimation" << int_val;
                qDebug() << "  actual decimation:" << rx->get_input_decim();
            }
            else
            {
                // update actual rate
                actual_rate /= (double)int_val;
                qDebug() << "Input decimation:" << int_val;
                qDebug() << "Quadrature rate:" << QString("%1").arg(actual_rate, 0, 'f', 6);
            }
        }
        else
            rx->set_input_decim(1);

        // update various widgets that need a sample rate
        uiDockRxOpt->setFilterOffsetRange((qint64)(actual_rate));
        uiDockFft->setSampleRate(actual_rate);
        ui->plotter->setSampleRate(actual_rate);
        ui->plotter->setSpanFreq((quint32)actual_rate);
        remote->setBandwidth((qint64)actual_rate);
        iq_tool->setSampleRate((qint64)actual_rate);
    }
    else
        qDebug() << "Error: Actual sample rate is" << actual_rate;

    int64_val = m_settings->value("input/bandwidth", 0).toInt(&conv_ok);
    if (conv_ok)
    {
        // set analog bw even if 0 since for some devices 0 Hz means "auto"
        double actual_bw = rx->set_analog_bandwidth((double) int64_val);
        qDebug() << "Requested bandwidth:" << int64_val << "Hz";
        qDebug() << "Actual bandwidth   :" << actual_bw << "Hz";
    }

    uiDockInputCtl->readSettings(m_settings); // this will also update freq range
    uiDockRxOpt->readSettings(m_settings);
    uiDockFft->readSettings(m_settings);
    uiDockAudio->readSettings(m_settings);
    dxc_options->readSettings(m_settings);

    {
        int64_val = m_settings->value("input/frequency", 14236000).toLongLong(&conv_ok);

        // If frequency is out of range set frequency to the center of the range.
        qint64 hw_freq = int64_val - d_lnb_lo - (qint64)(rx->get_filter_offset());
        if (hw_freq < d_hw_freq_start || hw_freq > d_hw_freq_stop)
        {
            int64_val = (d_hw_freq_stop - d_hw_freq_start) / 2 +
                        (qint64)(rx->get_filter_offset()) + d_lnb_lo;
        }

        ui->freqCtrl->setFrequency(int64_val);
        setNewFrequency(ui->freqCtrl->getFrequency()); // ensure all GUI and RF is updated
    }

    {
        int flo = m_settings->value("receiver/filter_low_cut", 0).toInt(&conv_ok);
        int fhi = m_settings->value("receiver/filter_high_cut", 0).toInt(&conv_ok);

        if (conv_ok && uiDockRxOpt->currentDemod() != DockRxOpt::MODE_OFF && flo != fhi)
        {
            on_plotter_newFilterFreq(flo, fhi);
        }
    }

    iq_tool->readSettings(m_settings);

    /*
     * Initialization the remote control at the end.
     * We must be sure that all variables initialized before starting RC server.
     */
    remote->readSettings(m_settings);
    bool_val = m_settings->value("remote_control/enabled", false).toBool();
    if (bool_val)
    {
       remote->start_server();
       ui->actionRemoteControl->setChecked(true);
    }

    emit m_recent_config->configLoaded(m_settings->fileName());

    return conf_ok;
}

/**
 * @brief Save current configuration to a file.
 * @param cfgfile
 * @returns True if the operation was successful.
 *
 * If cfgfile is an absolute path it will be used as is, otherwise it is
 * assumed to be the name of a file under m_cfg_dir.
 *
 * If cfgfile already exists it will be overwritten (we assume that a file
 * selection dialog has already asked for confirmation of overwrite).
 *
 * Since QSettings does not support "save as" we do this by copying the current
 * settings to a new file.
 */
bool MainWindow::saveConfig(const QString& cfgfile)
{
    QString oldfile = m_settings->fileName();
    QString newfile;

    qDebug() << "Saving configuration to:" << cfgfile;

    m_settings->sync();

    if (QDir::isAbsolutePath(cfgfile))
        newfile = cfgfile;
    else
        newfile = QString("%1/%2").arg(m_cfg_dir).arg(cfgfile);

    if (newfile == oldfile) {
        qDebug() << "New file is equal to old file => SYNCING...";
        emit m_recent_config->configSaved(newfile);
        return true;
    }

    if (QFile::exists(newfile))
    {
        qDebug() << "File" << newfile << "already exists => DELETING...";
        if (QFile::remove(newfile))
            qDebug() << "Deleted" << newfile;
        else
            qDebug() << "Failed to delete" << newfile;
    }
    if (QFile::copy(oldfile, newfile))
    {
        loadConfig(cfgfile, false, false);
        return true;
    }
    else
    {
        qDebug() << "Error saving configuration to" << newfile;
        return false;
    }
}

/**
 * Store session-related parameters (frequency, gain,...)
 *
 * This needs to be called when we switch input source, otherwise the
 * new source would use the parameters stored on last exit.
 */
void MainWindow::storeSession()
{
    if (m_settings)
    {
        m_settings->setValue("input/frequency", ui->freqCtrl->getFrequency());

        uiDockInputCtl->saveSettings(m_settings);
        uiDockRxOpt->saveSettings(m_settings);
        uiDockFft->saveSettings(m_settings);
        uiDockAudio->saveSettings(m_settings);

        remote->saveSettings(m_settings);
        iq_tool->saveSettings(m_settings);
        dxc_options->saveSettings(m_settings);

        {
            int     flo, fhi;
            ui->plotter->getHiLowCutFrequencies(&flo, &fhi);
            if (flo != fhi)
            {
                m_settings->setValue("receiver/filter_low_cut", flo);
                m_settings->setValue("receiver/filter_high_cut", fhi);
            }
        }
    }
}

/**
 * @brief Update hardware RF frequency range.
 * @param ignore_limits Whether ignore the hardware specd and allow DC-to-light
 *                      range.
 *
 * This function fetches the frequency range of the receiver. Useful when we
 * read a new configuration with a new input device or when the ignore_limits
 * setting is changed.
 */
void MainWindow::updateHWFrequencyRange(bool ignore_limits)
{
    double startd, stopd, stepd;

    if (ignore_limits)
    {
        d_hw_freq_start = (quint64) 0;
        d_hw_freq_stop  = (quint64) 9999e6;
    }
    else if (rx->get_rf_range(&startd, &stopd, &stepd) == receiver::STATUS_OK)
    {
        d_hw_freq_start = (quint64) startd;
        d_hw_freq_stop  = (quint64) stopd;
    }
    else
    {
        qDebug() << __func__ << "failed fetching new hardware frequency range";
        d_hw_freq_start = (quint64) 0;
        d_hw_freq_stop  = (quint64) 9999e6;
    }

    updateFrequencyRange(); // Also update the available frequency range
}

/**
 * @brief Update available frequency range.
 *
 * This function sets the available frequency range based on the hardware
 * frequency range, the selected filter offset and the LNB LO.
 *
 * This function must therefore be called whenever the LNB LO or the filter
 * offset has changed.
 */
void MainWindow::updateFrequencyRange()
{
    auto start = (qint64)(rx->get_filter_offset()) + d_hw_freq_start + d_lnb_lo;
    auto stop  = (qint64)(rx->get_filter_offset()) + d_hw_freq_stop  + d_lnb_lo;

    ui->freqCtrl->setup(0, start, stop, 1, FCTL_UNIT_NONE);
    uiDockRxOpt->setRxFreqRange(start, stop);
}

/**
 * @brief Update gain stages.
 * @param read_from_device If true, the gain value will be read from the device,
 *                         otherwise we set gain to the midpoint.
 *
 * This function fetches a list of available gain stages with their range
 * and sends them to the input control UI widget.
 */
void MainWindow::updateGainStages(bool read_from_device)
{
    gain_list_t gain_list;
    std::vector<std::string> gain_names = rx->get_gain_names();
    gain_t gain;

    std::vector<std::string>::iterator it;
    for (it = gain_names.begin(); it != gain_names.end(); ++it)
    {
        gain.name = *it;
        rx->get_gain_range(gain.name, &gain.start, &gain.stop, &gain.step);
        if (read_from_device)
        {
            gain.value = rx->get_gain(gain.name);
        }
        else
        {
            gain.value = (gain.start + gain.stop) / 2;
            rx->set_gain(gain.name, gain.value);
        }
        gain_list.push_back(gain);
    }

    uiDockInputCtl->setGainStages(gain_list);
    remote->setGainStages(gain_list);
}

/**
 * @brief Slot for receiving frequency change signals.
 * @param[in] freq The new frequency.
 *
 * This slot is connected to the CFreqCtrl::newFrequency() signal and is used
 * to set new receive frequency.
 */
void MainWindow::setNewFrequency(qint64 rx_freq)
{
    auto hw_freq = (double)(rx_freq - d_lnb_lo) - rx->get_filter_offset();
    auto center_freq = rx_freq - (qint64)rx->get_filter_offset();

    d_hw_freq = (qint64)hw_freq;

    // set receiver frequency
    rx->set_rf_freq(hw_freq);

    // update widgets
    ui->plotter->setCenterFreq(center_freq);
    uiDockRxOpt->setHwFreq(d_hw_freq);
    ui->freqCtrl->setFrequency(rx_freq);
    uiDockBookmarks->setNewFrequency(rx_freq);
}

// Update delta and center (of marker span) when markers are updated
void MainWindow::updateDeltaAndCenter()
{
    if (d_marker_a != MARKER_OFF && d_marker_b != MARKER_OFF)
    {
        qint64 delta = d_marker_b - d_marker_a;
        qint64 center = (d_marker_a + d_marker_b) / 2;
        ui->deltaFreqLabel->setText(QString("Δ%1 kHz   ⨏%2 kHz")
                            .arg(locale().toString(delta/1.e3, 'f', 3))
                            .arg(locale().toString(center/1.e3, 'f', 3)));
    }
    else {
        ui->deltaFreqLabel->setText("");
    }
}

// Set marker via slots
void MainWindow::setMarkerA(qint64 freq)
{
    d_marker_a = freq;
    if (freq != MARKER_OFF)
    {
        ui->markerLabelA->setText(QString("%1 kHz").arg(locale().toString(freq/1.e3, 'f', 3)));
    }
    else {
        ui->markerLabelA->setText("");
    }
    ui->plotter->setMarkers(d_marker_a, d_marker_b);
    updateDeltaAndCenter();
}

void MainWindow::setMarkerB(qint64 freq)
{
    d_marker_b = freq;
    if (freq != MARKER_OFF)
    {
        ui->markerLabelB->setText(QString("%1 kHz").arg(locale().toString(freq/1.e3, 'f', 3)));
    }
    else {
        ui->markerLabelB->setText("");
    }
    ui->plotter->setMarkers(d_marker_a, d_marker_b);
    updateDeltaAndCenter();
}

// Set marker via buttons
void MainWindow::on_setMarkerButtonA_clicked()
{
    auto center_freq = d_hw_freq + (qint64)rx->get_filter_offset();
    setMarkerA(center_freq + d_lnb_lo);
}

void MainWindow::on_setMarkerButtonB_clicked()
{
    auto center_freq = d_hw_freq + (qint64)rx->get_filter_offset();
    setMarkerB(center_freq + d_lnb_lo);
}

void MainWindow::on_clearMarkerButtonA_clicked()
{
    setMarkerA(MARKER_OFF);
}

void MainWindow::on_clearMarkerButtonB_clicked()
{
    setMarkerB(MARKER_OFF);
}

/**
 * @brief Set new LNB LO frequency.
 * @param freq_mhz The new frequency in MHz.
 */
void MainWindow::setLnbLo(double freq_mhz)
{
    // calculate current RF frequency
    auto rf_freq = ui->freqCtrl->getFrequency() - d_lnb_lo;

    d_lnb_lo = qint64(freq_mhz*1e6);
    qDebug() << "New LNB LO:" << d_lnb_lo << "Hz";

    // Update ranges and show updated frequency
    updateFrequencyRange();
    ui->freqCtrl->setFrequency(d_lnb_lo + rf_freq);
    ui->plotter->setCenterFreq(d_lnb_lo + d_hw_freq);

    // update LNB LO in settings
    if (freq_mhz == 0.f)
        m_settings->remove("input/lnb_lo");
    else
        m_settings->setValue("input/lnb_lo", d_lnb_lo);
}

/** Select new antenna connector. */
void MainWindow::setAntenna(const QString& antenna)
{
    qDebug() << "New antenna selected:" << antenna;
    rx->set_antenna(antenna.toStdString());
}

/**
 * @brief Set new channel filter offset.
 * @param freq_hz The new filter offset in Hz.
 */
void MainWindow::setFilterOffset(qint64 freq_hz)
{
    rx->set_filter_offset((double) freq_hz);
    ui->plotter->setFilterOffset(freq_hz);

    updateFrequencyRange();

    auto rx_freq = d_hw_freq + d_lnb_lo + freq_hz;
    ui->freqCtrl->setFrequency(rx_freq);

    if (rx->is_rds_decoder_active()) {
        rx->reset_rds_parser();
    }
}

/**
 * @brief Set a specific gain.
 * @param name The name of the gain stage to adjust.
 * @param gain The new value.
 */
void MainWindow::setGain(const QString& name, double gain)
{
    rx->set_gain(name.toStdString(), gain);
}

/** Enable / disable hardware AGC. */
void MainWindow::setAutoGain(bool enabled)
{
    rx->set_auto_gain(enabled);
    if (!enabled)
        uiDockInputCtl->restoreManualGains();
}

/**
 * @brief Set new frequency offset value.
 * @param ppm Frequency correction.
 *
 * The valid range is between -200 and 200.
 */
void MainWindow::setFreqCorr(double ppm)
{
    if (ppm < -200.0)
        ppm = -200.0;
    else if (ppm > 200.0)
        ppm = 200.0;

    qDebug() << __FUNCTION__ << ":" << ppm << "ppm";
    rx->set_freq_corr(ppm);
}


/** Enable/disable I/Q reversion. */
void MainWindow::setIqSwap(bool reversed)
{
    rx->set_iq_swap(reversed);
}

/** Enable/disable automatic DC removal. */
void MainWindow::setDcCancel(bool enabled)
{
    rx->set_dc_cancel(enabled);
}

/** Enable/disable automatic IQ balance. */
void MainWindow::setIqBalance(bool enabled)
{
    try
    {
        rx->set_iq_balance(enabled);
    }
    catch (std::exception &x)
    {
        qCritical() << "Failed to set IQ balance: " << x.what();
        m_settings->remove("input/iq_balance");
        uiDockInputCtl->setIqBalance(false);
        if (enabled)
        {
            QMessageBox::warning(this, tr("Gqrx error"),
                                 tr("Failed to set IQ balance.\n"
                                    "IQ balance setting in Input Control disabled."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

/**
 * @brief Ignore hardware limits.
 * @param ignore_limits Whether hardware limits should be ignored or not.
 *
 * This slot is triggered when the user changes the "Ignore hardware limits"
 * option. It will update the allowed frequency range and also update the
 * current RF center frequency, which may change when we switch from ignore to
 * don't ignore.
 */
void MainWindow::setIgnoreLimits(bool ignore_limits)
{
    updateHWFrequencyRange(ignore_limits);

    auto filter_offset = (qint64)rx->get_filter_offset();
    auto freq = (qint64)rx->get_rf_freq();
    ui->freqCtrl->setFrequency(d_lnb_lo + freq + filter_offset);

    // This will ensure that if frequency is clamped and that
    // the UI is updated with the correct frequency.
    freq = ui->freqCtrl->getFrequency();
    setNewFrequency(freq);
}


/** Reset lower digits of main frequency control widget */
void MainWindow::setFreqCtrlReset(bool enabled)
{
    ui->freqCtrl->setResetLowerDigits(enabled);
    uiDockRxOpt->setResetLowerDigits(enabled);
}

/** Invert scroll wheel direction */
void MainWindow::setInvertScrolling(bool enabled)
{
    ui->freqCtrl->setInvertScrolling(enabled);
    ui->plotter->setInvertScrolling(enabled);
    uiDockRxOpt->setInvertScrolling(enabled);
    uiDockAudio->setInvertScrolling(enabled);
}

/**
 * @brief Select new demodulator.
 * @param demod New demodulator.
 */
void MainWindow::selectDemod(const QString& strModulation)
{
    int iDemodIndex;

    iDemodIndex = DockRxOpt::GetEnumForModulationString(strModulation);
    qDebug() << "selectDemod(str):" << strModulation << "-> IDX:" << iDemodIndex;

    return selectDemod(iDemodIndex);
}

/**
 * @brief Select new demodulator.
 * @param demod New demodulator index.
 *
 * This slot basically maps the index of the mode selector to receiver::demod
 * and configures the default channel filter.
 *
 */
void MainWindow::selectDemod(int mode_idx)
{
    double  cwofs = 0.0;
    int     filter_preset = uiDockRxOpt->currentFilter();
    int     flo=0, fhi=0, click_res=100;
    bool    rds_enabled;

    // validate mode_idx
    if (mode_idx < DockRxOpt::MODE_OFF || mode_idx >= DockRxOpt::MODE_LAST)
    {
        qDebug() << "Invalid mode index:" << mode_idx;
        mode_idx = DockRxOpt::MODE_OFF;
    }
    qDebug() << "New mode index:" << mode_idx;

    uiDockRxOpt->getFilterPreset(mode_idx, filter_preset, &flo, &fhi);
    d_filter_shape = (receiver::filter_shape)uiDockRxOpt->currentFilterShape();

    rds_enabled = rx->is_rds_decoder_active();
    if (rds_enabled)
        setRdsDecoder(false);
    uiDockRDS->setDisabled();

    switch (mode_idx) {

    case DockRxOpt::MODE_OFF:
        /* Spectrum analyzer only */
        if (rx->is_recording_audio())
        {
            stopAudioRec();
            uiDockAudio->setAudioRecButtonState(false);
        }
        if (dec_afsk1200 != nullptr)
        {
            dec_afsk1200->close();
        }
        rx->set_demod(receiver::RX_DEMOD_OFF);
        click_res = 1000;
        break;

    case DockRxOpt::MODE_RAW:
        /* Raw I/Q; max 96 ksps*/
        rx->set_demod(receiver::RX_DEMOD_NONE);
        ui->plotter->setDemodRanges(-40000, -200, 200, 40000, true);
        uiDockAudio->setFftRange(0,24000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_AM:
        rx->set_demod(receiver::RX_DEMOD_AM);
        rx->set_am_dcr(uiDockRxOpt->currentAmDcr());
        ui->plotter->setDemodRanges(-40000, -200, 200, 40000, true);
        uiDockAudio->setFftRange(0,6000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_AM_SYNC:
        rx->set_demod(receiver::RX_DEMOD_AMSYNC);
        rx->set_amsync_dcr(uiDockRxOpt->currentAmsyncDcr());
        rx->set_amsync_pll_bw(uiDockRxOpt->currentAmsyncPll());
        ui->plotter->setDemodRanges(-40000, -200, 200, 40000, true);
        uiDockAudio->setFftRange(0,6000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_NFM:
        ui->plotter->setDemodRanges(-40000, -1000, 1000, 40000, true);
        uiDockAudio->setFftRange(0, 5000);
        rx->set_demod(receiver::RX_DEMOD_NFM);
        rx->set_fm_maxdev(uiDockRxOpt->currentMaxdev());
        rx->set_fm_deemph(uiDockRxOpt->currentEmph());
        click_res = 100;
        break;

    case DockRxOpt::MODE_WFM_MONO:
    case DockRxOpt::MODE_WFM_STEREO:
    case DockRxOpt::MODE_WFM_STEREO_OIRT:
        /* Broadcast FM */
        ui->plotter->setDemodRanges(-120e3, -10000, 10000, 120e3, true);
        uiDockAudio->setFftRange(0,24000);  /** FIXME: get audio rate from rx **/
        click_res = 1000;
        if (mode_idx == DockRxOpt::MODE_WFM_MONO)
            rx->set_demod(receiver::RX_DEMOD_WFM_M);
        else if (mode_idx == DockRxOpt::MODE_WFM_STEREO_OIRT)
            rx->set_demod(receiver::RX_DEMOD_WFM_S_OIRT);
        else
            rx->set_demod(receiver::RX_DEMOD_WFM_S);

        uiDockRDS->setEnabled();
        if (rds_enabled)
            setRdsDecoder(true);
        break;

    case DockRxOpt::MODE_LSB:
        /* LSB */
        rx->set_demod(receiver::RX_DEMOD_SSB);
        ui->plotter->setDemodRanges(-40000, -100, -5000, 0, false);
        uiDockAudio->setFftRange(0,3000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_USB:
        /* USB */
        rx->set_demod(receiver::RX_DEMOD_SSB);
        ui->plotter->setDemodRanges(0, 5000, 100, 40000, false);
        uiDockAudio->setFftRange(0,3000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_CWL:
        /* CW-L */
        rx->set_demod(receiver::RX_DEMOD_SSB);
        cwofs = -uiDockRxOpt->getCwOffset();
        ui->plotter->setDemodRanges(-5000, -100, 100, 5000, true);
        uiDockAudio->setFftRange(0,1500);
        click_res = 10;
        break;

    case DockRxOpt::MODE_CWU:
        /* CW-U */
        rx->set_demod(receiver::RX_DEMOD_SSB);
        cwofs = uiDockRxOpt->getCwOffset();
        ui->plotter->setDemodRanges(-5000, -100, 100, 5000, true);
        uiDockAudio->setFftRange(0,1500);
        click_res = 10;
        break;

    default:
        qDebug() << "Unsupported mode selection (can't happen!): " << mode_idx;
        flo = -5000;
        fhi = 5000;
        click_res = 100;
        break;
    }

    qDebug() << "Filter preset for mode" << mode_idx << "LO:" << flo << "HI:" << fhi;
    ui->plotter->setHiLowCutFrequencies(flo, fhi);
    ui->plotter->setClickResolution(click_res);
    ui->plotter->setFilterClickResolution(click_res);
    rx->set_filter((double)flo, (double)fhi, d_filter_shape);
    rx->set_cw_offset(cwofs);
    rx->set_sql_level(uiDockRxOpt->currentSquelchLevel());

    remote->setMode(mode_idx);
    remote->setPassband(flo, fhi);

    d_have_audio = (mode_idx != DockRxOpt::MODE_OFF);

    uiDockRxOpt->setCurrentDemod(mode_idx);
}


/**
 * @brief New FM deviation selected.
 * @param max_dev The enw FM deviation.
 */
void MainWindow::setFmMaxdev(float max_dev)
{
    qDebug() << "FM MAX_DEV: " << max_dev;

    /* receiver will check range */
    rx->set_fm_maxdev(max_dev);
}


/**
 * @brief New FM de-emphasis time consant selected.
 * @param tau The new time constant
 */
void MainWindow::setFmEmph(double tau)
{
    qDebug() << "FM TAU: " << tau;

    /* receiver will check range */
    rx->set_fm_deemph(tau);
}


/**
 * @brief AM DCR status changed (slot).
 * @param enabled Whether DCR is enabled or not.
 */
void MainWindow::setAmDcr(bool enabled)
{
    rx->set_am_dcr(enabled);
}

void MainWindow::setCwOffset(int offset)
{
    rx->set_cw_offset(offset);
}

/**
 * @brief AM-Sync DCR status changed (slot).
 * @param enabled Whether DCR is enabled or not.
 */
void MainWindow::setAmSyncDcr(bool enabled)
{
    rx->set_amsync_dcr(enabled);
}

/**
 * @brief New AM-Sync PLL BW selected.
 * @param pll_bw The new PLL BW.
 */
void MainWindow::setAmSyncPllBw(float pll_bw)
{
    qDebug() << "AM-Sync PLL BW: " << pll_bw;

    /* receiver will check range */
    rx->set_amsync_pll_bw(pll_bw);
}

/**
 * @brief Audio gain changed.
 * @param value The new audio gain in dB.
 */
void MainWindow::setAudioGain(float value)
{
    rx->set_af_gain(value);
}

/** Set AGC ON/OFF. */
void MainWindow::setAgcOn(bool agc_on)
{
    rx->set_agc_on(agc_on);
}

/** AGC hang ON/OFF. */
void MainWindow::setAgcHang(bool use_hang)
{
    rx->set_agc_hang(use_hang);
}

/** AGC threshold changed. */
void MainWindow::setAgcThreshold(int threshold)
{
    rx->set_agc_threshold(threshold);
}

/** AGC slope factor changed. */
void MainWindow::setAgcSlope(int factor)
{
    rx->set_agc_slope(factor);
}

/** AGC manual gain changed. */
void MainWindow::setAgcGain(int gain)
{
    rx->set_agc_manual_gain(gain);
}

/** AGC decay changed. */
void MainWindow::setAgcDecay(int msec)
{
    rx->set_agc_decay(msec);
}

/**
 * @brief Noise blanker configuration changed.
 * @param nb1 Noise blanker 1 ON/OFF.
 * @param nb2 Noise blanker 2 ON/OFF.
 * @param threshold Noise blanker threshold.
 */
void MainWindow::setNoiseBlanker(int nbid, bool on, float threshold)
{
    qDebug() << "Noise blanker NB:" << nbid << " ON:" << on << "THLD:"
             << threshold;

    rx->set_nb_on(nbid, on);
    rx->set_nb_threshold(nbid, threshold);
}

/**
 * @brief Squelch level changed.
 * @param level_db The new squelch level in dBFS.
 */
void MainWindow::setSqlLevel(double level_db)
{
    rx->set_sql_level(level_db);
    ui->sMeter->setSqlLevel(level_db);
}

/**
 * @brief Squelch level auto clicked.
 * @return The new squelch level.
 */
double MainWindow::setSqlLevelAuto()
{
    double level = rx->get_signal_pwr() + 3.0;
    if (level > -10.0)  // avoid 0 dBFS
        level = uiDockRxOpt->getSqlLevel();

    setSqlLevel(level);
    return level;
}

/** Signal strength meter timeout. */
void MainWindow::meterTimeout()
{
    float level;

    level = rx->get_signal_pwr();
    ui->sMeter->setLevel(level);
    remote->setSignalLevel(level);
}

/** Baseband FFT plot timeout. */
void MainWindow::iqFftTimeout()
{
    unsigned int    fftsize;
    unsigned int    i;

    // FIXME: fftsize is a reference
    rx->get_iq_fft_data(d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        return;
    }

    // Shifted mag^2(FFT)
    for (i = 0; i < fftsize/2; ++i)
        d_realFftData[i] = static_cast<float>(std::norm(d_fftData[i + fftsize/2]));
    for (i = fftsize/2; i < fftsize; ++i)
        d_realFftData[i] = static_cast<float>(std::norm(d_fftData[i - fftsize/2]));

    ui->plotter->setNewFftData(d_realFftData, fftsize);
}

/** Audio FFT plot timeout. */
void MainWindow::audioFftTimeout()
{
    unsigned int    fftsize;
    unsigned int    i;

    if (!d_have_audio || !uiDockAudio->isVisible())
        return;

    rx->get_audio_fft_data(d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        qDebug() << "No audio FFT data.";
        return;
    }

    for (i = 0; i < fftsize/2; ++i)
        d_realFftData[i] = static_cast<float>(std::norm(d_fftData[i + fftsize/2]));
    for (i = fftsize/2; i < fftsize; ++i)
        d_realFftData[i] = static_cast<float>(std::norm(d_fftData[i - fftsize/2]));

    uiDockAudio->setNewFftData(d_realFftData, fftsize);
}

/** RDS message display timeout. */
void MainWindow::rdsTimeout()
{
    std::string buffer;
    int num;

    rx->get_rds_data(buffer, num);
    while(num!=-1) {
        uiDockRDS->updateRDS(QString::fromStdString(buffer), num);
        rx->get_rds_data(buffer, num);
    }
}

/**
 * @brief Start audio recorder.
 * @param filename The file name into which audio should be recorded.
 */
void MainWindow::startAudioRec(const QString& filename)
{
    if (!d_have_audio)
    {
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("Recording audio requires a demodulator.\n"
                           "Currently, demodulation is switched off "
                           "(Mode->Demod Off)."));
        msg_box.exec();
        uiDockAudio->setAudioRecButtonState(false);
    }
    else if (rx->start_audio_recording(filename.toStdString()))
    {
        ui->statusBar->showMessage(tr("Error starting audio recorder"));

        /* reset state of record button */
        uiDockAudio->setAudioRecButtonState(false);
    }
    else
    {
        ui->statusBar->showMessage(tr("Recording audio to %1").arg(filename));
    }
}

/** Stop audio recorder. */
void MainWindow::stopAudioRec()
{
    if (rx->stop_audio_recording())
    {
        /* okay, this one would be weird if it really happened */
        ui->statusBar->showMessage(tr("Error stopping audio recorder"));

        uiDockAudio->setAudioRecButtonState(true);
    }
    else
    {
        ui->statusBar->showMessage(tr("Audio recorder stopped"), 5000);
    }
}


/** Start playback of audio file. */
void MainWindow::startAudioPlayback(const QString& filename)
{
    if (rx->start_audio_playback(filename.toStdString()))
    {
        ui->statusBar->showMessage(tr("Error trying to play %1").arg(filename));

        /* reset state of record button */
        uiDockAudio->setAudioPlayButtonState(false);
    }
    else
    {
        ui->statusBar->showMessage(tr("Playing %1").arg(filename));
    }
}

/** Stop playback of audio file. */
void MainWindow::stopAudioPlayback()
{
    if (rx->stop_audio_playback())
    {
        /* okay, this one would be weird if it really happened */
        ui->statusBar->showMessage(tr("Error stopping audio playback"));

        uiDockAudio->setAudioPlayButtonState(true);
    }
    else
    {
        ui->statusBar->showMessage(tr("Audio playback stopped"), 5000);
    }
}

/** Start streaming audio over UDP. */
void MainWindow::startAudioStream(const QString& udp_host, int udp_port, bool stereo)
{
    rx->start_udp_streaming(udp_host.toStdString(), udp_port, stereo);
}

/** Stop streaming audio over UDP. */
void MainWindow::stopAudioStreaming()
{
    rx->stop_udp_streaming();
}

/** Start I/Q recording. */
void MainWindow::startIqRecording(const QString& recdir)
{
    qDebug() << __func__;
    // generate file name using date, time, rf freq in kHz and BW in Hz
    // gqrx_iq_yyyymmdd_hhmmss_freq_bw_fc.raw
    auto freq = (qint64)(rx->get_rf_freq());
    auto sr = (qint64)(rx->get_input_rate());
    auto dec = (quint32)(rx->get_input_decim());
    auto lastRec = QDateTime::currentDateTimeUtc().
            toString("%1/gqrx_yyyyMMdd_hhmmss_%2_%3_fc.'raw'")
            .arg(recdir).arg(freq).arg(sr/dec);

    // start recorder; fails if recording already in progress
    if (rx->start_iq_recording(lastRec.toStdString()))
    {
        // reset action status
        ui->statusBar->showMessage(tr("Error starting I/Q recoder"));

        // show an error message to user
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("There was an error starting the I/Q recorder.\n"
                           "Check write permissions for the selected location."));
        msg_box.exec();

    }
    else
    {
        ui->statusBar->showMessage(tr("Recording I/Q data to: %1").arg(lastRec),
                                   5000);
    }
}

/** Stop current I/Q recording. */
void MainWindow::stopIqRecording()
{
    qDebug() << __func__;

    if (rx->stop_iq_recording())
        ui->statusBar->showMessage(tr("Error stopping I/Q recoder"));
    else
        ui->statusBar->showMessage(tr("I/Q data recoding stopped"), 5000);
}

void MainWindow::startIqPlayback(const QString& filename, float samprate, qint64 center_freq)
{
    if (ui->actionDSP->isChecked())
    {
        // suspend DSP while we reload settings
        on_actionDSP_triggered(false);
    }

    storeSession();

    auto sri = (int)samprate;
    auto cf  = center_freq;
    double current_offset = rx->get_filter_offset();
    QString escapedFilename = receiver::escape_filename(filename.toStdString()).c_str();
    auto devstr = QString("file=%1,rate=%2,freq=%3,throttle=true,repeat=false")
            .arg(escapedFilename).arg(sri).arg(cf);

    qDebug() << __func__ << ":" << devstr;

    rx->set_input_device(devstr.toStdString());
    updateHWFrequencyRange(false);

    // sample rate
    auto actual_rate = rx->set_input_rate(samprate);
    qDebug() << "Requested sample rate:" << samprate;
    qDebug() << "Actual sample rate   :" << QString("%1")
                .arg(actual_rate, 0, 'f', 6);

    uiDockRxOpt->setFilterOffsetRange((qint64)(actual_rate));
    ui->plotter->setSampleRate(actual_rate);
    ui->plotter->setSpanFreq((quint32)actual_rate);
    if (std::abs(current_offset) > actual_rate / 2)
        on_plotter_newDemodFreq(center_freq, 0);
    else
        on_plotter_newDemodFreq(center_freq + current_offset, current_offset);

    remote->setBandwidth(actual_rate);

    // FIXME: would be nice with good/bad status
    ui->statusBar->showMessage(tr("Playing %1").arg(filename));

    on_actionDSP_triggered(true);
}

void MainWindow::stopIqPlayback()
{
    if (ui->actionDSP->isChecked())
    {
        // suspend DSP while we reload settings
        on_actionDSP_triggered(false);
    }

    ui->statusBar->showMessage(tr("I/Q playback stopped"), 5000);

    // restore original input device
    auto indev = m_settings->value("input/device", "").toString();
    rx->set_input_device(indev.toStdString());

    // restore sample rate
    bool conv_ok;
    auto sr = m_settings->value("input/sample_rate", 0).toInt(&conv_ok);
    if (conv_ok && (sr > 0))
    {
        auto actual_rate = rx->set_input_rate(sr);
        qDebug() << "Requested sample rate:" << sr;
        qDebug() << "Actual sample rate   :" << QString("%1")
                    .arg(actual_rate, 0, 'f', 6);

        uiDockRxOpt->setFilterOffsetRange((qint64)(actual_rate));
        ui->plotter->setSampleRate(actual_rate);
        ui->plotter->setSpanFreq((quint32)actual_rate);
        remote->setBandwidth(sr);

        // not needed as long as we are not recording in iq_tool
        //iq_tool->setSampleRate(sr);
    }

    // restore frequency, gain, etc...
    uiDockInputCtl->readSettings(m_settings);
    bool centerOK = false;
    bool offsetOK = false;
    qint64 oldCenter = m_settings->value("input/frequency", 0).toLongLong(&centerOK);
    qint64 oldOffset = m_settings->value("receiver/offset", 0).toLongLong(&offsetOK);
    if (centerOK && offsetOK)
    {
        on_plotter_newDemodFreq(oldCenter, oldOffset);
    }

    if (ui->actionDSP->isChecked())
    {
        // restsart DSP
        on_actionDSP_triggered(true);
    }
}


/**
 * Go to a specific offset in the IQ file.
 * @param seek_pos The byte offset from the beginning of the file.
 */
void MainWindow::seekIqFile(qint64 seek_pos)
{
    rx->seek_iq_file((long)seek_pos);
}

/** FFT size has changed. */
void MainWindow::setIqFftSize(int size)
{
    qDebug() << "Changing baseband FFT size to" << size;
    rx->set_iq_fft_size(size);
}

/** Baseband FFT rate has changed. */
void MainWindow::setIqFftRate(int fps)
{
    int interval;

    d_fps = fps;

    if (fps == 0)
    {
        interval = 36e7; // 100 hours
        ui->plotter->setRunningState(false);
    }
    else
    {
        interval = 1000 / fps;

        ui->plotter->setFftRate(fps);
        if (iq_fft_timer->isActive())
            ui->plotter->setRunningState(true);
    }

    // Limit to 250 fps
    if (interval > 3 && iq_fft_timer->isActive())
        iq_fft_timer->setInterval(interval);

    uiDockFft->setWfResolution(ui->plotter->getWfTimeRes());
}

void MainWindow::setIqFftWindow(int type)
{
    rx->set_iq_fft_window(type);
}

/** Waterfall time span has changed. */
void MainWindow::setWfTimeSpan(quint64 span_ms)
{
    // set new time span, then send back new resolution to be shown by GUI label
    ui->plotter->setWaterfallSpan(span_ms);
    uiDockFft->setWfResolution(ui->plotter->getWfTimeRes());
}

void MainWindow::setWfSize()
{
    uiDockFft->setWfResolution(ui->plotter->getWfTimeRes());
}

/**
 * @brief Vertical split between waterfall and pandapter changed.
 * @param pct_pand The percentage of the waterfall.
 */
void MainWindow::setIqFftSplit(int pct_wf)
{
    if ((pct_wf >= 0) && (pct_wf <= 100))
        ui->plotter->setPercent2DScreen(pct_wf);
}

/** Audio FFT rate has changed. */
void MainWindow::setAudioFftRate(int fps)
{
    auto interval = 1000 / fps;

    if (interval < 10)
        return;

    if (audio_fft_timer->isActive())
        audio_fft_timer->setInterval(interval);
}

/** Set FFT plot color. */
void MainWindow::setFftColor(const QColor& color)
{
    ui->plotter->setFftPlotColor(color);
    uiDockAudio->setFftColor(color);
}

/** Enable/disable filling the aread below the FFT plot. */
void MainWindow::enableFftFill(bool enable)
{
    ui->plotter->enableFftFill(enable);
    uiDockAudio->setFftFill(enable);
}

/**
 * @brief Start/Stop DSP processing.
 * @param checked Flag indicating whether DSP processing should be ON or OFF.
 *
 * This slot is executed when the actionDSP is toggled by the user. This can
 * either be via the menu bar or the "power on" button in the main toolbar or
 * by remote control.
 */
void MainWindow::on_actionDSP_triggered(bool checked)
{
    remote->setReceiverStatus(checked);

    if (checked)
    {
        /* start receiver */
        rx->start();

        /* start GUI timers */
        meter_timer->start(100);

        if (uiDockFft->fftRate())
        {
            iq_fft_timer->start(1000/uiDockFft->fftRate());
            ui->plotter->setRunningState(true);
        }
        else
        {
            iq_fft_timer->start(36e7); // 100 hours
            ui->plotter->setRunningState(false);
        }

        audio_fft_timer->start(40);

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Stop DSP processing"));
        ui->actionDSP->setText(tr("Stop DSP"));
    }
    else
    {
        /* stop GUI timers */
        meter_timer->stop();
        iq_fft_timer->stop();
        audio_fft_timer->stop();
        rds_timer->stop();

        /* stop receiver */
        rx->stop();

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Start DSP processing"));
        ui->actionDSP->setText(tr("Start DSP"));

        ui->plotter->setRunningState(false);
    }

    ui->actionDSP->setChecked(checked); //for remote control

}

/**
 * @brief Action: I/O device configurator triggered.
 *
 * This slot is activated when the user selects "I/O Devices" in the
 * menu. It activates the I/O configurator and if the user closes the
 * configurator using the OK button, the new configuration is read and
 * sent to the receiver.
 */
int MainWindow::on_actionIoConfig_triggered()
{
    qDebug() << "Configure I/O devices.";

    auto *ioconf = new CIoConfig(m_settings, devList);
    auto confres = ioconf->exec();

    if (confres == QDialog::Accepted)
    {
        bool dsp_running = ui->actionDSP->isChecked();

        if (dsp_running)
            // suspend DSP while we reload settings
            on_actionDSP_triggered(false);

        // Refresh LNB LO in dock widget, otherwise changes will be lost
        uiDockInputCtl->readLnbLoFromSettings(m_settings);
        storeSession();
        loadConfig(m_settings->fileName(), false, false);

        if (dsp_running)
            // restsart DSP
            on_actionDSP_triggered(true);
    }

    delete ioconf;

    return confres;
}


/** Run first time configurator. */
int MainWindow::firstTimeConfig()
{
    qDebug() << __func__;

    auto *ioconf = new CIoConfig(m_settings, devList);
    auto confres = ioconf->exec();

    if (confres == QDialog::Accepted)
        loadConfig(m_settings->fileName(), false, false);

    delete ioconf;

    return confres;
}


/** Load configuration activated by user. */
void MainWindow::on_actionLoadSettings_triggered()
{
    auto cfgfile = QFileDialog::getOpenFileName(this, tr("Load settings"),
                                           m_last_dir.isEmpty() ? m_cfg_dir : m_last_dir,
                                           tr("Settings (*.conf)"));

    qDebug() << "File to open:" << cfgfile;

    if (cfgfile.isEmpty())
        return;

    if (!cfgfile.endsWith(".conf", Qt::CaseSensitive))
        cfgfile.append(".conf");

    loadConfig(cfgfile, cfgfile != m_settings->fileName(), cfgfile != m_settings->fileName());

    // store last dir
    QFileInfo fi(cfgfile);
    if (m_cfg_dir != fi.absolutePath())
        m_last_dir = fi.absolutePath();
}

/** Save configuration activated by user. */
void MainWindow::on_actionSaveSettings_triggered()
{
    auto cfgfile = QFileDialog::getSaveFileName(this, tr("Save settings"),
                                           m_last_dir.isEmpty() ? m_cfg_dir : m_last_dir,
                                           tr("Settings (*.conf)"));

    qDebug() << "File to save:" << cfgfile;

    if (cfgfile.isEmpty())
        return;

    if (!cfgfile.endsWith(".conf", Qt::CaseSensitive))
        cfgfile.append(".conf");

    storeSession();
    saveConfig(cfgfile);

    // store last dir
    QFileInfo fi(cfgfile);
    if (m_cfg_dir != fi.absolutePath())
        m_last_dir = fi.absolutePath();
}

void MainWindow::on_actionSaveWaterfall_triggered()
{
    QDateTime   dt(QDateTime::currentDateTimeUtc());

    // previously used location
    auto save_path = m_settings->value("wf_save_dir", "").toString();
    if (!save_path.isEmpty())
        save_path += "/";
    save_path += dt.toString("gqrx_wf_yyyyMMdd_hhmmss.png");

    auto wffile = QFileDialog::getSaveFileName(this, tr("Save waterfall"),
                                          save_path, nullptr);
    if (wffile.isEmpty())
        return;

    if (!ui->plotter->saveWaterfall(wffile))
    {
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("There was an error saving the waterfall"));
    }

    // store the location used for the waterfall file
    QFileInfo fi(wffile);
    m_settings->setValue("wf_save_dir", fi.absolutePath());
}

/** Show I/Q player. */
void MainWindow::on_actionIqTool_triggered()
{
    iq_tool->show();
}


/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_newDemodFreq(qint64 freq, qint64 delta)
{
    // set RX filter
    rx->set_filter_offset((double) delta);

    // update RF freq label and channel filter offset
    uiDockRxOpt->setFilterOffset(delta);
    ui->freqCtrl->setFrequency(freq);

    if (rx->is_rds_decoder_active())
        rx->reset_rds_parser();
}

/* CPlotter::NewfilterFreq() is emitted or bookmark activated */
void MainWindow::on_plotter_newFilterFreq(int low, int high)
{   /* parameter correctness will be checked in receiver class */
    receiver::status retcode = rx->set_filter((double) low, (double) high, d_filter_shape);

    /* Update filter range of plotter, in case this slot is triggered by
     * switching to a bookmark */
    ui->plotter->setHiLowCutFrequencies(low, high);

    if (retcode == receiver::STATUS_OK)
        uiDockRxOpt->setFilterParam(low, high);
}

/** Full screen button or menu item toggled. */
void MainWindow::on_actionFullScreen_triggered(bool checked)
{
    if (checked)
    {
        ui->statusBar->hide();
        showFullScreen();
    }
    else
    {
        ui->statusBar->show();
        showNormal();
    }
}

/** Remote control button (or menu item) toggled. */
void MainWindow::on_actionRemoteControl_triggered(bool checked)
{
    if (checked)
        remote->start_server();
    else
        remote->stop_server();
}

/** Remote control configuration button (or menu item) clicked. */
void MainWindow::on_actionRemoteConfig_triggered()
{
    auto *rcs = new RemoteControlSettings();

    rcs->setPort(remote->getPort());
    rcs->setHosts(remote->getHosts());

    if (rcs->exec() == QDialog::Accepted)
    {
        remote->setPort(rcs->getPort());
        remote->setHosts(rcs->getHosts());
    }

    delete rcs;
}


#define DATA_BUFFER_SIZE 48000

/**
 * AFSK1200 decoder action triggered.
 *
 * This slot is called when the user activates the AFSK1200
 * action. It will create an AFSK1200 decoder window and start
 * and start pushing data from the receiver to it.
 */
void MainWindow::on_actionAFSK1200_triggered()
{
    if (!d_have_audio)
    {
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("AFSK1200 decoder requires a demodulator.\n"
                           "Currently, demodulation is switched off "
                           "(Mode->Demod Off)."));
        msg_box.exec();
        return;
    }
    if (dec_afsk1200 != nullptr)
    {
        qDebug() << "AFSK1200 decoder already active.";
        dec_afsk1200->raise();
    }
    else
    {
        qDebug() << "Starting AFSK1200 decoder.";

        /* start sample sniffer */
        if (rx->start_sniffer(22050, DATA_BUFFER_SIZE) == receiver::STATUS_OK)
        {
            dec_afsk1200 = new Afsk1200Win(this);
            connect(dec_afsk1200, SIGNAL(windowClosed()), this, SLOT(afsk1200win_closed()));
            dec_afsk1200->setAttribute(Qt::WA_DeleteOnClose);
            dec_afsk1200->show();

            dec_timer->start(100);
        }
        else
            QMessageBox::warning(this, tr("Gqrx error"),
                                 tr("Error starting sample sniffer.\n"
                                    "Close all data decoders and try again."),
                                 QMessageBox::Ok, QMessageBox::Ok);
    }
}


/**
 * Destroy AFSK1200 decoder window got closed.
 *
 * This slot is connected to the windowClosed() signal of the AFSK1200 decoder
 * object. We need this to properly destroy the object, stop timeout and clean
 * up whatever need to be cleaned up.
 */
void MainWindow::afsk1200win_closed()
{
    /* stop cyclic processing */
    dec_timer->stop();
    rx->stop_sniffer();

    dec_afsk1200 = nullptr;
}

/** Show DXC Options. */
void MainWindow::on_actionDX_Cluster_triggered()
{
    dxc_options->show();
}

/**
 * Cyclic processing for acquiring samples from receiver and processing them
 * with data decoders (see dec_* objects)
 */
void MainWindow::decoderTimeout()
{
    float buffer[DATA_BUFFER_SIZE];
    unsigned int num;

    rx->get_sniffer_data(&buffer[0], num);
    if (dec_afsk1200)
        dec_afsk1200->process_samples(&buffer[0], num);
}

void MainWindow::setRdsDecoder(bool checked)
{
    if (checked)
    {
        qDebug() << "Starting RDS decoder.";
        uiDockRDS->showEnabled();
        rx->start_rds_decoder();
        rx->reset_rds_parser();
        rds_timer->start(250);
    }
    else
    {
        qDebug() << "Stopping RDS decoder.";
        uiDockRDS->showDisabled();
        rx->stop_rds_decoder();
        rds_timer->stop();
    }
    remote->setRDSstatus(checked);
}

void MainWindow::onBookmarkActivated(qint64 freq, const QString& demod, int bandwidth)
{
    setNewFrequency(freq);
    selectDemod(demod);

    /* Check if filter is symmetric or not by checking the presets */
    auto mode = uiDockRxOpt->currentDemod();
    auto preset = uiDockRxOpt->currentFilter();

    int lo, hi;
    uiDockRxOpt->getFilterPreset(mode, preset, &lo, &hi);

    if(lo + hi == 0)
    {
        lo = -bandwidth / 2;
        hi =  bandwidth / 2;
    }
    else if(lo >= 0 && hi >= 0)
    {
        hi = lo + bandwidth;
    }
    else if(lo <= 0 && hi <= 0)
    {
        lo = hi - bandwidth;
    }

    on_plotter_newFilterFreq(lo, hi);
}

void MainWindow::setPassband(int bandwidth)
{
    /* Check if filter is symmetric or not by checking the presets */
    auto mode = uiDockRxOpt->currentDemod();
    auto preset = uiDockRxOpt->currentFilter();

    int lo, hi;
    uiDockRxOpt->getFilterPreset(mode, preset, &lo, &hi);

    if(lo + hi == 0)
    {
        lo = -bandwidth / 2;
        hi =  bandwidth / 2;
    }
    else if(lo >= 0 && hi >= 0)
    {
        hi = lo + bandwidth;
    }
    else if(lo <= 0 && hi <= 0)
    {
        lo = hi - bandwidth;
    }

    remote->setPassband(lo, hi);

    on_plotter_newFilterFreq(lo, hi);
}

/** Launch Gqrx google group website. */
void MainWindow::on_actionUserGroup_triggered()
{
    auto res = QDesktopServices::openUrl(QUrl("https://groups.google.com/g/gqrx",
                                              QUrl::TolerantMode));
    if (!res)
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to open website:\n"
                                "https://groups.google.com/g/gqrx"),
                             QMessageBox::Close);
}

/**
 * Show ftxt in a dialog window.
 */
void MainWindow::on_actionNews_triggered()
{
    showSimpleTextFile(":/textfiles/news.txt", tr("Release news"));
}

/**
 * Show remote-contol.txt in a dialog window.
 */
void MainWindow::on_actionRemoteProtocol_triggered()
{
    showSimpleTextFile(":/textfiles/remote-control.txt",
                       tr("Remote control protocol"));
}

/**
 * Show kbd-shortcuts.txt in a dialog window.
 */
void MainWindow::on_actionKbdShortcuts_triggered()
{
    showSimpleTextFile(":/textfiles/kbd-shortcuts.txt",
                       tr("Keyboard shortcuts"));
}

/**
 * Show simple text file in a window.
 */
void MainWindow::showSimpleTextFile(const QString &resource_path,
                                    const QString &window_title)
{
    QResource resource(resource_path);
    QFile news(resource.absoluteFilePath());

    if (!news.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Unable to open file: " << news.fileName() <<
                    " because of error " << news.errorString();

        return;
    }

    QTextStream in(&news);
    auto content = in.readAll();
    news.close();

    auto *browser = new QTextBrowser();
    browser->setLineWrapMode(QTextEdit::NoWrap);
    browser->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    browser->append(content);
    browser->adjustSize();

    // scroll to the beginning
    auto cursor = browser->textCursor();
    cursor.setPosition(0);
    browser->setTextCursor(cursor);


    auto *layout = new QVBoxLayout();
    layout->addWidget(browser);

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(window_title);
    dialog->setLayout(layout);
    dialog->resize(800, 400);
    dialog->exec();

    delete dialog;
    // browser and layout deleted automatically
}

/**
 * @brief Slot for handling loadConfig signals
 * @param cfgfile
 */
void MainWindow::loadConfigSlot(const QString &cfgfile)
{
    loadConfig(cfgfile, cfgfile != m_settings->fileName(), cfgfile != m_settings->fileName());
}

/**
 * @brief Action: About gqrx.
 *
 * This slot is called when the user activates the
 * Help|About menu item (or Gqrx|About on Mac)
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Gqrx"),
        tr("<p>This is Gqrx %1</p>"
           "<p>Copyright (C) 2011-2023 Alexandru Csete & contributors.</p>"
           "<p>Gqrx is a software defined radio (SDR) receiver powered by "
           "<a href='https://www.gnuradio.org/'>GNU Radio</a> and the Qt toolkit. "
           "<p>Gqrx uses the <a href='https://osmocom.org/projects/gr-osmosdr/wiki/GrOsmoSDR'>GrOsmoSDR</a> "
           "input source block and works with any input device supported by it, including "
           "Funcube Dongle, RTL-SDR, Airspy, HackRF, RFSpace, BladeRF and USRP receivers."
           "</p>"
           "<p>You can download the latest version from the "
           "<a href='https://gqrx.dk/'>Gqrx website</a>."
           "</p>"
           "<p>"
           "Gqrx is licensed under the <a href='https://www.gnu.org/licenses/gpl-3.0.html'>GNU General Public License</a>."
           "</p>").arg(VERSION));
}

/**
 * @brief Action: About Qt
 *
 * This slot is called when the user activates the
 * Help|About Qt menu item (or Gqrx|About Qt on Mac)
 */
void MainWindow::on_actionAboutQt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_actionAddBookmark_triggered()
{
    bool ok=false;
    QString name;
    QStringList tags;

    // Create and show the Dialog for a new Bookmark.
    // Write the result into variable 'name'.
    {
        QDialog dialog(this);
        dialog.setWindowTitle("New bookmark");

        auto* LabelAndTextfieldName = new QGroupBox(&dialog);
        auto* label1 = new QLabel("Bookmark name:", LabelAndTextfieldName);
        auto* textfield = new QLineEdit(LabelAndTextfieldName);
        auto *layout = new QHBoxLayout;
        layout->addWidget(label1);
        layout->addWidget(textfield);
        LabelAndTextfieldName->setLayout(layout);

        auto* buttonCreateTag = new QPushButton("Create new Tag", &dialog);

        auto* taglist = new BookmarksTagList(&dialog, false);
        taglist->updateTags();
        taglist->DeselectAll();

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                              | QDialogButtonBox::Cancel);
        connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
        connect(buttonCreateTag, SIGNAL(clicked()), taglist, SLOT(AddNewTag()));

        auto *mainLayout = new QVBoxLayout(&dialog);
        mainLayout->addWidget(LabelAndTextfieldName);
        mainLayout->addWidget(buttonCreateTag);
        mainLayout->addWidget(taglist);
        mainLayout->addWidget(buttonBox);

        ok = dialog.exec();
        if (ok)
        {
            name = textfield->text();
            tags = taglist->getSelectedTags();
            qDebug() << "Tags: " << tags;
        }
        else
        {
            name.clear();
            tags.clear();
        }
    }

    // Add new Bookmark to Bookmarks.
    if(ok)
    {
        int i;

        BookmarkInfo info;
        info.frequency = ui->freqCtrl->getFrequency();
        info.bandwidth = ui->plotter->getFilterBw();
        info.modulation = uiDockRxOpt->currentDemodAsString();
        info.name=name;
        info.tags.clear();
        if (tags.empty())
            info.tags.append(Bookmarks::Get().findOrAddTag(""));


        for (i = 0; i < tags.size(); ++i)
            info.tags.append(Bookmarks::Get().findOrAddTag(tags[i]));

        Bookmarks::Get().add(info);
        uiDockBookmarks->updateTags();
    }
}

void MainWindow::updateClusterSpots()
{
    ui->plotter->updateOverlay();
}

void MainWindow::frequencyFocusShortcut()
{
    ui->freqCtrl->setFrequencyFocus();
}

void MainWindow::rxOffsetZeroShortcut()
{
    uiDockRxOpt->setFilterOffset(0);
}

void MainWindow::toggleFreezeShortcut()
{
    ui->plotter->toggleFreeze();
}

void MainWindow::enableMarkers(bool enabled)
{
    d_show_markers = enabled;
    ui->markerFrame->setVisible(d_show_markers);
}

void MainWindow::toggleMarkers()
{
    enableMarkers(!d_show_markers);
    uiDockFft->setMarkersEnabled(d_show_markers);
}