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
#include <QMenu>

#include <DockManager.h>

#include "qtgui/dockrxopt.h"
#include "qtgui/dockaudio.h"
#include "qtgui/dockrds.h"

#include "applications/gqrx/demodulator.h"
#include "applications/gqrx/receiver.h"

class DemodulatorController : public QObject
{
    Q_OBJECT;

public:
    typedef std::shared_ptr<DemodulatorController> sptr;

    explicit DemodulatorController(
        receiver::sptr rx,
        demodulator::sptr demod,
        ads::CDockManager *dockMgr,
        QMenu *viewMenu,
        QSettings *settings
    );
    ~DemodulatorController() override;

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

signals:
    void remove(size_t idx);

public slots:
    void onRemoveAction();
    void onIndexChanged(size_t idx);

    /* Frequency Control */

    void setFilterOffsetRange(qint64 range);
    void setFrequencyRange(qint64 hw_start, qint64 hw_stop);
    void setHwFrequency(qint64 hw_freq);
    void setFilterFrequency(int low, int high);

    /* UI Behaviour control */

    void setFreqCtrlReset(bool enabled);
    void setInvertScrolling(bool enabled);
    void setOffsetFollowsHw(bool enabled);
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
    void meterTimeout();

    void enableTimers(bool enabled);

private:
    receiver::sptr      rx;     /*!< The actual receiver DSP controller */
    demodulator::sptr   demod;  /*!< The actual demodulator DSP controller */

    // Dock management
    ads::CDockManager       *dockMgr;           // Borrowed from MainWindow
    QMenu                   *viewMenu;          // Borrowed from MainWindow
    QAction                 *viewMenuSection;

    // Rx controls
    DockRxOpt               *uiDockRxOpt;           /*!< Dock for the Rx settings */
    ads::CDockWidget        *dockDemod;             /*!< Wrapped dock widget for DockManager */
    bool                    d_have_audio;           /*!< Whether we have audio (i.e. not with demod_off. */
    enum rx_filter_shape    d_filter_shape;         /*!< The las used filter shape */
    bool                    d_offset_follows_hw;    /*!< Whether changing HW freq also changes offset */
    QTimer                  *meter_timer;

    // Audio FFT display
    DockAudio               *uiDockAudio;       /*!< Dock for the Audio display and control */
    ads::CDockWidget        *dockAudio;         /*!< Wrapped dock widget for DockManager */
    std::complex<float>     *d_fftData;
    float                   *d_realFftData;
    QTimer                  *audio_fft_timer;
    QAction                 *stopAudioRecAction;

    // UDP control
    QAction                 *stopUDPStreamAction;

    // RDS
    DockRDS                 *uiDockRDS;
    ads::CDockWidget        *dockRDS;           /*!< Wrapped dock widget for DockManager */
    QTimer                  *rds_timer;
    bool                    dec_rds;
};

#endif // DEMODULATOR_CONTROLLER_H
