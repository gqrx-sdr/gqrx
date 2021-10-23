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

#include <QDebug>
#include <QMessageBox>

#include "applications/gqrx/demodulator_controller.h"


#define DATA_BUFFER_SIZE 48000


DemodulatorController::DemodulatorController(
    receiver::sptr rx,
    demodulator::sptr demod,
    ads::CDockManager *dockMgr,
    QMenu *viewMenu,
    std::shared_ptr<QSettings> settings
) :
    rx(rx),
    demod(demod),
    dockMgr(dockMgr),
    viewMenu(viewMenu),
    d_have_audio(true),
    d_offset_follows_hw(false)
{
    d_filter_shape = rx_filter_shape::FILTER_SHAPE_NORMAL;

    connect(demod.get(), SIGNAL(indexChanged(size_t)), this, SLOT(onIndexChanged(size_t)));

    auto num = demod->get_idx() + 1;

    viewMenuSection = viewMenu->addSection(QString("Receiver %0").arg(num));

    uiDockRxOpt = new DockRxOpt();
    dockDemod = new ads::CDockWidget(QString("Demod %0").arg(num));
    dockDemod->setWidget(uiDockRxOpt);
    viewMenu->addAction(dockDemod->toggleViewAction());

    uiDockAudio = new DockAudio();
    dockAudio = new ads::CDockWidget(QString("Audio %0").arg(num));
    dockAudio->setWidget(uiDockAudio);
    viewMenu->addAction(dockAudio->toggleViewAction());

    uiDockRDS = new DockRDS();
    dockRDS = new ads::CDockWidget(QString("RDS %0").arg(num));
    dockRDS->setWidget(uiDockRDS);
    viewMenu->addAction(dockRDS->toggleViewAction());

    uiDockAFSK = new DockAFSK1200();
    dockAFSK = new ads::CDockWidget(QString("AFSK1200 %0").arg(num));
    dockAFSK->setWidget(uiDockAFSK);
    viewMenu->addAction(dockAFSK->toggleViewAction());

    dockMgr->addDockWidgetTab(ads::BottomDockWidgetArea, dockDemod);
    dockMgr->addDockWidgetTab(ads::BottomDockWidgetArea, dockAudio);
    dockMgr->addDockWidgetTab(ads::BottomDockWidgetArea, dockRDS);
    dockMgr->addDockWidgetTab(ads::BottomDockWidgetArea, dockAFSK);
    dockRDS->closeDockWidget();
    dockAFSK->closeDockWidget();

    // Set titles and colours
    onIndexChanged(demod->get_idx());

    uiDockRxOpt->setupShortcuts();

    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new float[MAX_FFT_SIZE];

    // update the real rx freq for audio recorder filename
    connect(uiDockRxOpt, SIGNAL(rxFreqChanged(qint64)), uiDockAudio, SLOT(setRxFrequency(qint64)));

    // Rx control
    connect(uiDockRxOpt, SIGNAL(remove()), this, SLOT(onRemoveAction()));
    connect(uiDockRxOpt, SIGNAL(bookmark()), this, SLOT(onBookmarkAction()));

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

    // meter timer
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

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
     connect(this, SIGNAL(wfColormapChanged(QString)), uiDockAudio, SLOT(setWfColormap(QString)));

    audio_fft_timer = new QTimer(this);
    connect(audio_fft_timer, SIGNAL(timeout()), this, SLOT(audioFftTimeout()));

    stopAudioRecAction = new QAction("Stop recording");
    stopAudioRecAction->setEnabled(false);
    viewMenu->addAction(stopAudioRecAction);
    connect(stopAudioRecAction, SIGNAL(triggered()), this, SLOT(stopAudioRec()));

    stopUDPStreamAction = new QAction("Stop UDP stream");
    stopUDPStreamAction->setEnabled(false);
    viewMenu->addAction(stopUDPStreamAction);
    connect(stopUDPStreamAction, SIGNAL(triggered()), this, SLOT(stopAudioStreaming()));

    // Audio remote control
    // connect(uiDockAudio, SIGNAL(audioRecStopped()), remote, SLOT(stopAudioRecorder()));
    // connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), remote, SLOT(startAudioRecorder(QString)));
    // connect(remote, SIGNAL(startAudioRecorderEvent()), uiDockAudio, SLOT(startAudioRecorder()));
    // connect(remote, SIGNAL(stopAudioRecorderEvent()), uiDockAudio, SLOT(stopAudioRecorder()));

    // RDS
    // connect(remote, SIGNAL(newRDSmode(bool)), uiDockRDS, SLOT(setRDSmode(bool)));
    // connect(uiDockRDS, SIGNAL(rdsDecoderToggled(bool)), remote, SLOT(setRDSstatus(bool)));
    // connect(uiDockRDS, SIGNAL(rdsPI(QString)), remote, SLOT(rdsPI(QString)));
    connect(uiDockRDS, SIGNAL(rdsDecoderToggled(bool)), this, SLOT(setRdsDecoder(bool)));
    rds_timer = new QTimer(this);
    connect(rds_timer, SIGNAL(timeout()), this, SLOT(rdsTimeout()));

    // AFSK
    connect(uiDockAFSK, SIGNAL(afskDecoderToggled(bool)), this, SLOT(setAfskDecoder(bool)));
    afsk_timer = new QTimer(this);
    connect(afsk_timer, SIGNAL(timeout()), this, SLOT(asfkTimeout()));
}

DemodulatorController::~DemodulatorController()
{
    // qInfo() << "DemodulatorController::~DemodulatorController begin";

    meter_timer->stop();
    meter_timer->deleteLater();

    audio_fft_timer->stop();
    audio_fft_timer->deleteLater();
    stopAudioRecAction->deleteLater();
    stopUDPStreamAction->deleteLater();

    rds_timer->stop();
    rds_timer->deleteLater();

    afsk_timer->stop();
    afsk_timer->deleteLater();

    rx->remove_demodulator(demod->get_idx());

    delete [] d_fftData;
    delete [] d_realFftData;

    dockMgr->removeDockWidget(dockDemod);
    dockMgr->removeDockWidget(dockAudio);
    dockMgr->removeDockWidget(dockRDS);
    dockMgr->removeDockWidget(dockAFSK);

    dockDemod->deleteLater();
    dockAudio->deleteLater();
    dockRDS->deleteLater();
    dockAFSK->deleteLater();

    viewMenu->removeAction(viewMenuSection);
    viewMenuSection->deleteLater();

    // qInfo() << "DemodulatorController::~DemodulatorController done";
}

void DemodulatorController::readSettings(std::shared_ptr<QSettings> settings)
{
//     qInfo() << "DemodulatorController::readSettings" << demod->get_idx();

    settings->beginGroup("receiver");
    settings->beginGroup(QString("%0").arg(demod->get_idx()));

    bool conv_ok = false;
    rx_filter_shape fshape = (rx_filter_shape)settings->value("filter_shape", 0).toInt(&conv_ok);
    if (conv_ok) {
        d_filter_shape = fshape;
    }
    int flo = settings->value("filter_low_cut", 0).toInt(&conv_ok);
    int fhi = settings->value("filter_high_cut", 0).toInt(&conv_ok);

    settings->endGroup(); // idx
    settings->endGroup(); // receiver

    if (conv_ok)
    {
        setFilterFrequency(flo, fhi);
    }

    setHwFrequency(rx->get_rf_freq(), false);
    setFilterOffset(demod->get_filter_offset());
    setFilterOffsetRange(rx->get_input_rate());

    uiDockRxOpt->readSettings(settings, demod->get_idx());
    uiDockAudio->readSettings(settings, demod->get_idx());
}

void DemodulatorController::saveSettings(std::shared_ptr<QSettings> settings)
{
    uiDockRxOpt->saveSettings(settings, demod->get_idx());
    uiDockAudio->saveSettings(settings, demod->get_idx());

    settings->beginGroup("receiver");
    settings->beginGroup(QString("%0").arg(demod->get_idx()));

    settings->setValue("filter_low_cut", demod->get_filter_lowcut());
    settings->setValue("filter_high_cut", demod->get_filter_highcut());
    settings->setValue("filter_shape", (int)demod->get_filter_shape());

    settings->endGroup(); // idx
    settings->endGroup(); // receiver
}

void DemodulatorController::emitCurrentSettings()
{
    emit filterOffset(demod->get_idx(), demod->get_filter_offset());
    emit filterFrequency(demod->get_idx(), demod->get_filter_lowcut(), demod->get_filter_highcut());
    emit filterRanges(
        demod->get_idx(),
        m_filter_ranges.lowMin,
        m_filter_ranges.lowMax,
        m_filter_ranges.highMin,
        m_filter_ranges.highMax,
        m_filter_ranges.symmetric,
        m_filter_ranges.resolution
    );
}

void DemodulatorController::onRemoveAction()
{
    // qInfo() << "DemodulatorController" << this << "::onRemoveAction for demod idx" << demod->get_idx();
    emit remove(demod->get_idx());
}

void DemodulatorController::onBookmarkAction()
{
    emit bookmark(demod->get_idx());
}

void DemodulatorController::onIndexChanged(size_t idx)
{
    auto num = idx + 1;
    viewMenuSection->setText(QString("Receiver %0").arg(num));

    // all the tab widgets have the same style, we can copy from the first and apply to all
    auto nextStyle = dockDemod->tabWidget()->styleSheet() + "\n" +
            QString("ads--CDockWidgetTab[focused=\"true\"]   { background-color: hsv(%0, 240, 240); }").arg(15 * idx) + "\n" +
            QString("ads--CDockWidgetTab[focused=\"true\"] * { color: white; }") + "\n" +
            QString("ads--CDockWidgetTab                     { background-color: hsv(%0, 240, 240); }").arg(15 * idx) + "\n" +
            QString("ads--CDockWidgetTab                   * { color: #222; }");

    dockDemod->setObjectName(QString("Demod %0").arg(num));
    dockDemod->setWindowTitle(QString("Demod %0").arg(num));
    dockDemod->tabWidget()->setStyleSheet(nextStyle);

    dockAudio->setObjectName(QString("Audio %0").arg(num));
    dockAudio->setWindowTitle(QString("Audio %0").arg(num));
    dockAudio->tabWidget()->setStyleSheet(nextStyle);

    dockRDS->setObjectName(QString("RDS %0").arg(num));
    dockRDS->setWindowTitle(QString("RDS %0").arg(num));
    dockRDS->tabWidget()->setStyleSheet(nextStyle);

    dockAFSK->setObjectName(QString("ASFK1200 %0").arg(num));
    dockAFSK->setWindowTitle(QString("ASFK1200 %0").arg(num));
    dockAFSK->tabWidget()->setStyleSheet(nextStyle);

    emitCurrentSettings();
}

/* Frequency Control */

void DemodulatorController::setFilterOffsetRange(qint64 range)
{
    uiDockRxOpt->setFilterOffsetRange(range);
}

void DemodulatorController::setFrequencyRange(qint64 hw_start, qint64 hw_stop)
{
    uiDockRxOpt->setRxFreqRange(hw_start, hw_stop);
}

void DemodulatorController::setHwFrequency(qint64 hw_freq, bool forceOffsetFollow)
{
//    qInfo() << "DemodulatorController::setHwFrequency" << hw_freq << forceOffsetFollow;
    uiDockRxOpt->setHwFreq(hw_freq, forceOffsetFollow || d_offset_follows_hw);
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

void DemodulatorController::setOffsetFollowsHw(bool enabled)
{
    d_offset_follows_hw = enabled;
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
 * @param offset The new filter offset in Hz.
 */
void DemodulatorController::setFilterOffset(qint64 offset)
{
//     qInfo() << "DemodulatorController::setFilterOffset" << demod->get_idx() << offset;

    demod->set_filter_offset(offset);

    uiDockRxOpt->blockSignals(true);
    uiDockRxOpt->setFilterOffset(offset);
    uiDockRxOpt->blockSignals(false);

    if (demod->is_rds_decoder_active()) {
        demod->reset_rds_parser();
    }

    emit filterOffset(demod->get_idx(), offset);
}

void DemodulatorController::setFilterFrequency(int low, int high)
{
    /* parameter correctness will be checked in receiver class */
    rx_status retcode = demod->set_filter((double) low, (double) high, d_filter_shape);

    if (retcode == rx_status::STATUS_OK) {
        uiDockRxOpt->setFilterParam(low, high);
    }

    emit filterFrequency(demod->get_idx(), low, high);
}

/**
 * @brief Select new demodulator.
 * @param demod New demodulator.
 */
void DemodulatorController::selectDemod(const QString& demod)
{
    int iDemodIndex = DockRxOpt::GetEnumForModulationString(demod);
//    qInfo() << "selectDemod(str):" << demod << "-> IDX:" << iDemodIndex;

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
    int     flo=0, fhi=0;

    // validate mode_idx
    if (mode_idx < DockRxOpt::MODE_OFF || mode_idx >= DockRxOpt::MODE_LAST)
    {
        qInfo() << "Invalid mode index:" << mode_idx;
        mode_idx = DockRxOpt::MODE_OFF;
    }
//    qInfo() << "New mode index:" << mode_idx;

    uiDockRxOpt->getFilterPreset(mode_idx, filter_preset, &flo, &fhi);
    d_filter_shape = (rx_filter_shape)uiDockRxOpt->currentFilterShape();

    bool rds_enabled = demod->is_rds_decoder_active();
    if (rds_enabled) {
        setRdsDecoder(false);
    }
    uiDockRDS->setDisabled();

    switch (mode_idx) {

    case DockRxOpt::MODE_OFF:
        /* Spectrum analyzer only */
        // Yes - Audio recorder state now inside this controller!
        // if (rx->is_recording_audio())
        // {
        //     stopAudioRec();
        //     uiDockAudio->setAudioRecButtonState(false);
        // }

        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_OFF);
        break;

    case DockRxOpt::MODE_RAW:
        /* Raw I/Q; max 96 ksps*/
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_NONE);
        setFilterRanges(-40000, -200, 200, 40000, true, 100);
        uiDockAudio->setFftRange(0,24000);
        break;

    case DockRxOpt::MODE_AM:
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_AM);
        setFilterRanges(-40000, -200, 200, 40000, true, 100);
        uiDockAudio->setFftRange(0,6000);
        break;

    case DockRxOpt::MODE_AM_SYNC:
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_AMSYNC);
        setFilterRanges(-40000, -200, 200, 40000, true, 100);
        uiDockAudio->setFftRange(0,6000);
        break;

    case DockRxOpt::MODE_NFM:
        setFilterRanges(-40000, -1000, 1000, 40000, true, 100);
        uiDockAudio->setFftRange(0, 5000);

        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_NFM);
        demod->set_fm_maxdev(uiDockRxOpt->currentMaxdev());
        demod->set_fm_deemph(uiDockRxOpt->currentEmph());
        break;

    case DockRxOpt::MODE_WFM_MONO:
    case DockRxOpt::MODE_WFM_STEREO:
    case DockRxOpt::MODE_WFM_STEREO_OIRT:
        /* Broadcast FM */
        setFilterRanges(-120e3, -10000, 10000, 120e3, true, 1000);
        uiDockAudio->setFftRange(0, demod->get_audio_rate());

        // must set demod via rx due to flowgraph reconfiguration
        if (mode_idx == DockRxOpt::MODE_WFM_MONO)
            rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_WFM_M);
        else if (mode_idx == DockRxOpt::MODE_WFM_STEREO_OIRT)
            rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_WFM_S_OIRT);
        else
            rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_WFM_S);

        uiDockRDS->setEnabled();
        if (rds_enabled) {
            setRdsDecoder(true);
        }
        break;

    case DockRxOpt::MODE_LSB:
        /* LSB */
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_SSB);
        setFilterRanges(-40000, -100, -5000, 0, false, 100);
        uiDockAudio->setFftRange(0,3000);
        break;

    case DockRxOpt::MODE_USB:
        /* USB */
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_SSB);
        setFilterRanges(0, 5000, 100, 40000, false, 100);
        uiDockAudio->setFftRange(0,3000);
        break;

    case DockRxOpt::MODE_CWL:
        /* CW-L */
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_SSB);
        cwofs = -uiDockRxOpt->getCwOffset();
        setFilterRanges(-5000, -100, 100, 5000, true, 10);
        uiDockAudio->setFftRange(0,1500);
        break;

    case DockRxOpt::MODE_CWU:
        /* CW-U */
        // must set demod via rx due to flowgraph reconfiguration
        rx->set_demod(demod->get_idx(), rx_demod::RX_DEMOD_SSB);
        cwofs = uiDockRxOpt->getCwOffset();
        setFilterRanges(-5000, -100, 100, 5000, true, 10);
        uiDockAudio->setFftRange(0,1500);
        break;

    default:
        qInfo() << "Unsupported mode selection (can't happen!): " << mode_idx;
        flo = -5000;
        fhi = 5000;
        break;
    }

//    qInfo() << "Filter preset for mode" << mode_idx << "LO:" << flo << "HI:" << fhi;

    demod->set_filter((double)flo, (double)fhi, d_filter_shape);
    demod->set_cw_offset(cwofs);
    demod->set_sql_level(uiDockRxOpt->currentSquelchLevel());

    // remote->setMode(mode_idx);
    // remote->setPassband(flo, fhi);

    d_have_audio = (mode_idx != DockRxOpt::MODE_OFF);

    uiDockRxOpt->setCurrentDemod(mode_idx);

    emitCurrentSettings();
}

/**
 * @brief New FM deviation selected.
 * @param max_dev The enw FM deviation.
 */
void DemodulatorController::setFmMaxdev(float max_dev)
{
//    qInfo() << "FM MAX_DEV: " << max_dev;

    /* receiver will check range */
    demod->set_fm_maxdev(max_dev);
}

/**
 * @brief New FM de-emphasis time consant selected.
 * @param tau The new time constant
 */
void DemodulatorController::setFmEmph(double tau)
{
//    qInfo() << "FM TAU: " << tau;

    /* receiver will check range */
    demod->set_fm_deemph(tau);
}

/**
 * @brief AM DCR status changed (slot).
 * @param enabled Whether DCR is enabled or not.
 */
void DemodulatorController::setAmDcr(bool enabled)
{
    demod->set_am_dcr(enabled);
}

void DemodulatorController::setCwOffset(int offset)
{
    demod->set_cw_offset(offset);
}

/**
 * @brief AM-Sync DCR status changed (slot).
 * @param enabled Whether DCR is enabled or not.
 */
void DemodulatorController::setAmSyncDcr(bool enabled)
{
    demod->set_amsync_dcr(enabled);
}

/**
 * @brief New AM-Sync PLL BW selected.
 * @param pll_bw The new PLL BW.
 */
void DemodulatorController::setAmSyncPllBw(float pll_bw)
{
//    qInfo() << "AM-Sync PLL BW: " << pll_bw;

    /* receiver will check range */
    demod->set_amsync_pll_bw(pll_bw);
}

/**
 * @brief Audio gain changed.
 * @param value The new audio gain in dB.
 */
void DemodulatorController::setAudioGain(float gain)
{
    demod->set_af_gain(gain);
}

/**
 * @brief Set AGC ON/OFF.
 */
void DemodulatorController::setAgcOn(bool agc_on)
{
    demod->set_agc_on(agc_on);
}

/**
 * @brief AGC hang ON/OFF.
 */
void DemodulatorController::setAgcHang(bool use_hang)
{
    demod->set_agc_hang(use_hang);
}

/**
 * @brief AGC threshold changed.
 */
void DemodulatorController::setAgcThreshold(int threshold)
{
    demod->set_agc_threshold(threshold);
}

/**
 *  @brief AGC slope factor changed.
 */
void DemodulatorController::setAgcSlope(int factor)
{
    demod->set_agc_slope(factor);
}

/**
 *  @brief AGC manual gain changed.
 */
void DemodulatorController::setAgcGain(int gain)
{
    demod->set_agc_manual_gain(gain);
}

/**
 *  @brief AGC decay changed.
 */
void DemodulatorController::setAgcDecay(int msec)
{
    demod->set_agc_decay(msec);
}

/**
 * @brief Noise blanker configuration changed.
 * @param nb1 Noise blanker 1 ON/OFF.
 * @param nb2 Noise blanker 2 ON/OFF.
 * @param threshold Noise blanker threshold.
 */
void DemodulatorController::setNoiseBlanker(int nbid, bool on, float threshold)
{
//    qInfo() << "Noise blanker" << demod->get_idx() << "NB:" << nbid << " ON:" << on << "THLD:" << threshold;

    demod->set_nb_on(nbid, on);
    demod->set_nb_threshold(nbid, threshold);
}

/**
 * @brief Squelch level changed.
 * @param level_db The new squelch level in dBFS.
 */
void DemodulatorController::setSqlLevel(double level_db)
{
//    qInfo() << "DemodulatorController::setSqlLevel" << level_db;
    demod->set_sql_level(level_db);
}

/**
 * @brief Squelch level auto clicked.
 * @return The new squelch level.
 */
double DemodulatorController::setSqlLevelAuto()
{
    double level = demod->get_signal_pwr(true) + 1.0;
    if (level > -10.0) {
        // avoid 0 dBFS
        level = uiDockRxOpt->getSqlLevel();
    }

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

    emit filterFrequency(demod->get_idx(), lo, hi);
}

float DemodulatorController::get_signal_pwr(bool dbfs) const
{
    return demod->get_signal_pwr(dbfs);
}

QString DemodulatorController::currentDemodAsString()
{
    return uiDockRxOpt->currentDemodAsString();
}

/* Audio recording and playback */

/**
 * @brief Start audio recorder.
 * @param filename The file name into which audio should be recorded.
 */
void DemodulatorController::startAudioRec(const QString& filename)
{
    if (!d_have_audio)
    {
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("Recording audio requires a demodulator.\n"
                           "Currently, demodulation is switched off "
                           "(Mode->Demod off)."));
        msg_box.exec();
        uiDockAudio->setAudioRecButtonState(false);
    }
    else if (demod->start_audio_recording(filename.toStdString()) != rx_status::STATUS_OK)
    {
        qInfo() << tr("Error starting audio recorder");

        /* reset state of record button */
        uiDockAudio->setAudioRecButtonState(false);
    }
    else
    {
//        qInfo() << tr("Recording audio to %1").arg(filename);
        stopAudioRecAction->setEnabled(true);
    }
}

/**
 *  @brief Stop audio recorder.
 */
void DemodulatorController::stopAudioRec()
{
    if (demod->stop_audio_recording() != rx_status::STATUS_OK)
    {
        qInfo() << tr("Error stopping audio recorder");
        uiDockAudio->setAudioRecButtonState(true);
    }
    else
    {
        uiDockAudio->setAudioRecButtonState(false);
        stopAudioRecAction->setEnabled(false);
    }
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
 */
void DemodulatorController::startAudioStream(const QString& udp_host, int udp_port, bool stereo)
{
//    qInfo() << "DemodulatorController::startAudioStream";
    demod->start_udp_streaming(udp_host.toStdString(), udp_port, stereo);
    stopUDPStreamAction->setEnabled(true);
}

/**
 * @brief Stop streaming audio over UDP.
 */
void DemodulatorController::stopAudioStreaming()
{
    demod->stop_udp_streaming();
    stopUDPStreamAction->setEnabled(false);
    uiDockAudio->setAudioStreamButtonState(false);
}

/* RDS */

void DemodulatorController::setRdsDecoder(bool enabled)
{
    if (enabled)
    {
//        qInfo() << "Starting RDS decoder.";
        uiDockRDS->showEnabled();
        demod->start_rds_decoder();
        demod->reset_rds_parser();
        rds_timer->start(250);
    }
    else
    {
//        qInfo() << "Stopping RDS decoder.";
        uiDockRDS->showDisabled();
        demod->stop_rds_decoder();
        rds_timer->stop();
    }
}

/* AFSK 1200 */

void DemodulatorController::setAfskDecoder(bool enabled)
{
//    qInfo() << "Set AFSK1200 decoder" << enabled;
    if (enabled)
    {

        /* start sample sniffer */
        if (demod->start_sniffer(22050, DATA_BUFFER_SIZE) == rx_status::STATUS_OK)
        {
            afsk_timer->start(100);
        }
        else
            QMessageBox::warning(uiDockAFSK, tr("Gqrx error"),
                                 tr("Error starting sample sniffer.\n"
                                    "Close all data decoders and try again."),
                                 QMessageBox::Ok, QMessageBox::Ok);
    }
    else
    {
        afsk_timer->stop();
        demod->stop_sniffer();
    }
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

/** Signal strength meter timeout. */
void DemodulatorController::meterTimeout()
{
    float level;
    level = demod->get_signal_pwr(true);
    uiDockRxOpt->setSignalLevel(level);
    // remote->setSignalLevel(level);
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

    demod->get_audio_fft_data(d_fftData, fftsize);

    if (fftsize == 0)
    {
        /* nothing to do, wait until next activation. */
        qInfo() << "No audio FFT data.";
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
    std::string buffer;
    int num;

    demod->get_rds_data(buffer, num);
    while (num != -1) {
        demod->get_rds_data(buffer, num);
        uiDockRDS->updateRDS(QString::fromStdString(buffer), num);
    }
}

/**
 * Periodic processing for acquiring samples from receiver and processing them
 * with data decoders (see dec_* objects)
 */
void DemodulatorController::asfkTimeout()
{
    float buffer[DATA_BUFFER_SIZE];
    unsigned int num;

    demod->get_sniffer_data(&buffer[0], num);
    uiDockAFSK->process_samples(&buffer[0], num);
}

void DemodulatorController::enableTimers(bool enabled)
{
    if (enabled) {
        meter_timer->start(100);
        audio_fft_timer->start(40);
    } else {
        meter_timer->stop();
        audio_fft_timer->stop();
        rds_timer->stop();
    }
}
