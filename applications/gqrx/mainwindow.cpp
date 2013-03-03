/* -*- c++ -*- */
/*
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include <QSettings>
#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QDebug>
#include <QTimer>
#include "qtgui/ioconfig.h"
#include "mainwindow.h"

/* Qt Designer files */
#include "ui_mainwindow.h"

/* DSP */
#include "receiver.h"


MainWindow::MainWindow(const QString cfgfile, QWidget *parent) :
    QMainWindow(parent),
    configOk(true),
    ui(new Ui::MainWindow),
    d_lnb_lo(0),
    d_fftAvg(0.5),
    d_have_audio(true),
    dec_afsk1200(0)
{
    ui->setupUi(this);

    /* Initialise default configuration directory */
    QByteArray xdg_dir = qgetenv("XDG_CONFIG_HOME");
    if (xdg_dir.isEmpty())
        m_cfg_dir = QString("%1/.config/gqrx").arg(QDir::homePath()); // Qt takes care of conversion to native separators
    else
        m_cfg_dir = QString("%1/gqrx").arg(xdg_dir.data());

    setWindowTitle(QString("Gqrx %1").arg(VERSION));

    /* frequency control widget */
    ui->freqCtrl->setup(10, (quint64) 0, (quint64) 9999e6, 1, UNITS_MHZ);
    ui->freqCtrl->setFrequency(144500000);

    d_filter_shape = receiver::FILTER_SHAPE_NORMAL;

    /* create receiver object */
    rx = new receiver("", "");
    rx->set_rf_freq(144500000.0f);

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* FFT timer & data */
    iq_fft_timer = new QTimer(this);
    connect(iq_fft_timer, SIGNAL(timeout()), this, SLOT(iqFftTimeout()));

    audio_fft_timer = new QTimer(this);
    connect(audio_fft_timer, SIGNAL(timeout()), this, SLOT(audioFftTimeout()));

    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new double[MAX_FFT_SIZE];
    d_pwrFftData = new double[MAX_FFT_SIZE]();
    d_iirFftData = new double[MAX_FFT_SIZE];
    for (int i = 0; i < MAX_FFT_SIZE; i++)
        d_iirFftData[i] = -120.0;  // dBFS

    /* timer for data decoders */
    dec_timer = new QTimer(this);
    connect(dec_timer, SIGNAL(timeout()), this, SLOT(decoderTimeout()));

    /* create dock widgets */
    uiDockRxOpt = new DockRxOpt();
    uiDockAudio = new DockAudio();
    uiDockInputCtl = new DockInputCtl();
    //uiDockIqPlay = new DockIqPlayer();
    uiDockFft = new DockFft();

    /* Add dock widgets to main window. This should be done even for
       dock widgets that are going to be hidden, otherwise they will
       end up floating in their own top-level window and can not be
       docked to the mainwindow.
    */
    addDockWidget(Qt::RightDockWidgetArea, uiDockInputCtl);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRxOpt);
    tabifyDockWidget(uiDockInputCtl, uiDockRxOpt);

    addDockWidget(Qt::RightDockWidgetArea, uiDockAudio);
    addDockWidget(Qt::RightDockWidgetArea, uiDockFft);
    tabifyDockWidget(uiDockFft, uiDockAudio);

    //addDockWidget(Qt::BottomDockWidgetArea, uiDockIqPlay);

    /* hide docks that we don't want to show initially */
    /** FIXME: Hide them initially but store layout in config **/
    //    uiDockInputCtl->hide();
    //    uiDockFft->hide();
    //uiDockIqPlay->hide();

    /* misc configurations */
    //uiDockAudio->setFftRange(0, 8000); // FM

    /* Add dock widget actions to View menu. By doing it this way all signal/slot
       connections will be established automagially.
    */
    ui->menu_View->addAction(uiDockInputCtl->toggleViewAction());
    ui->menu_View->addAction(uiDockRxOpt->toggleViewAction());
    ui->menu_View->addAction(uiDockAudio->toggleViewAction());
    ui->menu_View->addAction(uiDockFft->toggleViewAction());
    //ui->menu_View->addAction(uiDockIqPlay->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->mainToolBar->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->actionFullScreen);

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(uiDockInputCtl, SIGNAL(lnbLoChanged(double)), this, SLOT(setLnbLo(double)));
    connect(uiDockInputCtl, SIGNAL(gainChanged(double)), SLOT(setRfGain(double)));
    connect(uiDockInputCtl, SIGNAL(freqCorrChanged(int)), this, SLOT(setFreqCorr(int)));
    connect(uiDockInputCtl, SIGNAL(iqSwapChanged(bool)), this, SLOT(setIqSwap(bool)));
    connect(uiDockInputCtl, SIGNAL(dcCancelChanged(bool)), this, SLOT(setDcCancel(bool)));
    connect(uiDockInputCtl, SIGNAL(ignoreLimitsChanged(bool)), this, SLOT(setIgnoreLimits(bool)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), this, SLOT(selectDemod(int)));
    connect(uiDockRxOpt, SIGNAL(fmMaxdevSelected(float)), this, SLOT(setFmMaxdev(float)));
    connect(uiDockRxOpt, SIGNAL(fmEmphSelected(double)), this, SLOT(setFmEmph(double)));
    connect(uiDockRxOpt, SIGNAL(amDcrToggled(bool)), this, SLOT(setAmDcr(bool)));
    connect(uiDockRxOpt, SIGNAL(agcToggled(bool)), this, SLOT(setAgcOn(bool)));
    connect(uiDockRxOpt, SIGNAL(agcHangToggled(bool)), this, SLOT(setAgcHang(bool)));
    connect(uiDockRxOpt, SIGNAL(agcThresholdChanged(int)), this, SLOT(setAgcThreshold(int)));
    connect(uiDockRxOpt, SIGNAL(agcSlopeChanged(int)), this, SLOT(setAgcSlope(int)));
    connect(uiDockRxOpt, SIGNAL(agcGainChanged(int)), this, SLOT(setAgcGain(int)));
    connect(uiDockRxOpt, SIGNAL(agcDecayChanged(int)), this, SLOT(setAgcDecay(int)));
    connect(uiDockRxOpt, SIGNAL(noiseBlankerChanged(int,bool,float)), this, SLOT(setNoiseBlanker(int,bool,float)));
    connect(uiDockRxOpt, SIGNAL(sqlLevelChanged(double)), this, SLOT(setSqlLevel(double)));
    connect(uiDockAudio, SIGNAL(audioGainChanged(float)), this, SLOT(setAudioGain(float)));
    connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), this, SLOT(startAudioRec(QString)));
    connect(uiDockAudio, SIGNAL(audioRecStopped()), this, SLOT(stopAudioRec()));
    connect(uiDockAudio, SIGNAL(audioPlayStarted(QString)), this, SLOT(startAudioPlayback(QString)));
    connect(uiDockAudio, SIGNAL(audioPlayStopped()), this, SLOT(stopAudioPlayback()));
    connect(uiDockAudio, SIGNAL(fftRateChanged(int)), this, SLOT(setAudioFftRate(int)));
    connect(uiDockFft, SIGNAL(fftSizeChanged(int)), this, SLOT(setIqFftSize(int)));
    connect(uiDockFft, SIGNAL(fftRateChanged(int)), this, SLOT(setIqFftRate(int)));
    connect(uiDockFft, SIGNAL(fftSplitChanged(int)), this, SLOT(setIqFftSplit(int)));
    connect(uiDockFft, SIGNAL(fftAvgChanged(double)), this, SLOT(setIqFftAvg(double)));
    connect(uiDockFft, SIGNAL(resetFftZoom()), ui->plotter, SLOT(resetHorizontalZoom()));
    connect(uiDockFft, SIGNAL(gotoFftCenter()), ui->plotter, SLOT(moveToCenterFreq()));
    connect(uiDockFft, SIGNAL(gotoDemodFreq()), ui->plotter, SLOT(moveToDemodFreq()));


    // restore last session
    if (!loadConfig(cfgfile, true))
    {
        qDebug() << "No input device found";
        if (on_actionIoConfig_triggered() != QDialog::Accepted)
        {
            qDebug() << "I/O device configuration cancelled.";
            configOk = false;
        }
    }

}

MainWindow::~MainWindow()
{
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
        m_settings->setValue("configversion", 2);
        m_settings->setValue("crashed", false);

        // save session
        storeSession();

        m_settings->sync();
        delete m_settings;
    }

    delete ui;
    delete uiDockRxOpt;
    delete uiDockAudio;
    delete uiDockFft;
    //delete uiDockIqPlay;
    delete uiDockInputCtl;
    delete rx;
    delete [] d_fftData;
    delete [] d_realFftData;
    delete [] d_iirFftData;
    delete [] d_pwrFftData;
}

/*! \brief Load new configuration.
 *  \param cfgfile
 *  \returns True if config is OK, False if not (e.g. no input device specified).
 *
 * If cfgfile is an absolute path it will be used as is, otherwise it is assumed to be the
 * name of a file under m_cfg_dir.
 *
 * If cfgfile does not exist it will be created.
 *
 * If no input device is specified, we return false to signal that the I/O configuration
 * dialog should be run.
 *
 * FIXME: Refactor.
 */
bool MainWindow::loadConfig(const QString cfgfile, bool check_crash)
{
    bool conf_ok = false;
    bool skipLoadingSettings = false;

    qDebug() << "Loading configuration from:" << cfgfile;

    if (m_settings)
        delete m_settings;

    if (QDir::isAbsolutePath(cfgfile))
        m_settings = new QSettings(cfgfile, QSettings::IniFormat);
    else
        m_settings = new QSettings(QString("%1/%2").arg(m_cfg_dir).arg(cfgfile), QSettings::IniFormat);

    qDebug() << "Configuration file:" << m_settings->fileName();

    if (check_crash)
    {
        if (m_settings->value("crashed", false).toBool())
        {
            qDebug() << "Crash guard triggered!" << endl;
            QMessageBox* askUserAboutConfig =
                    new QMessageBox(QMessageBox::Warning, tr("Crash Detected!"),
                                    tr("<p>Gqrx has detected problems with the current configuration. "
                                       "Loading the configuration again could cause the application to crash.</p>"
                                       "<p>Do you want to edit the settings?</p>"),
                                    QMessageBox::Yes | QMessageBox::No);
            askUserAboutConfig->setDefaultButton(QMessageBox::Yes);
            askUserAboutConfig->setTextFormat(Qt::RichText);
            askUserAboutConfig->exec();
            if (askUserAboutConfig->result() == QMessageBox::Yes)
                skipLoadingSettings = true;

            delete askUserAboutConfig;
        }
        else
        {
            m_settings->setValue("crashed", true); // clean exit will set this to FALSE
            saveConfig(cfgfile);
        }
    }

    if (skipLoadingSettings)
        return false;

    emit configChanged(m_settings);

    // manual reconf (FIXME: check status)
    bool conv_ok = false;

    QString indev = m_settings->value("input/device", "").toString();
    if (!indev.isEmpty())
    {
        conf_ok = true;
        rx->set_input_device(indev.toStdString());

        // Update window title
        QRegExp regexp("'([a-zA-Z0-9 \\-\\_\\/\\.\\,\\(\\)]+)'");
        QString devlabel;
        if (regexp.indexIn(indev, 0) != -1)
            devlabel = regexp.cap(1);
        else
            devlabel = indev; //"Unknown";

        setWindowTitle(QString("Gqrx %1 - %2").arg(VERSION).arg(devlabel));
    }

    QString outdev = m_settings->value("output/device", "").toString();
    rx->set_output_device(outdev.toStdString());

    int sr = m_settings->value("input/sample_rate", 0).toInt(&conv_ok);
    if (conv_ok && (sr > 0))
    {
        double actual_rate = rx->set_input_rate(sr);
        qDebug() << "Requested sample rate:" << sr;
        qDebug() << "Actual sample rate   :" << QString("%1").arg(actual_rate, 0, 'f', 6);
        uiDockRxOpt->setFilterOffsetRange((qint64)(0.9*actual_rate));
        ui->plotter->setSampleRate(actual_rate);
        ui->plotter->setSpanFreq((quint32)actual_rate);
    }

    ui->freqCtrl->setFrequency(m_settings->value("input/frequency", 144500000).toLongLong(&conv_ok));
    setNewFrequency(ui->freqCtrl->getFrequency()); // ensure all GUI and RF is updated

    uiDockInputCtl->readSettings(m_settings);
    uiDockRxOpt->readSettings(m_settings);
    uiDockFft->readSettings(m_settings);

    return conf_ok;
}

/*! \brief Save current configuration to a file.
 *  \param cfgfile
 *  \returns True if the operation was successful.
 *
 * If cfgfile is an absolute path it will be used as is, otherwise it is assumed to be the
 * name of a file under m_cfg_dir.
 *
 * If cfgfile already exists it will be overwritten (we assume that a file selection dialog
 * has already asked for confirmation of overwrite.
 *
 * Since QSettings does not support "save as" we do this by copying the current
 * settings to a new file.
 */
bool MainWindow::saveConfig(const QString cfgfile)
{
    QString oldfile = m_settings->fileName();
    QString newfile;

    qDebug() << "Saving configuration to:" << cfgfile;

    m_settings->sync();

    if (QDir::isAbsolutePath(cfgfile))
        newfile = cfgfile;
    else
        newfile = QString("%1/%2").arg(m_cfg_dir).arg(cfgfile);

    if (QFile::copy(oldfile, newfile))
    {
        loadConfig(cfgfile, false);
        return true;
    }
    else
    {
        qDebug() << "Error saving configuration to" << newfile;
        return false;
    }
}

/*! \brief Store session-related parameters (frequency, gain,...)
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
    }
}

/*! \brief Update RF frequency range.
 *  \param ignore_limits Whether ignore the hardware specd and allow DC-to-light range.
 *
 * Useful when we read a new configuration with a new input device. This function will
 * fetch the frequency range of the receiver and update the frequency control and frequency
 * bar widgets.
 *
 * This function must also be called when the LNB LO has changed.
 */
void MainWindow::updateFrequencyRange(bool ignore_limits)
{
    double startd, stopd, stepd;

    if (ignore_limits)
    {
        ui->freqCtrl->setup(10, (quint64) 0, (quint64) 9999e6, 1, UNITS_MHZ);
    }
    else if (rx->get_rf_range(&startd, &stopd, &stepd) == receiver::STATUS_OK)
    {
        qDebug() << QString("New frequnecy range: %1 - %2 MHz (step is %3 Hz but we use 1 Hz).").
                    arg(startd*1.0e-6).arg(stopd*1.0e-6).arg(stepd);

        qint64 start = (qint64)startd + d_lnb_lo;
        qint64 stop  = (qint64)stopd  + d_lnb_lo;

        ui->freqCtrl->setup(10, start, stop, 1, UNITS_MHZ);
    }
    else
    {
        qDebug() << __func__ << "failed fetching new frequency range";
    }
}

/*! \brief Slot for receiving frequency change signals.
 *  \param[in] freq The new frequency.
 *
 * This slot is connected to the CFreqCtrl::newFrequency() signal and is used
 * to set new RF frequency.
 */
void MainWindow::setNewFrequency(qint64 freq)
{
    /* set receiver frequency */
    rx->set_rf_freq((double) (freq-d_lnb_lo));

    /* update pandapter */
    ui->plotter->setCenterFreq(freq);

    /* update RX frequncy label in rxopts */
    uiDockRxOpt->setRfFreq(freq);
}

/*! \brief Set new LNB LO frequency.
 *  \param freq_mhz The new frequency in MHz.
 */
void MainWindow::setLnbLo(double freq_mhz)
{
    // calculate current RF frequency
    qint64 rf_freq = ui->freqCtrl->getFrequency() - d_lnb_lo;

    d_lnb_lo = qint64(freq_mhz*1e6);
    qDebug() << "New LNB LO:" << d_lnb_lo << "Hz";

    // Update ranges and show updated frequency in display
    updateFrequencyRange(uiDockInputCtl->ignoreLimits());
    ui->freqCtrl->setFrequency(d_lnb_lo + rf_freq);
}

/*! \brief Set new channel filter offset.
 *  \param freq_hs The new filter offset in Hz.
 */
void MainWindow::setFilterOffset(qint64 freq_hz)
{
    rx->set_filter_offset((double) freq_hz);
    ui->plotter->setFilterOffset(freq_hz);
}

/*! \brief Set RF gain.
 *  \param gain The new RF gain.
 *
 * Valid range depends on hardware.
 */
void MainWindow::setRfGain(double gain)
{
    rx->set_rf_gain(gain);
}

/*! \brief Set new frequency offset value.
 *  \param ppm Frequency correction.
 *
 * The valid range is between -200 and 200, though this is not checked.
 */
void MainWindow::setFreqCorr(int ppm)
{
    qDebug() << __FUNCTION__ << ":" << ppm << "ppm";
    rx->set_freq_corr(ppm);
}


/*! \brief Enable/disable I/Q reversion. */
void MainWindow::setIqSwap(bool reversed)
{
    rx->set_iq_swap(reversed);
}

/*! \brief Enable/disable automatic DC removal. */
void MainWindow::setDcCancel(bool enabled)
{
    rx->set_dc_cancel(enabled);
}

/*! \brief Ignore hardware limits.
 *  \param ignore_limits Whether harware limits should be ignored or not.
 *
 * This slot is triggered when the user changes the "Ignore hardware limits" option.
 * It will update the allowed frequency range and also update the current RF center
 * frequency, which may change when we swich from ignore to don't ignore.
 */
void MainWindow::setIgnoreLimits(bool ignore_limits)
{
    updateFrequencyRange(ignore_limits);

    qint64 freq = (qint64)rx->get_rf_freq();
    ui->freqCtrl->setFrequency(d_lnb_lo + freq);

    // This will ensure that if frequency is clamped, the UI
    // will be updated with the correct frequwncy.
    freq = ui->freqCtrl->getFrequency();
    setNewFrequency(freq);
}

/*! \brief Select new demodulator.
 *  \param demod New demodulator index.
 *
 * This slot basically maps the index of the mode selector to receiver::demod
 * and configures the default channel filter.
 *
 */
void MainWindow::selectDemod(int index)
{
    double quad_rate;
    float maxdev;
    int filter_preset = uiDockRxOpt->currentFilter();
    int flo=0, fhi=0, click_res=100;


    switch (index) {

    case DockRxOpt::MODE_OFF:
        /* Spectrum analyzer only */
        rx->set_demod(receiver::RX_DEMOD_OFF);
        flo = 0;
        fhi = 0;
        click_res = 100;

        break;

    case DockRxOpt::MODE_RAW:
        /* Raw I/Q */
        qDebug() << "RAW I/Q mode not implemented!";
        break;

        /* AM */
    case DockRxOpt::MODE_AM:
        rx->set_demod(receiver::RX_DEMOD_AM);
        ui->plotter->setDemodRanges(-20000, -250, 250, 20000, true);
        uiDockAudio->setFftRange(0,15000);
        click_res = 100;
        switch (filter_preset)
        {
        case 0: //wide
            flo = -10000;
            fhi = 10000;
            break;
        case 2: // narrow
            flo = -2500;
            fhi = 2500;
            break;
        default: // normal
            flo = -5000;
            fhi = 5000;
            break;
        }
        break;

        /* Narrow FM */
    case DockRxOpt::MODE_NFM:
        rx->set_demod(receiver::RX_DEMOD_NFM);
        click_res = 100;
        maxdev = uiDockRxOpt->currentMaxdev();
        if (maxdev < 20000.0)
        {   /** FIXME **/
            ui->plotter->setDemodRanges(-25000, -250, 250, 25000, true);
            uiDockAudio->setFftRange(0,12000);
            switch (filter_preset) {
            case 0: //wide
                flo = -10000;
                fhi = 10000;
                break;
            case 2: // narrow
                flo = -2500;
                fhi = 2500;
                break;
            default: // normal
                flo = -5000;
                fhi = 5000;
                break;
            }
        }
        else
        {
            ui->plotter->setDemodRanges(-45000, -10000, 10000, 45000, true);
            uiDockAudio->setFftRange(0,24000);
            switch (filter_preset) {
            /** FIXME: not sure about these **/
            case 0: //wide
                flo = -45000;
                fhi = 45000;
                break;
            case 2: // narrow
                flo = -10000;
                fhi = 10000;
                break;
            default: // normal
                flo = -35000;
                fhi = 35000;
                break;
            }
        }
        break;

        /* Broadcast FM */
    case DockRxOpt::MODE_WFM_MONO:
    case DockRxOpt::MODE_WFM_STEREO:
        quad_rate = rx->get_input_rate();
        if (quad_rate < 200.0e3)
            ui->plotter->setDemodRanges(-0.9*quad_rate/2.0, -10000,
                                        10000, 0.9*quad_rate/2.0,
                                        true);
        else
            ui->plotter->setDemodRanges(-250000, -10000, 10000, 250000, true);
        uiDockAudio->setFftRange(0,24000);  /** FIXME: get audio rate from rx **/
        click_res = 1000;
        switch (filter_preset)
        {
        case 0: //wide
            flo = -100000;
            fhi = 100000;
            break;
        case 2: // narrow
            flo = -60000;
            fhi = 60000;
            break;
        default: // normal
            flo = -80000;
            fhi = 80000;
            break;
        }
        if (index == DockRxOpt::MODE_WFM_MONO)
            rx->set_demod(receiver::RX_DEMOD_WFM_M);
        else
            rx->set_demod(receiver::RX_DEMOD_WFM_S);

        break;

        /* LSB */
    case DockRxOpt::MODE_LSB:
        rx->set_demod(receiver::RX_DEMOD_SSB);
        ui->plotter->setDemodRanges(-10000, -100, -5000, 0, false);
        uiDockAudio->setFftRange(0,3500);
        click_res = 10;
        switch (filter_preset)
        {
        case 0: //wide
            flo = -4100;
            fhi = -100;
            break;
        case 2: // narrow
            flo = -1600;
            fhi = -200;
            break;
        default: // normal
            flo = -3000;
            fhi = -200;
            break;
        }
        break;

        /* USB */
    case DockRxOpt::MODE_USB:
        rx->set_demod(receiver::RX_DEMOD_SSB);
        ui->plotter->setDemodRanges(0, 5000, 100, 10000, false);
        uiDockAudio->setFftRange(0,3500);
        click_res = 10;
        switch (filter_preset)
        {
        case 0: //wide
            flo = 100;
            fhi = 4100;
            break;
        case 2: // narrow
            flo = 200;
            fhi = 1600;
            break;
        default: // normal
            flo = 200;
            fhi = 3000;
            break;
        }
        break;

        /* CW-L */
    case DockRxOpt::MODE_CWL:
        rx->set_demod(receiver::RX_DEMOD_SSB);
        ui->plotter->setDemodRanges(-10000, -100, -5000, 0, false);
        uiDockAudio->setFftRange(0,1500);
        click_res = 10;
        switch (filter_preset)
        {
        case 0: //wide
            flo = -2300;
            fhi = -200;
            break;
        case 2: // narrow
            flo = -900;
            fhi = -400;
            break;
        default: // normal
            flo = -1200;
            fhi = -200;
            break;
        }
        break;

        /* CW-U */
    case DockRxOpt::MODE_CWU:
        rx->set_demod(receiver::RX_DEMOD_SSB);
        ui->plotter->setDemodRanges(0, 5000, 100, 10000, false);
        uiDockAudio->setFftRange(0,1500);
        click_res = 10;
        switch (filter_preset)
        {
        case 0: //wide
            flo = 200;
            fhi = 2300;
            break;
        case 2: // narrow
            flo = 400;
            fhi = 900;
            break;
        default: // normal
            flo = 200;
            fhi = 1200;
            break;
        }
        break;

    default:
        qDebug() << "Unsupported mode selection: " << index;
        flo = -5000;
        fhi = 5000;
        click_res = 100;
        break;
    }

    qDebug() << "Filter preset for mode" << index << "LO:" << flo << "HI:" << fhi;
    ui->plotter->setHiLowCutFrequencies(flo, fhi);
    ui->plotter->setClickResolution(click_res);
    ui->plotter->setFilterClickResolution(click_res);
    rx->set_filter((double)flo, (double)fhi, receiver::FILTER_SHAPE_NORMAL);

    d_have_audio = ((index != DockRxOpt::MODE_OFF) && (index != DockRxOpt::MODE_RAW));
}


/*! \brief New FM deviation selected.
 *  \param max_dev The enw FM deviation.
 */
void MainWindow::setFmMaxdev(float max_dev)
{
    qDebug() << "FM MAX_DEV: " << max_dev;

    /* receiver will check range */
    rx->set_fm_maxdev(max_dev);

    /* update filter */
    if (max_dev < 20000.0)
    {
        ui->plotter->setDemodRanges(-25000, -1000, 1000, 25000, true);
        ui->plotter->setHiLowCutFrequencies(-5000, 5000);
        rx->set_filter(-5000.0, 5000.0, receiver::FILTER_SHAPE_NORMAL);
    }
    else
    {
        ui->plotter->setDemodRanges(-45000, -10000, 10000, 45000, true);
        ui->plotter->setHiLowCutFrequencies(-35000, 35000);
        rx->set_filter(-35000.0, 35000.0, receiver::FILTER_SHAPE_NORMAL);
    }
}


/*! \brief New FM de-emphasis time consant selected.
 *  \param tau The new time constant
 */
void MainWindow::setFmEmph(double tau)
{
    qDebug() << "FM TAU: " << tau;

    /* receiver will check range */
    rx->set_fm_deemph(tau);
}


/*! \brief AM DCR status changed (slot).
 *  \param enabled Whether DCR is enabled or not.
 */
void MainWindow::setAmDcr(bool enabled)
{
    rx->set_am_dcr(enabled);
}

/*! \brief Audio gain changed.
 *  \param value The new audio gain in dB.
 */
void MainWindow::setAudioGain(float value)
{
    rx->set_af_gain(value);
}

/*! \brief Set AGC ON/OFF.
 *  \param agc_on Whether AGC is ON (true) or OFF (false).
 */
void MainWindow::setAgcOn(bool agc_on)
{
    rx->set_agc_on(agc_on);
}

/*! \brief AGC hang ON/OFF.
 *  \param use_hang Whether to use hang.
 */
void MainWindow::setAgcHang(bool use_hang)
{
    rx->set_agc_hang(use_hang);
}

/*! \brief AGC threshold changed.
 *  \param threshold The new threshold.
 */
void MainWindow::setAgcThreshold(int threshold)
{
    rx->set_agc_threshold(threshold);
}

/*! \brief AGC slope factor changed.
 *  \param factor The new slope factor.
 */
void MainWindow::setAgcSlope(int factor)
{
    rx->set_agc_slope(factor);
}

/*! \brief AGC manual gain changed.
 *  \param gain The new manual gain in dB.
 */
void MainWindow::setAgcGain(int gain)
{
    rx->set_agc_manual_gain(gain);
}

/*! \brief AGC decay changed.
 *  \param factor The new AGC decay.
 */
void MainWindow::setAgcDecay(int msec)
{
    rx->set_agc_decay(msec);
}

/*! \brief Noide blanker configuration changed.
 *  \param nb1 Noise blanker 1 ON/OFF.
 *  \param nb2 Noise blanker 2 ON/OFF.
 *  \param threshold Noise blanker threshold.
 */
void MainWindow::setNoiseBlanker(int nbid, bool on, float threshold)
{
    qDebug() << "Noise blanker NB:" << nbid << " ON:" << on << "THLD:" << threshold;

    rx->set_nb_on(nbid, on);
    rx->set_nb_threshold(nbid, threshold);
}


/*! \brief Squelch level changed.
 *  \param level_db The new squelch level in dBFS.
 */
void MainWindow::setSqlLevel(double level_db)
{
    rx->set_sql_level(level_db);
}


/*! \brief Signal strength meter timeout */
void MainWindow::meterTimeout()
{
    float level;

    level = rx->get_signal_pwr(true);
    ui->sMeter->setLevel(level);
}

/*! \brief Baseband FFT plot timeout. */
void MainWindow::iqFftTimeout()
{
    unsigned int fftsize;
    unsigned int i;
    double gain;
    double pwr;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */


    rx->get_iq_fft_data(d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        return;
    }

    scaleFactor = std::complex<float>((float)fftsize);

    /** FIXME: move post processing to rx_fft_c **/
    /* Normalize, calculcate power and shift the FFT */
    for (i = 0; i < fftsize; i++)
    {

        /* normalize and shift */
        if (i < fftsize/2)
        {
            pt = d_fftData[fftsize/2+i] / scaleFactor;
        }
        else
        {
            pt = d_fftData[i-fftsize/2] / scaleFactor;
        }
        pwr = pt.imag()*pt.imag() + pt.real()*pt.real();

        /* calculate power in dBFS */
        d_realFftData[i] = 10.0 * log10(pwr + 1.0e-20);

        /* FFT averaging (aka. video filter) */
        gain = d_fftAvg * (150.0+d_realFftData[i])/150.0;
        //gain = 0.1;

        d_iirFftData[i] = (1.0 - gain) * d_iirFftData[i] + gain * d_realFftData[i];

    }

    ui->plotter->setNewFttData(d_iirFftData, d_realFftData, fftsize);

}

/*! \brief Audio FFT plot timeout. */
void MainWindow::audioFftTimeout()
{
    unsigned int fftsize;
    unsigned int i;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */

    if (!d_have_audio)
        return;

    rx->get_audio_fft_data(d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        qDebug() << "No audio FFT data.";
        return;
    }

    scaleFactor = std::complex<float>((float)fftsize);

    /** FIXME: move post processing to rx_fft_f **/
    /* Normalize, calculcate power and shift the FFT */
    for (i = 0; i < fftsize; i++)
    {
        /* normalize and shift */
        if (i < fftsize/2)
        {
            pt = d_fftData[fftsize/2+i] / scaleFactor;
        }
        else
        {
            pt = d_fftData[i-fftsize/2] / scaleFactor;
        }

        /* calculate power in dBFS */
        d_realFftData[i] = 10.0 * log10(pt.imag()*pt.imag() + pt.real()*pt.real() + 1.0e-20);
    }

    uiDockAudio->setNewFttData(d_realFftData, fftsize);
}


/*! \brief Start audio recorder.
 *  \param filename The file name into which audio should be recorded.
 */
void MainWindow::startAudioRec(const QString filename)
{
    if (rx->start_audio_recording(filename.toStdString()))
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


/*! \brief Stop audio recorder. */
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


/*! \brief Start playback of audio file. */
void MainWindow::startAudioPlayback(const QString filename)
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

/*! \brief Stop playback of audio file. */
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


/*! \brief Start/stop I/Q data playback.
 *  \param play True if playback is started, false if it is stopped.
 *  \param filename Full path of the I/Q data file.
 */
void MainWindow::toggleIqPlayback(bool play, const QString filename)
{
    if (play)
    {
        /* starting playback */
        if (rx->start_iq_playback(filename.toStdString(), 96000.0))
        {
            ui->statusBar->showMessage(tr("Error trying to play %1").arg(filename));
        }
        else
        {
            ui->statusBar->showMessage(tr("Playing %1").arg(filename));

            /* disable REC button */
            ui->actionIqRec->setEnabled(false);
        }
    }
    else
    {
        /* stopping playback */
        if (rx->stop_iq_playback())
        {
            /* okay, this one would be weird if it really happened */
            ui->statusBar->showMessage(tr("Error stopping I/Q playback"));
        }
        else
        {
            ui->statusBar->showMessage(tr("I/Q playback stopped"), 5000);
        }

        /* enable REC button */
        ui->actionIqRec->setEnabled(true);
    }
}


/*! \brief FFT size has changed. */
void MainWindow::setIqFftSize(int size)
{
    qDebug() << "Changing baseband FFT size to" << size;
    rx->set_iq_fft_size(size);
}

/*! \brief Baseband FFT rate has changed. */
void MainWindow::setIqFftRate(int fps)
{
    int interval;

    if (fps == 0)
    {
        interval = 36e7; // 100 hours
        ui->plotter->setRunningState(false);
    }
    else
    {
        interval = 1000 / fps;

        if (iq_fft_timer->isActive())
            ui->plotter->setRunningState(true);
    }

    if (interval > 9 && iq_fft_timer->isActive())
        iq_fft_timer->setInterval(interval);
}

/*! \brief Vertical split between waterfall and pandapter changed.
 *  \param pct_pand The percentage of the waterfall.
 */
void MainWindow::setIqFftSplit(int pct_wf)
{
    if ((pct_wf >= 20) && (pct_wf <= 80))
    {
        ui->plotter->setPercent2DScreen(pct_wf);
    }
}

void MainWindow::setIqFftAvg(double avg)
{
    if ((avg >= 0) && (avg <= 1.0))
        d_fftAvg = avg;
}

/*! \brief Audio FFT rate has changed. */
void MainWindow::setAudioFftRate(int fps)
{
    int interval = 1000 / fps;

    if (interval < 10)
        return;

    if (audio_fft_timer->isActive())
        audio_fft_timer->setInterval(interval);
}

/*! \brief Force receiver reconfiguration.
 *
 * Aka. jerky dongle workaround.
 *
 * This function forces a receiver reconfiguration by sending a fake
 * selectDemod() signal using the current demodulator selection.
 *
 * This function provides a workaround for the "jerky streaming" that has
 * been experienced using some RTL-SDR dongles when DSP processing is
 * started. The jerkyness disappears when trhe receiver is reconfigured
 * by selecting a new demodulator.
 */
void MainWindow::forceRxReconf()
{
    qDebug() << "Force RX reconf (jerky dongle workarond)...";
    selectDemod(uiDockRxOpt->currentDemod());
}

/*! \brief Start/Stop DSP processing.
 *  \param checked Flag indicating whether DSP processing should be ON or OFF.
 *
 * This slot is executed when the actionDSP is toggled by the user. This can either be
 * via the menu bar or the "power on" button in the main toolbar.
 */
void MainWindow::on_actionDSP_triggered(bool checked)
{
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

        audio_fft_timer->start(100);

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Stop DSP processing"));
        ui->actionDSP->setText(tr("Stop DSP"));

        // reconfigure RX after 1s to counteract possible jerky streaming from rtl dongles
        QTimer::singleShot(1000, this, SLOT(forceRxReconf()));
    }
    else
    {
        /* stop GUI timers */
        meter_timer->stop();
        iq_fft_timer->stop();
        audio_fft_timer->stop();

        /* stop receiver */
        rx->stop();

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Start DSP processing"));
        ui->actionDSP->setText(tr("Start DSP"));

        ui->plotter->setRunningState(false);
    }
}

/*! \brief Action: I/O device configurator triggered.
 *
 * This slot is activated when the user selects "I/O Devices" in the
 * menu. It activates the I/O configurator and if the user closes the
 * configurator using the OK button, the new configuration is read and
 * sent to the receiver.
 */
int MainWindow::on_actionIoConfig_triggered()
{
    qDebug() << "Configure I/O devices.";

    CIoConfig *ioconf = new CIoConfig(m_settings);
    int confres = ioconf->exec();

    if (confres == QDialog::Accepted)
    {
        if (ui->actionDSP->isChecked())
            // suspend DSP while we reload settings
            on_actionDSP_triggered(false);

        storeSession();
        loadConfig(m_settings->fileName(), false);

        if (ui->actionDSP->isChecked())
            // restsart DSP
            on_actionDSP_triggered(true);
    }

    delete ioconf;

    return confres;
}

/*! \brief Load configuration activated by user. */
void MainWindow::on_actionLoadSettings_triggered()
{
    QString cfgfile = QFileDialog::getOpenFileName(this,
                                                   tr("Load settings"),
                                                   m_last_dir.isEmpty() ? m_cfg_dir : m_last_dir,
                                                   tr("Settings (*.conf)"));

    qDebug() << "File to open:" << cfgfile;

    if (cfgfile.isEmpty())
        return;

    if (!cfgfile.endsWith(".conf", Qt::CaseSensitive))
        cfgfile.append(".conf");

    loadConfig(cfgfile, cfgfile != m_settings->fileName());

    // store last dir
    QFileInfo fi(cfgfile);
    if (m_cfg_dir != fi.absolutePath())
        m_last_dir = fi.absolutePath();
}

/*! \brief Save configuration activated by user. */
void MainWindow::on_actionSaveSettings_triggered()
{
    QString cfgfile = QFileDialog::getSaveFileName(this,
                                                   tr("Save settings"),
                                                   m_last_dir.isEmpty() ? m_cfg_dir : m_last_dir,
                                                   tr("Settings (*.conf)"));

    qDebug() << "File to save:" << cfgfile;

    if (cfgfile.isEmpty())
        return;

    if (!cfgfile.endsWith(".conf", Qt::CaseSensitive))
        cfgfile.append(".conf");

    saveConfig(cfgfile);

    // store last dir
    QFileInfo fi(cfgfile);
    if (m_cfg_dir != fi.absolutePath())
        m_last_dir = fi.absolutePath();
}


/*! \brief Toggle I/Q recording. */
void MainWindow::on_actionIqRec_triggered(bool checked)
{
    Q_UNUSED(checked)

#if 0
    if (checked)
    {
        /* generate file name using date, time, rf freq and BW */
        int freq = (int)rx->get_rf_freq()/1000;
        // FIXME: option to use local time
        QString lastRec = QDateTime::currentDateTimeUtc().toString("gqrx-yyyyMMdd-hhmmss-%1-96.'bin'").arg(freq);

        /* start recorder */
        if (rx->start_iq_recording(lastRec.toStdString()))
        {
            /* reset action status */
            ui->actionIqRec->toggle();
            ui->statusBar->showMessage(tr("Error starting I/Q recoder"));
        }
        else
        {
            ui->statusBar->showMessage(tr("Recording I/Q data to: %1").arg(lastRec), 5000);

            /* disable I/Q player */
            uiDockIqPlay->setEnabled(false);
        }
    }
    else
    {
        /* stop current recording */
        if (rx->stop_iq_recording())
        {
            ui->statusBar->showMessage(tr("Error stopping I/Q recoder"));
        }
        else
        {
            ui->statusBar->showMessage(tr("I/Q data recoding stopped"), 5000);
        }

        /* enable I/Q player */
        uiDockIqPlay->setEnabled(true);
    }
#endif
}

/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_newDemodFreq(qint64 freq, qint64 delta)
{
    // set RX filter
    rx->set_filter_offset((double) delta);

    // update RF freq label and channel filter offset
    uiDockRxOpt->setFilterOffset(delta);
    uiDockRxOpt->setRfFreq(freq-delta);
}

/* CPlotter::NewfilterFreq() is emitted */
void MainWindow::on_plotter_newFilterFreq(int low, int high)
{
    receiver::status retcode;

    /* parameter correctness will be checked in receiver class */
    retcode = rx->set_filter((double) low, (double) high, d_filter_shape);

    if (retcode == receiver::STATUS_OK)
        uiDockRxOpt->setFilterParam(low, high);
}

void MainWindow::on_plotter_newCenterFreq(qint64 f)
{
    rx->set_rf_freq(f);
    ui->freqCtrl->setFrequency(f);
}

/*! \brief Full screen button or menu item toggled. */
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


#define DATA_BUFFER_SIZE 48000

/*! \brief AFSK1200 decoder action triggered.
 *
 * This slot is called when the user activates the AFSK1200
 * action. It will create an AFSK1200 decoder window and start
 * and start pushing data from the receiver to it.
 */
void MainWindow::on_actionAFSK1200_triggered()
{

    if (dec_afsk1200 != 0)
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
            dec_afsk1200->show();

            dec_timer->start(100);
        }
        else
        {
            QMessageBox::warning(this, tr("Gqrx error"),
                                 tr("Error starting sample sniffer.\n"
                                    "Close all data decoders and try again."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}


/*! \brief Destroy AFSK1200 decoder window got closed.
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

    /* delete decoder object */
    delete dec_afsk1200;
    dec_afsk1200 = 0;
}


/*! \brief Cyclic processing for acquiring samples from receiver and
 *         processing them with data decoders (see dec_* objects)
 */
void MainWindow::decoderTimeout()
{
    float buffer[DATA_BUFFER_SIZE];
    unsigned int num;

    //qDebug() << "Process decoder";

    rx->get_sniffer_data(&buffer[0], num);
    if (dec_afsk1200)
    {
        dec_afsk1200->process_samples(&buffer[0], num);
    }
    /* else stop timeout and sniffer? */
}


/*! \brief Launch Gqrx google group website. */
void MainWindow::on_actionUserGroup_triggered()
{
    bool res = QDesktopServices::openUrl(QUrl("https://groups.google.com/forum/#!forum/gqrx",
                                              QUrl::TolerantMode));
    if (!res)
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to open website:\n"
                                "https://groups.google.com/forum/#!forum/gqrx"),
                             QMessageBox::Close);
    }
}

/*! \brief Action: About Qthid
 *
 * This slot is called when the user activates the
 * Help|About menu item (or Gqrx|About on Mac)
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Gqrx"),
                       tr("<p>This is Gqrx %1</p>"
                          "<p>Copyright (C) 2011-2012 Alexandru Csete & contributors.</p>"
                          "<p>Gqrx is a software defined radio receiver powered by GNU Radio and the Qt toolkit. "
                          "<p>Gqrx uses the OsmoSDR GNU Radio package and and works with any input device "
                          "supported by OsmoSDR, including:"
                          "<ul>"
                          "<li>Funcube Dongle Pro</li>"
                          "<li>RTL2832U-based DVB-T tuners (rtlsdr and rtlsdr-tcp)</li>"
                          "<li>Ettus Research USRP devices</li>"
                          "<li>OsmoSDR devices</li>"
                          "</ul></p>"
                          "<p>You can download the latest version from the "
                          "<a href='http://gqrx.sf.net/'>Gqrx website</a>.<br/>"
                          "Help is available in the <a href='https://groups.google.com/forum/#!forum/gqrx'>Gqrx Google group</a>."
                          "</p>"
                          "<p>"
                          "Gqrx is licensed under the <a href='http://www.gnu.org/licenses/gpl.html'>GNU General Public License</a>."
                          "</p>"
                          "<hr />"
                          "<p>References:"
                          "<ul>"
                          "<li><a href='http://www.gnuradio.org/'>GNU Radio website</a></li>"
                          "<li><a href='http://sdr.osmocom.org/trac/wiki/GrOsmoSDR'>OsmoSDR GNU Radio Source</a></li>"
                          "<li><a href='http://funcubedongle.com/'>Funcube Dongle website</a></li>"
                          "<li><a href='http://www.ettus.com/'>Ettus Research (USRP)</a></li>"
                          "</li>"
                          "</p>").arg(VERSION));
}

/*! \brief Action: About Qt
 *
 * This slot is called when the user activates the
 * Help|About Qt menu item (or Gqrx|About Qt on Mac)
 */
void MainWindow::on_actionAboutQt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
