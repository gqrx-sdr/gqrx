/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#include <QDateTime>
#include <QDesktopServices>
#include <QDebug>
#include "qtgui/ioconfig.h"
#include "mainwindow.h"

/* Qt Designer files */
#include "ui_mainwindow.h"

/* DSP */
#include "receiver.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    d_lnb_lo(0),
    dec_bpsk1000(0),
    dec_afsk1200(0)
{
    ui->setupUi(this);

    setWindowTitle(QString("gqrx %1 (Funcube Dongle)").arg(VERSION));

    /* frequency control widget */
    ui->freqCtrl->Setup(10, (quint64) 0, (quint64) 9999e6, 1, UNITS_MHZ);
    ui->freqCtrl->SetFrequency(144500000);

    d_filter_shape = receiver::FILTER_SHAPE_NORMAL;

    /* create receiver object */
    QSettings settings;

    QString indev = CIoConfig::getFcdDeviceName();
    //QString outdev = settings.value("output").toString();

    rx = new receiver(indev.toStdString(), "");
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

    /* timer for data decoders */
    dec_timer = new QTimer(this);
    connect(dec_timer, SIGNAL(timeout()), this, SLOT(decoderTimeout()));

    /* create dock widgets */
    uiDockRxOpt = new DockRxOpt();
    uiDockAudio = new DockAudio();
    uiDockFcdCtl = new DockFcdCtl();
    //uiDockIqPlay = new DockIqPlayer();
    uiDockFft = new DockFft();

    /* Add dock widgets to main window. This should be done even for
       dock widgets that are going to be hidden, otherwise they will
       end up floating in their own top-level window and can not be
       docked to the mainwindow.
    */
    addDockWidget(Qt::RightDockWidgetArea, uiDockFcdCtl);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRxOpt);
    tabifyDockWidget(uiDockFcdCtl, uiDockRxOpt);

    addDockWidget(Qt::RightDockWidgetArea, uiDockAudio);
    addDockWidget(Qt::RightDockWidgetArea, uiDockFft);
    tabifyDockWidget(uiDockAudio, uiDockFft);

    //addDockWidget(Qt::BottomDockWidgetArea, uiDockIqPlay);

    /* hide docks that we don't want to show initially */
    uiDockFcdCtl->hide();
    uiDockFft->hide();
    //uiDockIqPlay->hide();

    /* misc configurations */
    //uiDockAudio->setFftRange(0, 8000); // FM

    /* Add dock widget actions to View menu. By doing it this way all signal/slot
       connections will be established automagially.
    */
    ui->menu_View->addAction(uiDockFcdCtl->toggleViewAction());
    ui->menu_View->addAction(uiDockRxOpt->toggleViewAction());
    ui->menu_View->addAction(uiDockAudio->toggleViewAction());
    ui->menu_View->addAction(uiDockFft->toggleViewAction());
    //ui->menu_View->addAction(uiDockIqPlay->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->mainToolBar->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->actionFullScreen);

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(uiDockFcdCtl, SIGNAL(lnbLoChanged(double)), this, SLOT(setLnbLo(double)));
    connect(uiDockFcdCtl, SIGNAL(lnaGainChanged(float)), SLOT(setRfGain(float)));
    connect(uiDockFcdCtl, SIGNAL(freqCorrChanged(int)), this, SLOT(setFreqCorr(int)));
    connect(uiDockFcdCtl, SIGNAL(iqCorrChanged(double,double)), this, SLOT(setIqCorr(double,double)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), this, SLOT(selectDemod(int)));
    connect(uiDockRxOpt, SIGNAL(fmMaxdevSelected(float)), this, SLOT(setFmMaxdev(float)));
    connect(uiDockRxOpt, SIGNAL(fmEmphSelected(double)), this, SLOT(setFmEmph(double)));
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

    delete ui;
    delete uiDockRxOpt;
    delete uiDockAudio;
    delete uiDockFft;
    //delete uiDockIqPlay;
    delete uiDockFcdCtl;
    delete rx;
    delete [] d_fftData;
    delete [] d_realFftData;
}


/*! \brief Slot for receiving frequency change signals.
 *  \param[in] freq The new frequency.
 *
 * This slot is connected to the CFreqCtrl::NewFrequency() signal and is used
 * to set new RF frequency.
 */
void MainWindow::setNewFrequency(qint64 freq)
{
    /* set receiver frequency */
    rx->set_rf_freq((double) (freq-d_lnb_lo));

    /* update pandapter */
    ui->plotter->SetCenterFreq(freq);

    /* update RX frequncy label in rxopts */
    uiDockRxOpt->setRfFreq(freq);
}

/*! \brief Set new LNB LO frequency.
 *  \param freq_mhz The new frequency in MHz.
 */
void MainWindow::setLnbLo(double freq_mhz)
{
    // calculate current RF frequency
    qint64 rf_freq = ui->freqCtrl->GetFrequency() - d_lnb_lo;

    d_lnb_lo = qint64(freq_mhz*1e6);
    qDebug() << "New LNB LO:" << d_lnb_lo << "Hz";

    // Show updated frequency in display
    ui->freqCtrl->SetFrequency(d_lnb_lo + rf_freq);
}

/*! \brief Set new channel filter offset.
 *  \param freq_hs The new filter offset in Hz.
 */
void MainWindow::setFilterOffset(qint64 freq_hz)
{
    rx->set_filter_offset((double) freq_hz);
    ui->plotter->SetFilterOffset(freq_hz);
}

/*! \brief Set RF gain.
 *  \param gain The new RF gain.
 *
 * Valid range depends on hardware.
 */
void MainWindow::setRfGain(float gain)
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
    qDebug() << "PPM:" << ppm;
    rx->set_freq_corr(ppm);
}

/*! \brief Set new DC offset values.
 *  \param dci I correction.
 *  \param dcq Q correction.
 *
 * The valid range is between -1.0 and 1.0, though hthis is not checked.
 */
void MainWindow::setDcCorr(double dci, double dcq)
{
    qDebug() << "DCI:" << dci << "  DCQ:" << dcq;
    rx->set_dc_corr(dci, dcq);
}


/*! \brief Set new IQ correction values.
 *  \param gain IQ gain correction.
 *  \param phase IQ phase correction.
 *
 * The valid range is between -1.0 and 1.0, though hthis is not checked.
 */
void MainWindow::setIqCorr(double gain, double phase)
{
    qDebug() << "Gain:" << gain << "  Phase:" << phase;
    rx->set_iq_corr(gain, phase);
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
    float maxdev;
    int filter_preset = uiDockRxOpt->currentFilter();
    int flo, fhi;


    switch (index) {

    case 0:
        /* Raw I/Q */
        qDebug() << "RAW I/Q mode not implemented!";
        break;

        /* AM */
    case 1:
        rx->set_demod(receiver::DEMOD_AM);
        ui->plotter->SetDemodRanges(-20000, -100, 100, 20000, true);
        uiDockAudio->setFftRange(0,15000);
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
        break;

        /* FM */
    case 2:
        rx->set_demod(receiver::DEMOD_FM);
        maxdev = uiDockRxOpt->currentMaxdev();
        if (maxdev < 20000.0) {
            ui->plotter->SetDemodRanges(-25000, -100, 100, 25000, true);
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
        else {
            ui->plotter->SetDemodRanges(-45000, -10000, 10000, 45000, true);
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

        /* LSB */
    case 3:
        rx->set_demod(receiver::DEMOD_SSB);
        ui->plotter->SetDemodRanges(-10000, -100, -5000, 0, false);
        uiDockAudio->setFftRange(0,3500);
        switch (filter_preset) {
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
    case 4:
        rx->set_demod(receiver::DEMOD_SSB);
        ui->plotter->SetDemodRanges(0, 5000, 100, 10000, false);
        uiDockAudio->setFftRange(0,3500);
        switch (filter_preset) {
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

        /* CWL */
    case 5:
        rx->set_demod(receiver::DEMOD_SSB);
        ui->plotter->SetDemodRanges(-10000, -100, -5000, 0, false);
        uiDockAudio->setFftRange(0,1500);
        switch (filter_preset) {
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

        /* CWU */
    case 6:
        rx->set_demod(receiver::DEMOD_SSB);
        ui->plotter->SetDemodRanges(0, 5000, 100, 10000, false);
        uiDockAudio->setFftRange(0,1500);
        switch (filter_preset) {
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
        qDebug() << "Invalid mode selection: " << index;
        flo = -5000;
        fhi = 5000;
        break;
    }

    qDebug() << "Filter preset for mode" << index << "LO:" << flo << "HI:" << fhi;
    ui->plotter->SetHiLowCutFrequencies(flo, fhi);
    rx->set_filter((double)flo, (double)fhi, receiver::FILTER_SHAPE_NORMAL);
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
    if (max_dev < 20000.0) {
        ui->plotter->SetDemodRanges(-25000, -1000, 1000, 25000, true);
        ui->plotter->SetHiLowCutFrequencies(-5000, 5000);
        rx->set_filter(-5000.0, 5000.0, receiver::FILTER_SHAPE_NORMAL);
    }
    else {
        ui->plotter->SetDemodRanges(-45000, -10000, 10000, 45000, true);
        ui->plotter->SetHiLowCutFrequencies(-35000, 35000);
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
void MainWindow::setAmDcrStatus(bool enabled)
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
    int fftsize;
    int i;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */
    double min=0.0,max=-120.0,avg=0.0;


    rx->get_iq_fft_data(d_fftData, fftsize);

    if (fftsize == 0) {
        /* nothing to do, wait until next activation. */
        return;
    }

    scaleFactor = std::complex<float>((float)fftsize);

    /** FIXME: move post processing to rx_fft_c **/
    /* Normalize, calculcate power and shift the FFT */
    for (i = 0; i < fftsize; i++) {

        /* normalize and shift */
        if (i < fftsize/2) {
            pt = d_fftData[fftsize/2+i] / scaleFactor;
        }
        else {
            pt = d_fftData[i-fftsize/2] / scaleFactor;
        }

        /* calculate power in dBFS */
        d_realFftData[i] = 10.0 * log10(pt.imag()*pt.imag() + pt.real()*pt.real() + 1.0e-20);

/*
        if (d_realFftData[i] < min)
            min = d_realFftData[i];

        if (d_realFftData[i] > max)
            max = d_realFftData[i];

        avg = (avg+d_realFftData[i]) / 2.0;
*/
    }

    ui->plotter->SetNewFttData(d_realFftData, fftsize);

    //qDebug() << "FFT size: " << fftsize;
    //qDebug() << "FFT[0]=" << d_realFftData[0] << "  FFT[MID]=" << d_realFftData[fftsize/2];
    //qDebug() << "MIN:" << min << "  AVG:" << avg << "  MAX:" << max;
}

/*! \brief Audio FFT plot timeout. */
void MainWindow::audioFftTimeout()
{
    int fftsize;
    int i;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */
    double min=0.0,max=-120.0,avg=0.0;


    rx->get_audio_fft_data(d_fftData, fftsize);

    if (fftsize == 0) {
        /* nothing to do, wait until next activation. */
        qDebug() << "No audio FFT data.";
        return;
    }

    scaleFactor = std::complex<float>((float)fftsize);

    /** FIXME: move post processing to rx_fft_f **/
    /* Normalize, calculcate power and shift the FFT */
    for (i = 0; i < fftsize; i++) {

        /* normalize and shift */
        if (i < fftsize/2) {
            pt = d_fftData[fftsize/2+i] / scaleFactor;
        }
        else {
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
    if (rx->start_audio_recording(filename.toStdString())) {
        ui->statusBar->showMessage(tr("Error starting audio recorder"));

        /* reset state of record button */
        uiDockAudio->setAudioRecButtonState(false);
    }
    else {
        ui->statusBar->showMessage(tr("Recording audio to %1").arg(filename));
    }
}


/*! \brief Stop audio recorder. */
void MainWindow::stopAudioRec()
{
    if (rx->stop_audio_recording()) {
        /* okay, this one would be weird if it really happened */
        ui->statusBar->showMessage(tr("Error stopping audio recorder"));

        uiDockAudio->setAudioRecButtonState(true);
    }
    else {
        ui->statusBar->showMessage(tr("Audio recorder stopped"), 5000);
    }
}


/*! \brief Start playback of audio file. */
void MainWindow::startAudioPlayback(const QString filename)
{
    if (rx->start_audio_playback(filename.toStdString())) {
        ui->statusBar->showMessage(tr("Error trying to play %1").arg(filename));

        /* reset state of record button */
        uiDockAudio->setAudioPlayButtonState(false);
    }
    else {
        ui->statusBar->showMessage(tr("Playing %1").arg(filename));
    }
}

/*! \brief Stop playback of audio file. */
void MainWindow::stopAudioPlayback()
{
    if (rx->stop_audio_playback()) {
        /* okay, this one would be weird if it really happened */
        ui->statusBar->showMessage(tr("Error stopping audio playback"));

        uiDockAudio->setAudioPlayButtonState(true);
    }
    else {
        ui->statusBar->showMessage(tr("Audio playback stopped"), 5000);
    }
}


/*! \brief Start/stop I/Q data playback.
 *  \param play True if playback is started, false if it is stopped.
 *  \param filename Full path of the I/Q data file.
 */
void MainWindow::toggleIqPlayback(bool play, const QString filename)
{
    if (play) {
        /* starting playback */
        if (rx->start_iq_playback(filename.toStdString(), 96000.0)) {
            ui->statusBar->showMessage(tr("Error trying to play %1").arg(filename));
        }
        else {
            ui->statusBar->showMessage(tr("Playing %1").arg(filename));

            /* disable REC button */
            ui->actionIqRec->setEnabled(false);
        }
    }
    else {
        /* stopping playback */
        if (rx->stop_iq_playback()) {
            /* okay, this one would be weird if it really happened */
            ui->statusBar->showMessage(tr("Error stopping I/Q playback"));
        }
        else {
            ui->statusBar->showMessage(tr("I/Q playback stopped"), 5000);
        }

        /* enable REC button */
        ui->actionIqRec->setEnabled(true);
    }
}


/*! \brief FFT size has changed. */
void MainWindow::setIqFftSize(int size)
{
    qDebug() << "Changing baseband FFT size TBD...";
}

/*! \brief Baseband FFT rate has changed. */
void MainWindow::setIqFftRate(int fps)
{
    int interval = 1000 / fps;

    if (interval < 10)
        return;

    if (iq_fft_timer->isActive())
        iq_fft_timer->setInterval(interval);
}

/*! \brief Vertical split between waterfall and pandapter changed.
 *  \param pct_pand The percentage of the waterfall.
 */
void MainWindow::setIqFftSplit(int pct_wf)
{
    if ((pct_wf >= 20) && (pct_wf <= 80)) {
        ui->plotter->SetPercent2DScreen(pct_wf);
    }
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


/*! \brief Start/Stop DSP processing.
 *  \param checked Flag indicating whether DSP processing should be ON or OFF.
 *
 * This slot is executed when the actionDSP is toggled by the user. This can either be
 * via the menu bar or the "power on" button in the main toolbar.
 */
void MainWindow::on_actionDSP_triggered(bool checked)
{
    if (checked) {
        /* start receiver */
        rx->start();

        /* start GUI timers */
        meter_timer->start(100);
        iq_fft_timer->start(1000/uiDockFft->fftRate());
        audio_fft_timer->start(1000/uiDockAudio->fftRate());

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Stop DSP processing"));
        ui->actionDSP->setText(tr("Stop DSP"));
    }
    else {
        /* stop GUI timers */
        meter_timer->stop();
        iq_fft_timer->stop();
        audio_fft_timer->stop();

        /* stop receiver */
        rx->stop();

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Start DSP processing"));
        ui->actionDSP->setText(tr("Start DSP"));
    }
}

/*! \brief Toggle I/Q recording. */
void MainWindow::on_actionIqRec_triggered(bool checked)
{
#if 0
    if (checked) {
        /* generate file name using date, time, rf freq and BW */
        int freq = (int)rx->get_rf_freq()/1000;
        // FIXME: option to use local time
        QString lastRec = QDateTime::currentDateTimeUtc().toString("gqrx-yyyyMMdd-hhmmss-%1-96.'bin'").arg(freq);

        /* start recorder */
        if (rx->start_iq_recording(lastRec.toStdString())) {
            /* reset action status */
            ui->actionIqRec->toggle();
            ui->statusBar->showMessage(tr("Error starting I/Q recoder"));
        }
        else {
            ui->statusBar->showMessage(tr("Recording I/Q data to: %1").arg(lastRec), 5000);

            /* disable I/Q player */
            uiDockIqPlay->setEnabled(false);
        }
    }
    else {
        /* stop current recording */
        if (rx->stop_iq_recording()) {
            ui->statusBar->showMessage(tr("Error stopping I/Q recoder"));
        }
        else {
            ui->statusBar->showMessage(tr("I/Q data recoding stopped"), 5000);
        }

        /* enable I/Q player */
        uiDockIqPlay->setEnabled(true);
    }
#endif
}

/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_NewDemodFreq(qint64 freq, qint64 delta)
{
    // set RX filter
    rx->set_filter_offset((double) delta);

    // update RF freq label and channel filter offset
    uiDockRxOpt->setFilterOffset(delta);
    uiDockRxOpt->setRfFreq(freq-delta);
}

/* CPlotter::NewfilterFreq() is emitted */
void MainWindow::on_plotter_NewFilterFreq(int low, int high)
{
    receiver::status retcode;

    /* parameter correctness will be checked in receiver class */
    retcode = rx->set_filter((double) low, (double) high, d_filter_shape);

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

/*! \brief Action: I/O device configurator triggered.
 *
 * This slot is activated when the user selects "I/O Devices" in the
 * menu. It activates the I/O configurator and if the user closes the
 * configurator using the OK button, the new configuration is read and
 * sent to the receiver.
 */
void MainWindow::on_actionIODevices_triggered()
{
    QSettings settings;
    QString cindev = settings.value("input").toString();
    QString coutdev = settings.value("output").toString();


    CIoConfig *ioconf = new CIoConfig();
    int confres = ioconf->exec();

    if (confres == QDialog::Accepted) {
        QString nindev = settings.value("input").toString();
        QString noutdev = settings.value("output").toString();

        // we need to ensure that we don't reconfigure RX
        // with the same device as the already used one because
        // that can crash the receiver when using ALSA :(
        if (cindev != nindev)
            rx->set_input_device(nindev.toStdString());

        if (coutdev != noutdev)
            rx->set_output_device(noutdev.toStdString());
    }

    delete ioconf;
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

    if (dec_afsk1200 != 0) {
        qDebug() << "AFSK1200 decoder already active.";
        dec_afsk1200->raise();
    }
    else {
        qDebug() << "Starting AFSK1200 decoder.";

        /* start sample sniffer */
        if (rx->start_sniffer(22050, DATA_BUFFER_SIZE) == receiver::STATUS_OK) {
            dec_afsk1200 = new Afsk1200Win(this);
            connect(dec_afsk1200, SIGNAL(windowClosed()), this, SLOT(afsk1200win_closed()));
            dec_afsk1200->show();

            dec_timer->start(100);
        }
        else {
            int ret = QMessageBox::warning(this, tr("Gqrx error"),
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


/*! \brief BPSK1000 decoder action triggered.
 *
 * This slot is called when the user activates the BPSK1000
 * action. It will create an BPSK1000 decoder window and start
 * and start pushing data from the receiver to it.
 */
void MainWindow::on_actionBPSK1000_triggered()
{

    if (dec_bpsk1000 != 0) {
        qDebug() << "BPSK1000 decoder already active.";
        dec_bpsk1000->raise();
    }
    else {
        qDebug() << "Starting BPSK1000 decoder.";

        /* start sample sniffer */
        if (rx->start_sniffer(48000, DATA_BUFFER_SIZE) == receiver::STATUS_OK) {
            dec_bpsk1000 = new Bpsk1000Win(this);
            connect(dec_bpsk1000, SIGNAL(windowClosed()), this, SLOT(bpsk1000win_closed()));
            dec_bpsk1000->show();

            dec_timer->start(100);
        }
        else {
            int ret = QMessageBox::warning(this, tr("Gqrx error"),
                                           tr("Error starting sample sniffer.\n"
                                              "Close all data decoders and try again."),
                                           QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}


/*! \brief Destroy BPSK1000 decoder window got closed.
 *
 * This slot is connected to the windowClosed() signal of the BPSK1000 decoder
 * object. We need this to properly destroy the object, stop timeout and clean
 * up whatever need to be cleaned up.
 */
void MainWindow::bpsk1000win_closed()
{
    /* stop cyclic processing */
    dec_timer->stop();
    rx->stop_sniffer();

    /* delete decoder object */
    delete dec_bpsk1000;
    dec_bpsk1000 = 0;
}


/*! \brief Cyclic processing for acquiring samples from receiver and
 *         processing them with data decoders (see dec_* objects)
 */
void MainWindow::decoderTimeout()
{
    float buffer[DATA_BUFFER_SIZE];
    int num;

    //qDebug() << "Process decoder";

    rx->get_sniffer_data(&buffer[0], num);
    if (dec_bpsk1000) {
        dec_bpsk1000->process_samples(&buffer[0], num);
    }
    else if (dec_afsk1200) {
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
                          "<p><b>This is a beta release.</b></p>"
                          "<p>Gqrx is a software defined radio receiver for the Funcube Dongle.</p>"
                          "<p>Gqrx is powered by GNU Radio and the Qt toolkit (see About Qt) and is "
                          "currently available for Linux. You can download the latest version from the "
                          "<a href='http://gqrx.sf.net/'>Gqrx website</a>.</p>"
                          "<p>"
                          "You can get help in the <a href='https://groups.google.com/forum/#!forum/gqrx'>Gqrx Google group</a>."
                          "</p>"
                          "<p>"
                          "<a href='http://www.gnuradio.org/'>GNU Radio website</a><br/>"
                          "<a href='http://funcubedongle.com/'>Funcube Dongle website</a><br/>"
                          "<a href='http://www.ettus.com/'>Ettus Research (USRP)</a><br/>"
                          "</p>"
                          "<p>"
                          "Gqrx license: <a href='http://www.gnu.org/licenses/gpl.html'>GNU GPL</a>."
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
