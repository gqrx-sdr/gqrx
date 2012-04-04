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
    dec_bpsk1000(0),
    dec_afsk1200(0)
{
    ui->setupUi(this);

    setWindowTitle(QString("gqrx %1").arg(VERSION));

    /* frequency control widget */
    ui->freqCtrl->Setup(10, (quint64) 50e6, (quint64) 2e9, 1, UNITS_MHZ);
    ui->freqCtrl->SetFrequency(144500000);

    d_filter_shape = receiver::FILTER_SHAPE_NORMAL;

    /* create receiver object */
    QSettings settings;

    QString indev = CIoConfig::getFcdDeviceName(); /** FIXME: Need some checks **/
    //QString outdev = settings.value("output").toString();

    rx = new receiver(indev.toStdString(), "");
    rx->set_rf_freq(144500000.0f);

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* FFT timer & data */
    fft_timer = new QTimer(this);
    connect(fft_timer, SIGNAL(timeout()), this, SLOT(fftTimeout()));
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

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(uiDockFcdCtl, SIGNAL(lnaGainChanged(float)), SLOT(setRfGain(float)));
    connect(uiDockFcdCtl, SIGNAL(freqCorrChanged(int)), this, SLOT(setFreqCorr(int)));
    connect(uiDockFcdCtl, SIGNAL(dcCorrChanged(double,double)), this, SLOT(setDcCorr(double,double)));
    connect(uiDockFcdCtl, SIGNAL(iqCorrChanged(double,double)), this, SLOT(setIqCorr(double,double)));
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), this, SLOT(selectDemod(int)));
    connect(uiDockRxOpt, SIGNAL(fmMaxdevSelected(float)), this, SLOT(setFmMaxdev(float)));
    connect(uiDockRxOpt, SIGNAL(fmEmphSelected(double)), this, SLOT(setFmEmph(double)));
    connect(uiDockRxOpt, SIGNAL(sidebandSelected(int)), this, SLOT(setSideBand(int)));
    connect(uiDockRxOpt, SIGNAL(agcToggled(bool)), this, SLOT(setAgcOn(bool)));
    connect(uiDockRxOpt, SIGNAL(agcHangToggled(bool)), this, SLOT(setAgcHang(bool)));
    connect(uiDockRxOpt, SIGNAL(agcThresholdChanged(int)), this, SLOT(setAgcThreshold(int)));
    connect(uiDockRxOpt, SIGNAL(agcSlopeChanged(int)), this, SLOT(setAgcSlope(int)));
    connect(uiDockRxOpt, SIGNAL(agcGainChanged(int)), this, SLOT(setAgcGain(int)));
    connect(uiDockRxOpt, SIGNAL(agcDecayChanged(int)), this, SLOT(setAgcDecay(int)));
    connect(uiDockRxOpt, SIGNAL(sqlLevelChanged(double)), this, SLOT(setSqlLevel(double)));
    //connect(uiDockIqPlay, SIGNAL(playbackToggled(bool,QString)), this, SLOT(toggleIqPlayback(bool,QString)));
    connect(uiDockAudio, SIGNAL(audioGainChanged(float)), this, SLOT(setAudioGain(float)));
    connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), this, SLOT(startAudioRec(QString)));
    connect(uiDockAudio, SIGNAL(audioRecStopped()), this, SLOT(stopAudioRec()));
    connect(uiDockAudio, SIGNAL(audioPlayStarted(QString)), this, SLOT(startAudioPlayback(QString)));
    connect(uiDockAudio, SIGNAL(audioPlayStopped()), this, SLOT(stopAudioPlayback()));
    connect(uiDockFft, SIGNAL(fftSizeChanged(int)), this, SLOT(setFftSize(int)));
    connect(uiDockFft, SIGNAL(fftRateChanged(int)), this, SLOT(setFftRate(int)));
    connect(uiDockFft, SIGNAL(fftYminChanged(int)), this, SLOT(setFftYmin(int)));
    connect(uiDockFft, SIGNAL(fftYmaxChanged(int)), this, SLOT(setFftYmax(int)));
    connect(uiDockFft, SIGNAL(fftSplitChanged(int)), this, SLOT(setFftSplit(int)));
}

MainWindow::~MainWindow()
{
    /* stop and delete timers */
    dec_timer->stop();
    delete dec_timer;

    meter_timer->stop();
    delete meter_timer;

    fft_timer->stop();
    delete fft_timer;

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
    rx->set_rf_freq((double) freq);

    /* update pandapter */
    ui->plotter->SetCenterFreq(freq);

    /* update RX frequncy label in rxopts */
    uiDockRxOpt->setRfFreq(freq);
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
        fft_timer->start(1000/uiDockFft->fftRate());

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Stop DSP processing"));
        ui->actionDSP->setText(tr("Stop DSP"));
    }
    else {
        /* stop GUI timers */
        meter_timer->stop();
        fft_timer->stop();

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
 *  \param demod New demodulator index, see receiver::demod.
 */
void MainWindow::selectDemod(int index)
{
    float maxdev;
    receiver::demod mode = (receiver::demod)index;

    rx->set_demod(mode);

    switch (mode) {

    case receiver::DEMOD_SSB:
        if (uiDockRxOpt->currentSideBand()) {
            /* USB */
            ui->plotter->SetDemodRanges(0, 500, 600, 10000, false);
            ui->plotter->SetHiLowCutFrequencies(200, 3000);
            rx->set_filter(200.0, 3000.0, receiver::FILTER_SHAPE_NORMAL);
        }
        else {
            /* LSB */
            ui->plotter->SetDemodRanges(-10000, -600, -500, 0, false);
            ui->plotter->SetHiLowCutFrequencies(-3000, -200);
            rx->set_filter(-3000.0, -200.0, receiver::FILTER_SHAPE_NORMAL);
        }
        break;

    case receiver::DEMOD_AM:
        ui->plotter->SetDemodRanges(-15000, -1000, 1000, 15000, true);
        ui->plotter->SetHiLowCutFrequencies(-5000, 5000);
        rx->set_filter(-5000.0, 5000.0, receiver::FILTER_SHAPE_NORMAL);
        break;

    case receiver::DEMOD_FM:
        /* filter params depend on max deviation */
        maxdev = uiDockRxOpt->currentMaxdev();
        if (maxdev < 20000.0) {
            ui->plotter->SetDemodRanges(-25000, -1000, 1000, 25000, true);
            ui->plotter->SetHiLowCutFrequencies(-5000, 5000);
            rx->set_filter(-5000.0, 5000.0, receiver::FILTER_SHAPE_NORMAL);
        }
        else {
            ui->plotter->SetDemodRanges(-45000, -10000, 10000, 45000, true);
            ui->plotter->SetHiLowCutFrequencies(-35000, 35000);
            rx->set_filter(-35000.0, 35000.0, receiver::FILTER_SHAPE_NORMAL);
        }
        break;

    default:
        qDebug() << "Invalid mode selection: " << mode;
        break;

    }

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

/*! \brief Set new SSB sideband.
 *  \param sideband The new sideband(= = LSB, 1 = USB)
 */
void MainWindow::setSideBand(int sideband)
{
    receiver::demod mode = (receiver::demod)uiDockRxOpt->currentDemod();

    if (mode != receiver::DEMOD_SSB) {
        // This should not be possible because sideband selector is only
        // visible when demodulator type is SSB
        qDebug() << "Sideband selected but current mode is not SSB";
        return;
    }

    if (sideband) {
        /* USB */
        ui->plotter->SetDemodRanges(0, 500, 600, 10000, false);
        ui->plotter->SetHiLowCutFrequencies(200, 3000);
        rx->set_filter(200.0, 3000.0, receiver::FILTER_SHAPE_NORMAL);
    }
    else {
        /* LSB */
        ui->plotter->SetDemodRanges(-10000, -600, -500, 0, false);
        ui->plotter->SetHiLowCutFrequencies(-3000, -200);
        rx->set_filter(-3000.0, -200.0, receiver::FILTER_SHAPE_NORMAL);
    }

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
    ui->rxSigLabel->setText(QString("%1 dBFS").arg(level, 7, 'f', 2, ' '));
    ui->sMeter->setLevel(level);
}

/*! \brief FFT plot timeout. */
void MainWindow::fftTimeout()
{
    int fftsize;
    int i;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */
    double min=0.0,max=-120.0,avg=0.0;


    rx->get_fft_data(d_fftData, fftsize);

    if (fftsize == 0) {
        /* nothing to do, wait until next activation. */
        return;
    }

    scaleFactor = std::complex<float>((float)fftsize);


    /* Normalize, calculcate power and shift the FFT */
    for (i = 0; i < fftsize; i++) {

        /*if (i < fftsize/2) {
            d_realFftData[i] = 10.0*log10((d_fftData[fftsize/2+i].imag() * d_fftData[fftsize/2+i].imag() +
                                           d_fftData[fftsize/2+i].real() * d_fftData[fftsize/2+i].real()) + 1.0e-20);
        }
        else {
            d_realFftData[i] = 10.0*log10((d_fftData[i-fftsize/2].imag() * d_fftData[i-fftsize/2].imag() +
                                           d_fftData[i-fftsize/2].real() * d_fftData[i-fftsize/2].real()) + 1.0e-20);

        }*/

        /* normalize and shift */
        if (i < fftsize/2) {
            pt = d_fftData[fftsize/2+i] / scaleFactor;
        }
        else {
            pt = d_fftData[i-fftsize/2] / scaleFactor;
        }

        /* calculate power in dBFS */
        d_realFftData[i] = 10.0 * log10(pt.imag()*pt.imag() + pt.real()*pt.real() + 1.0e-20);

        if (d_realFftData[i] < min)
            min = d_realFftData[i];

        if (d_realFftData[i] > max)
            max = d_realFftData[i];

        avg = (avg+d_realFftData[i]) / 2.0;

        //d_realFftData[i] = -110.0;

    }

    ui->plotter->SetNewFttData(d_realFftData, fftsize);

    //qDebug() << "FFT size: " << fftsize;
    //qDebug() << "FFT[0]=" << d_realFftData[0] << "  FFT[MID]=" << d_realFftData[fftsize/2];
    //qDebug() << "MIN:" << min << "  AVG:" << avg << "  MAX:" << max;


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
void MainWindow::setFftSize(int size)
{
    qDebug() << "Changing FFT size TBD...";
}

/*! \brief FFT rate has changed. */
void MainWindow::setFftRate(int fps)
{
    int interval = 1000 / fps;

    if (interval < 10)
        return;

    if (fft_timer->isActive())
        fft_timer->setInterval(interval);
}



/*! \brief Lower limit of FFT plot Y-axis changed. */
void MainWindow::setFftYmin(int value)
{
    qDebug() << "Changing FFT Y min TBD...";
}


/*! \brief Upper limit of FFT plot Y-axis changed. */
void MainWindow::setFftYmax(int value)
{
    qDebug() << "Changing FFT Y max TBD...";
}


/*! \brief Vertical split between waterfall and pandapter changed.
 *  \param pct_pand The percentage of the waterfall.
 */
void MainWindow::setFftSplit(int pct_wf)
{
    if ((pct_wf >= 20) && (pct_wf <= 80)) {
        ui->plotter->SetPercent2DScreen(pct_wf);
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



/*! \brief Action: About Qthid
 *
 * This slot is called when the user activates the
 * Help|About menu item (or Gqrx|About on Mac)
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Gqrx"),
                       tr("<p>This is Gqrx %1</p>"
                          "<p><b>This is an alpha release!</b></p>"
                          "<p>Gqrx is a software defined radio receiver for Funcube Dongle and "
                          "Universal Software Radio Peripheral devices via the UHD driver.</p>"
                          "<p>Gqrx is powered by GNU Radio and the Qt toolkit (see About Qt) and will be available "
                          "for Linux, Mac and Windows. You can download the latest version from the "
                          "<a href='http://gqrx.sf.net/'>gqrx website</a>.</p>"
                          "<p>"
                          "<a href='http://www.gnuradio.org/'>GNU Radio website</a><br/>"
                          "<a href='http://www.ettus.com/'>Ettus Research (USRP)</a><br/>"
                          "<a href='http://funcubedongle.com/'>Funcube Dongle website</a><br/>"
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
