/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2014 Alexandru Csete OZ9AEC.
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

#include "applications/gqrx/demodulator_controller.h"

DemodulatorController::DemodulatorController(
    receiver::sptr rx,
    size_t idx,
    QMainWindow *parent
) :
    idx(idx),
    rx(rx),
    d_have_audio(true)
{
    d_filter_shape = rx_filter_shape::FILTER_SHAPE_NORMAL;

    uiDockRxOpt = new DockRxOpt();
    uiDockRxOpt->set_idx(idx);
    uiDockAudio = new DockAudio();
    // uiDockAudio->set_idx(idx);
    parent->addDockWidget(Qt::LeftDockWidgetArea, uiDockRxOpt);
    parent->addDockWidget(Qt::LeftDockWidgetArea, uiDockAudio);
    parent->tabifyDockWidget(uiDockRxOpt, uiDockAudio);

    uiDockRDS = new DockRDS();
    parent->addDockWidget(Qt::RightDockWidgetArea, uiDockRDS);
    uiDockRDS->hide();

    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new float[MAX_FFT_SIZE];

    // Rx Frequency Control
    // connect(uiDockRxOpt, SIGNAL(rxFreqChanged(qint64)), ui->freqCtrl, SLOT(setFrequency(qint64)));
    // connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), uiDockAudio, SLOT(setRxFrequency(qint64)));
    // connect(ui->freqCtrl, SIGNAL(newFrequency(qint64)), uiDockRxOpt, SLOT(setRxFreq(qint64)));

    // Rx control
    connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), this, SLOT(setFilterOffset(qint64)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), this, SLOT(selectDemod(int)));
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

    // Rx remote control
    // connect(uiDockRxOpt, SIGNAL(sqlLevelChanged(double)), remote, SLOT(setSquelchLevel(double)));
    // connect(uiDockRxOpt, SIGNAL(filterOffsetChanged(qint64)), remote, SLOT(setFilterOffset(qint64)));
    // connect(uiDockRxOpt, SIGNAL(demodSelected(int)), remote, SLOT(setMode(int)));
    // connect(remote, SIGNAL(newFilterOffset(qint64)), uiDockRxOpt, SLOT(setFilterOffset(qint64)));
    // connect(remote, SIGNAL(newMode(int)), uiDockRxOpt, SLOT(setCurrentDemod(int)));
    // connect(remote, SIGNAL(newSquelchLevel(double)), uiDockRxOpt, SLOT(setSquelchLevel(double)));

    // Audio options
    connect(uiDockAudio, SIGNAL(audioGainChanged(float)), this, SLOT(setAudioGain(float)));
    connect(uiDockAudio, SIGNAL(audioStreamingStarted(QString,int,bool)), this, SLOT(startAudioStream(QString,int,bool)));
    connect(uiDockAudio, SIGNAL(audioStreamingStopped()), this, SLOT(stopAudioStreaming()));
    connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), this, SLOT(startAudioRec(QString)));
    connect(uiDockAudio, SIGNAL(audioRecStopped()), this, SLOT(stopAudioRec()));
    connect(uiDockAudio, SIGNAL(audioPlayStarted(QString)), this, SLOT(startAudioPlayback(QString)));
    connect(uiDockAudio, SIGNAL(audioPlayStopped()), this, SLOT(stopAudioPlayback()));
    connect(uiDockAudio, SIGNAL(fftRateChanged(int)), this, SLOT(setAudioFftRate(int)));
    // Audio display
    // connect(uiDockFft, SIGNAL(wfColormapChanged(QString)), uiDockAudio, SLOT(setWfColormap(QString)));

    audio_fft_timer = new QTimer(this);
    connect(audio_fft_timer, SIGNAL(timeout()), this, SLOT(audioFftTimeout()));

    // Audio remote control
    // connect(uiDockAudio, SIGNAL(audioRecStopped()), remote, SLOT(stopAudioRecorder()));
    // connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), remote, SLOT(startAudioRecorder(QString)));
    // connect(remote, SIGNAL(startAudioRecorderEvent()), uiDockAudio, SLOT(startAudioRecorder()));
    // connect(remote, SIGNAL(stopAudioRecorderEvent()), uiDockAudio, SLOT(stopAudioRecorder()));

    // RDS
    connect(uiDockRDS, SIGNAL(rdsDecoderToggled(bool)), this, SLOT(setRdsDecoder(bool)));

    rds_timer = new QTimer(this);
    connect(rds_timer, SIGNAL(timeout()), this, SLOT(rdsTimeout()));
}

DemodulatorController::~DemodulatorController()
{
    audio_fft_timer->stop();
    delete audio_fft_timer;

    rds_timer->stop();
    delete audio_fft_timer;

    delete [] d_fftData;
    delete [] d_realFftData;

    delete uiDockRxOpt;
    delete uiDockAudio;
    delete uiDockRDS;
}

void DemodulatorController::readSettings(QSettings *settings)
{
    uiDockRxOpt->readSettings(settings, idx);
    uiDockAudio->readSettings(settings, idx);
}

void DemodulatorController::saveSettings(QSettings *settings)
{
    uiDockRxOpt->saveSettings(settings, idx);
    uiDockAudio->saveSettings(settings, idx);
}

void DemodulatorController::ensureOffsetInRange(qint64 freq, qint64 lnb_lo, qint64 hw_freq_start, qint64 hw_freq_stop)
{
    // If frequency is out of range set frequency to the center of the range.
    qint64 hw_freq = freq - lnb_lo - (qint64)(rx->get_filter_offset(idx));
    if (hw_freq < hw_freq_start || hw_freq > hw_freq_stop)
    {
        qint64 next = (hw_freq_stop - hw_freq_start) / 2 + lnb_lo;
        rx->set_filter_offset(idx, next);
    }
}

/* Frequency Control */

void DemodulatorController::setFilterOffsetRange(qint64 range)
{
    uiDockRxOpt->setFilterOffsetRange(range);
}

void DemodulatorController::setFrequencyRange(qint64 hw_start, qint64 hw_stop)
{
    auto start = (qint64)(rx->get_filter_offset(idx)) + hw_start;
    auto stop  = (qint64)(rx->get_filter_offset(idx)) + hw_stop;
    uiDockRxOpt->setRxFreqRange(start, stop);
}

void DemodulatorController::setHwFrequency(qint64 rx_freq, qint64 d_lnb_lo)
{
    auto hw_freq = (double)(rx_freq - d_lnb_lo) - rx->get_filter_offset(idx);
    uiDockRxOpt->setHwFreq(hw_freq);
}

void DemodulatorController::setFilterFrequency(int low, int high)
{
    /* parameter correctness will be checked in receiver class */
    rx_status retcode = rx->set_filter(idx, (double) low, (double) high, d_filter_shape);

    if (retcode == rx_status::STATUS_OK) {
        uiDockRxOpt->setFilterParam(low, high);
    }
}

/* UI Behaviour control */

void DemodulatorController::setFreqCtrlReset(bool enabled)
{
    uiDockRxOpt->setResetLowerDigits(enabled);
}

void DemodulatorController::setInvertScrolling(bool enabled)
{
    uiDockRxOpt->setInvertScrolling(enabled);
    uiDockAudio->setInvertScrolling(enabled);
}

void DemodulatorController::setFftColor(const QColor& color)
{
    uiDockAudio->setFftColor(color);
}

void DemodulatorController::setFftFill(bool enable)
{
    uiDockAudio->setFftFill(enable);
}

/* Demodulator controls */

/**
 * @brief Set new channel filter offset.
 * @param freq_hz The new filter offset in Hz.
 */
void DemodulatorController::setFilterOffset(qint64 freq_hz)
{
    rx->set_filter_offset(idx, (double) freq_hz);
    // ui->plotter->setFilterOffset(freq_hz);

    // update RF freq label and channel filter offset
    uiDockRxOpt->setFilterOffset(freq_hz);

    emit updateFrequencyRange();

    // if (rx->is_rds_decoder_active()) {
    //     rx->reset_rds_parser();
    // }
}

/**
 * @brief Select new demodulator.
 * @param demod New demodulator.
 */
void DemodulatorController::selectDemod(const QString& demod)
{
    int iDemodIndex = DockRxOpt::GetEnumForModulationString(demod);
    qDebug() << "selectDemod(str):" << demod << "-> IDX:" << iDemodIndex;

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
void DemodulatorController::selectDemod(int mode_idx)
{
    double  cwofs = 0.0;
    int     filter_preset = uiDockRxOpt->currentFilter();
    int     flo=0, fhi=0, click_res=100;

    // validate mode_idx
    if (mode_idx < DockRxOpt::MODE_OFF || mode_idx >= DockRxOpt::MODE_LAST)
    {
        qDebug() << "Invalid mode index:" << mode_idx;
        mode_idx = DockRxOpt::MODE_OFF;
    }
    qDebug() << "New mode index:" << mode_idx;

    uiDockRxOpt->getFilterPreset(mode_idx, filter_preset, &flo, &fhi);
    d_filter_shape = (rx_filter_shape)uiDockRxOpt->currentFilterShape();

    // Yes - RDS belongs inside this controller!
    // bool rds_enabled = rx->is_rds_decoder_active(idx);
    // if (rds_enabled)
    //     setRdsDecoder(false);
    // uiDockRDS->setDisabled();

    switch (mode_idx) {

    case DockRxOpt::MODE_OFF:
        /* Spectrum analyzer only */
        // Yes - Audio recorder state now inside this controller!
        // if (rx->is_recording_audio())
        // {
        //     stopAudioRec();
        //     uiDockAudio->setAudioRecButtonState(false);
        // }
        rx->set_demod(idx, rx_demod::RX_DEMOD_OFF);
        click_res = 1000;
        break;

    case DockRxOpt::MODE_RAW:
        /* Raw I/Q; max 96 ksps*/
        rx->set_demod(idx, rx_demod::RX_DEMOD_NONE);
        // ui->plotter->setDemodRanges(-40000, -200, 200, 40000, true);
        uiDockAudio->setFftRange(0,24000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_AM:
        rx->set_demod(idx, rx_demod::RX_DEMOD_AM);
        // ui->plotter->setDemodRanges(-40000, -200, 200, 40000, true);
        uiDockAudio->setFftRange(0,6000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_AM_SYNC:
        rx->set_demod(idx, rx_demod::RX_DEMOD_AMSYNC);
        // ui->plotter->setDemodRanges(-40000, -200, 200, 40000, true);
        uiDockAudio->setFftRange(0,6000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_NFM:
        // ui->plotter->setDemodRanges(-40000, -1000, 1000, 40000, true);
        uiDockAudio->setFftRange(0, 5000);
        rx->set_demod(idx, rx_demod::RX_DEMOD_NFM);
        rx->set_fm_maxdev(idx, uiDockRxOpt->currentMaxdev());
        rx->set_fm_deemph(idx, uiDockRxOpt->currentEmph());
        click_res = 100;
        break;

    case DockRxOpt::MODE_WFM_MONO:
    case DockRxOpt::MODE_WFM_STEREO:
    case DockRxOpt::MODE_WFM_STEREO_OIRT:
        /* Broadcast FM */
        // ui->plotter->setDemodRanges(-120e3, -10000, 10000, 120e3, true);
        uiDockAudio->setFftRange(0, 24000);  /** FIXME: get audio rate from rx **/
        click_res = 1000;
        if (mode_idx == DockRxOpt::MODE_WFM_MONO)
            rx->set_demod(idx, rx_demod::RX_DEMOD_WFM_M);
        else if (mode_idx == DockRxOpt::MODE_WFM_STEREO_OIRT)
            rx->set_demod(idx, rx_demod::RX_DEMOD_WFM_S_OIRT);
        else
            rx->set_demod(idx, rx_demod::RX_DEMOD_WFM_S);

        // uiDockRDS->setEnabled();
        // if (rds_enabled)
        //     setRdsDecoder(true);
        break;

    case DockRxOpt::MODE_LSB:
        /* LSB */
        rx->set_demod(idx, rx_demod::RX_DEMOD_SSB);
        // ui->plotter->setDemodRanges(-40000, -100, -5000, 0, false);
        uiDockAudio->setFftRange(0,3000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_USB:
        /* USB */
        rx->set_demod(idx, rx_demod::RX_DEMOD_SSB);
        // ui->plotter->setDemodRanges(0, 5000, 100, 40000, false);
        uiDockAudio->setFftRange(0,3000);
        click_res = 100;
        break;

    case DockRxOpt::MODE_CWL:
        /* CW-L */
        rx->set_demod(idx, rx_demod::RX_DEMOD_SSB);
        cwofs = -uiDockRxOpt->getCwOffset();
        // ui->plotter->setDemodRanges(-5000, -100, 100, 5000, true);
        uiDockAudio->setFftRange(0,1500);
        click_res = 10;
        break;

    case DockRxOpt::MODE_CWU:
        /* CW-U */
        rx->set_demod(idx, rx_demod::RX_DEMOD_SSB);
        cwofs = uiDockRxOpt->getCwOffset();
        // ui->plotter->setDemodRanges(-5000, -100, 100, 5000, true);
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
    // ui->plotter->setHiLowCutFrequencies(flo, fhi);
    // ui->plotter->setClickResolution(click_res);
    // ui->plotter->setFilterClickResolution(click_res);

    rx->set_filter(idx, (double)flo, (double)fhi, d_filter_shape);
    rx->set_cw_offset(idx, cwofs);
    rx->set_sql_level(idx, uiDockRxOpt->currentSquelchLevel());

    // remote->setMode(mode_idx);
    // remote->setPassband(flo, fhi);

    d_have_audio = (mode_idx != DockRxOpt::MODE_OFF);

    uiDockRxOpt->setCurrentDemod(mode_idx);
}

/**
 * @brief New FM deviation selected.
 * @param max_dev The enw FM deviation.
 */
void DemodulatorController::setFmMaxdev(float max_dev)
{
    qDebug() << "FM MAX_DEV: " << max_dev;

    /* receiver will check range */
    rx->set_fm_maxdev(idx, max_dev);
}

/**
 * @brief New FM de-emphasis time consant selected.
 * @param tau The new time constant
 */
void DemodulatorController::setFmEmph(double tau)
{
    qDebug() << "FM TAU: " << tau;

    /* receiver will check range */
    rx->set_fm_deemph(idx, tau);
}

/**
 * @brief AM DCR status changed (slot).
 * @param enabled Whether DCR is enabled or not.
 */
void DemodulatorController::setAmDcr(bool enabled)
{
    rx->set_am_dcr(idx, enabled);
}

void DemodulatorController::setCwOffset(int offset)
{
    rx->set_cw_offset(idx, offset);
}

/**
 * @brief AM-Sync DCR status changed (slot).
 * @param enabled Whether DCR is enabled or not.
 */
void DemodulatorController::setAmSyncDcr(bool enabled)
{
    rx->set_amsync_dcr(idx, enabled);
}

/**
 * @brief New AM-Sync PLL BW selected.
 * @param pll_bw The new PLL BW.
 */
void DemodulatorController::setAmSyncPllBw(float pll_bw)
{
    qDebug() << "AM-Sync PLL BW: " << pll_bw;

    /* receiver will check range */
    rx->set_amsync_pll_bw(idx, pll_bw);
}

/**
 * @brief Audio gain changed.
 * @param value The new audio gain in dB.
 */
void DemodulatorController::setAudioGain(float gain)
{
    rx->set_af_gain(idx, gain);
}

/**
 * @brief Set AGC ON/OFF.
 */
void DemodulatorController::setAgcOn(bool agc_on)
{
    rx->set_agc_on(idx, agc_on);
}

/**
 * @brief AGC hang ON/OFF.
 */
void DemodulatorController::setAgcHang(bool use_hang)
{
    rx->set_agc_hang(idx, use_hang);
}

/**
 * @brief AGC threshold changed.
 */
void DemodulatorController::setAgcThreshold(int threshold)
{
    rx->set_agc_threshold(idx, threshold);
}

/**
 *  @brief AGC slope factor changed.
 */
void DemodulatorController::setAgcSlope(int factor)
{
    rx->set_agc_slope(idx, factor);
}

/**
 *  @brief AGC manual gain changed.
 */
void DemodulatorController::setAgcGain(int gain)
{
    rx->set_agc_manual_gain(idx, gain);
}

/**
 *  @brief AGC decay changed.
 */
void DemodulatorController::setAgcDecay(int msec)
{
    rx->set_agc_decay(idx, msec);
}

/**
 * @brief Noise blanker configuration changed.
 * @param nb1 Noise blanker 1 ON/OFF.
 * @param nb2 Noise blanker 2 ON/OFF.
 * @param threshold Noise blanker threshold.
 */
void DemodulatorController::setNoiseBlanker(int nbid, bool on, float threshold)
{
    qDebug() << "Noise blanker" << idx << "NB:" << nbid << " ON:" << on << "THLD:"
             << threshold;

    rx->set_nb_on(idx, nbid, on);
    rx->set_nb_threshold(idx, nbid, threshold);
}

/**
 * @brief Squelch level changed.
 * @param level_db The new squelch level in dBFS.
 */
void DemodulatorController::setSqlLevel(double level_db)
{
    rx->set_sql_level(idx, level_db);
    // ui->sMeter->setSqlLevel(level_db);
}

/**
 * @brief Squelch level auto clicked.
 * @return The new squelch level.
 */
double DemodulatorController::setSqlLevelAuto()
{
    double level = rx->get_signal_pwr(idx, true) + 1.0;
    if (level > -10.0)  // avoid 0 dBFS
        level = uiDockRxOpt->getSqlLevel();

    setSqlLevel(level);
    return level;
}

void DemodulatorController::setPassband(int bandwidth)
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

//        remote->setPassband(lo, hi);
//        on_plotter_newFilterFreq(lo, hi);
}

/* Audio recording and playback */

/**
 * @brief Start audio recorder.
 * @param filename The file name into which audio should be recorded.
 */
void DemodulatorController::startAudioRec(const QString& filename)
{
//    if (!d_have_audio)
//    {
//        QMessageBox msg_box;
//        msg_box.setIcon(QMessageBox::Critical);
//        msg_box.setText(tr("Recording audio requires a demodulator.\n"
//                           "Currently, demodulation is switched off "
//                           "(Mode->Demod off)."));
//        msg_box.exec();
//        uiDockAudio->setAudioRecButtonState(false);
//    }
//    else if (rx->start_audio_recording(filename.toStdString()))
//    {
//        ui->statusBar->showMessage(tr("Error starting audio recorder"));

//        /* reset state of record button */
//        uiDockAudio->setAudioRecButtonState(false);
//    }
//    else
//    {
//        ui->statusBar->showMessage(tr("Recording audio to %1").arg(filename));
//    }
}

/**
 *  @brief Stop audio recorder.
 */
void DemodulatorController::stopAudioRec()
{
//    if (rx->stop_audio_recording())
//    {
//        /* okay, this one would be weird if it really happened */
//        ui->statusBar->showMessage(tr("Error stopping audio recorder"));

//        uiDockAudio->setAudioRecButtonState(true);
//    }
//    else
//    {
//        ui->statusBar->showMessage(tr("Audio recorder stopped"), 5000);
//    }
}

/**
 * @brief Start playback of audio file.
 *
 * TODO: is it *really* that important that this application can
 * do something as mundane as play back an audio file?
 * Why not use something else?
 */
void DemodulatorController::startAudioPlayback(const QString& filename)
{
//    if (rx->start_audio_playback(filename.toStdString()))
//    {
//        ui->statusBar->showMessage(tr("Error trying to play %1").arg(filename));

//        /* reset state of record button */
//        uiDockAudio->setAudioPlayButtonState(false);
//    }
//    else
//    {
//        ui->statusBar->showMessage(tr("Playing %1").arg(filename));
//    }
}

/**
 * @brief Stop playback of audio file.
 */
void DemodulatorController::stopAudioPlayback()
{
//    if (rx->stop_audio_playback())
//    {
//        /* okay, this one would be weird if it really happened */
//        ui->statusBar->showMessage(tr("Error stopping audio playback"));

//        uiDockAudio->setAudioPlayButtonState(true);
//    }
//    else
//    {
//        ui->statusBar->showMessage(tr("Audio playback stopped"), 5000);
//    }
}

/* Audio network streaming */

/**
 * @brief Start streaming audio over UDP.
 *
 * TODO: how is this controller supposed to know that the port is
 * not in use by soe other receiver controller?
 */
void DemodulatorController::startAudioStream(const QString& udp_host, int udp_port, bool stereo)
{
//    rx->start_udp_streaming(udp_host.toStdString(), udp_port, stereo);
}

/**
 * @brief Stop streaming audio over UDP.
 */
void DemodulatorController::stopAudioStreaming()
{
//    rx->stop_udp_streaming();
}

/* RDS */

void DemodulatorController::setRdsDecoder(bool checked)
{
//    if (checked)
//    {
//        qDebug() << "Starting RDS decoder.";
//        uiDockRDS->showEnabled();
//        rx->start_rds_decoder();
//        rx->reset_rds_parser();
//        rds_timer->start(250);
//    }
//    else
//    {
//        qDebug() << "Stopping RDS decoder.";
//        uiDockRDS->showDisabled();
//        rx->stop_rds_decoder();
//        rds_timer->stop();
//    }
}

/* Timers */

void DemodulatorController::setAudioFftRate(int fps)
{
    auto interval = 1000 / fps;

    if (interval < 10)
        return;

    if (audio_fft_timer->isActive()) {
        audio_fft_timer->setInterval(interval);
    }
}

/**
 * @brief Audio FFT plot timeout.
 */
void DemodulatorController::audioFftTimeout()
{
    unsigned int    fftsize;
    unsigned int    i;
    float           pwr;
    float           pwr_scale;
    std::complex<float> pt;             /* a single FFT point used in calculations */

    if (!d_have_audio || !uiDockAudio->isVisible())
        return;

    rx->get_audio_fft_data(idx, d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        qDebug() << "No audio FFT data.";
        return;
    }

    pwr_scale = 1.0 / (fftsize * fftsize);

    /** FIXME: move post processing to rx_fft_f **/
    /* Normalize, calculate power and shift the FFT */
    for (i = 0; i < fftsize; i++)
    {
        /* normalize and shift */
        if (i < fftsize/2)
        {
            pt = d_fftData[fftsize/2+i];
        }
        else
        {
            pt = d_fftData[i-fftsize/2];
        }

        /* calculate power in dBFS */
        pwr = pwr_scale * (pt.imag() * pt.imag() + pt.real() * pt.real());
        d_realFftData[i] = 10.0 * log10f(pwr + 1.0e-20);
    }

    uiDockAudio->setNewFftData(d_realFftData, fftsize);
}

/**
 *  @brief RDS message display timeout.
 */
void DemodulatorController::rdsTimeout()
{
//    std::string buffer;
//    int num;

//    rx->get_rds_data(buffer, num);
//    while(num!=-1) {
//        rx->get_rds_data(buffer, num);
//        uiDockRDS->updateRDS(QString::fromStdString(buffer), num);
//    }
}

void DemodulatorController::enableTimers(bool enabled)
{
    if (enabled) {
        audio_fft_timer->start(40);
    } else {
        audio_fft_timer->stop();
        rds_timer->stop();
    }
}
