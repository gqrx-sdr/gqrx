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
#ifndef   DEMODULATOR_CONTROLLER_H
#define   DEMODULATOR_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QMainWindow>

#include <DockManager.h>

#include "qtgui/dockrxopt.h"
#include "qtgui/dockaudio.h"
#include "qtgui/dockrds.h"

#include "applications/gqrx/demodulator.h"
#include "applications/gqrx/receiver.h"

// TODO: This could be a dock UI component itself, encapsulating demod and audio?
class DemodulatorController : public QObject
{
    Q_OBJECT;

public:
    typedef std::shared_ptr<DemodulatorController> sptr;

    explicit DemodulatorController(
        receiver::sptr rx,
        demodulator::sptr demod,
        ads::CDockManager *dockMgr
    );
    ~DemodulatorController() override;

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

    void ensureOffsetInRange(qint64 freq, qint64 lnb_lo, qint64 hw_freq_start, qint64 hw_freq_stop);

signals:
    void remove(size_t idx);
    void updateFrequencyRange();

public slots:
    void onRemoveAction();

    /* Frequency Control */

    void setFilterOffsetRange(qint64 range);
    void setFrequencyRange(qint64 hw_start, qint64 hw_stop);
    void setHwFrequency(qint64 rx_freq, qint64 d_lnb_lo);
    void setFilterFrequency(int low, int high);

    /* UI Behaviour control */

    void setFreqCtrlReset(bool enabled);
    void setInvertScrolling(bool enabled);
    void setFftColor(const QColor& color);
    void setFftFill(bool enable);

    /* Rx controls */

    void setFilterOffset(qint64 freq_hz);
    void selectDemod(const QString& demod);
    void selectDemod(int mode_idx);
    void setFmMaxdev(float max_dev);
    void setFmEmph(double tau);
    void setAmDcr(bool enabled);
    void setCwOffset(int offset);
    void setAmSyncDcr(bool enabled);
    void setAmSyncPllBw(float pll_bw);
    void setAudioGain(float gain);
    void setAgcOn(bool agc_on);
    void setAgcHang(bool use_hang);
    void setAgcThreshold(int threshold);
    void setAgcSlope(int factor);
    void setAgcGain(int gain);
    void setAgcDecay(int msec);
    void setNoiseBlanker(int nbid, bool on, float threshold);
    void setSqlLevel(double level_db);
    double setSqlLevelAuto();
    void setPassband(int bandwidth);
    float get_signal_pwr(bool dbfs) const;

    /* Audio recording and playback */

    void startAudioRec(const QString& filename);
    void stopAudioRec();
    void startAudioPlayback(const QString& filename);
    void stopAudioPlayback();

    /* Audio network streaming */

    void startAudioStream(const QString& udp_host, int udp_port, bool stereo);
    void stopAudioStreaming();

    /* RDS */

    void setRdsDecoder(bool checked);

    /* Timers */

    void setAudioFftRate(int fps);
    void audioFftTimeout();
    void rdsTimeout();
    void enableTimers(bool enabled);

private:
    receiver::sptr      rx;     /*!< The actual receiver DSP controller */
    demodulator::sptr   demod;  /*!< The actual demodulator DSP controller */

    // Rx controls

    DockRxOpt               *uiDockRxOpt;   /*!< Dock for the Rx settings */
    bool                    d_have_audio;   /*!< Whether we have audio (i.e. not with demod_off. */
    enum rx_filter_shape    d_filter_shape; /*!< The las used filter shape */

    // Audio FFT display
    DockAudio           *uiDockAudio;       /*!< Dock for the Audio display and control */
    std::complex<float> *d_fftData;
    float               *d_realFftData;
    QTimer              *audio_fft_timer;

    // RDS
    DockRDS     *uiDockRDS;
    QTimer      *rds_timer;
    bool        dec_rds;
};

#endif // DEMODULATOR_CONTROLLER_H
