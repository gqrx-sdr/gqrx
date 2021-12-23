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
#include <DockAreaWidget.h>
#include <DockWidget.h>
#include <DockWidgetTab.h>

#include "qtgui/dockafsk1200.h"
#include "qtgui/dockrxopt.h"
#include "qtgui/dockaudio.h"
#include "qtgui/dockrds.h"

#include "applications/gqrx/demodulator.h"
#include "applications/gqrx/receiver.h"

struct FilterRanges
{
    int lowMin;
    int lowMax;
    int highMin;
    int highMax;
    bool symmetric;
    int resolution;
};

class DemodulatorControllerRemoteDelegate : public QObject
{
    Q_OBJECT;

public:
    explicit DemodulatorControllerRemoteDelegate();
    ~DemodulatorControllerRemoteDelegate();

    void setMode(int demod) {
        emit mode(demod);
    }
    void setPassband(int lo, int high) {
        emit passband(lo, high);
    }
    void setSignalLevel(float level) {
        emit signalLevel(level);
    }

signals:
    // Demod -> remote
    void squelchLevel(double);
    void filterOffset(qint64);
    void mode(int);
    void passband(int, int);

    // remote -> Demod
    void newFilterOffset(qint64);
    void newMode(int);
    void newSquelchLevel(double);

    // Audio -> remote
    void stopAudioRecorder();
    void startAudioRecorder(QString);
    void signalLevel(float);

    // remote -> Audio
    void startAudioRecorderEvent();
    void stopAudioRecorderEvent();

    // RDS -> remote
    void setRDSstatus(bool);
    void rdsPI(QString);

    // remote -> RDS
    void newRDSmode(bool);
};

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
        std::shared_ptr<QSettings> settings
    );
    ~DemodulatorController() override;

    void readSettings(std::shared_ptr<QSettings> settings);
    void saveSettings(std::shared_ptr<QSettings> settings);

    DemodulatorControllerRemoteDelegate* getRemote() const {
        return remote;
    }

    QAction* getToolbarAction() const {
        return toolbarAction;
    }

signals:
    void remove(size_t idx);
    void bookmark(size_t idx);
    void centerFFT(size_t idx);
    void focussed(size_t idx);

    void filterOffset(size_t idx, qint64 offset);
    void filterFrequency(size_t idx, int low, int high);
    void filterRanges(size_t idx, int lowMin, int lowMax, int highMin, int highMax, bool symmetric, int resolution);

    void wfColormapChanged(const QString &cmap);

public slots:
    void emitCurrentSettings();

    void onRemoveAction();
    void onBookmarkAction();
    void onCenterFFTAction();

    void onIndexChanged(size_t idx);
    void setFocus();
    void onDockDemodVisbilityChanged(bool visible);

    /* Frequency Control */

    void setFilterOffsetRange(qint64 range);
    void setFrequencyRange(qint64 hw_start, qint64 hw_stop);
    void setHwFrequency(qint64 hw_freq, bool forceOffsetFollow);

    void setFilterRanges(int lowMin, int lowMax, int highMin, int highMax, bool symmetric, int resolution)
    {
        m_filter_ranges.lowMin = lowMin;
        m_filter_ranges.lowMax = lowMax;
        m_filter_ranges.highMin = highMin;
        m_filter_ranges.highMax = highMax;
        m_filter_ranges.symmetric = symmetric;
        m_filter_ranges.resolution = resolution;
    }

    /* UI Behaviour control */

    void setFreqCtrlReset(bool enabled);
    void setInvertScrolling(bool enabled);
    void setOffsetFollowsHw(bool enabled);
    void setFftColor(const QColor& color);
    void setFftFill(bool enable);

    /* Demodulator controls */

    void setFilterOffset(qint64 offset);
    void setFilterFrequency(int low, int high);

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
    float get_signal_pwr() const;

    QString currentDemodAsString();

    /* Audio recording and playback */

    void startAudioRec(const QString& filename);
    void stopAudioRec();
    void startAudioPlayback(const QString& filename);
    void stopAudioPlayback();

    /* Audio network streaming */

    void startAudioStream(const QString& udp_host, int udp_port, bool stereo);
    void stopAudioStreaming();

    /* RDS */

    void setRdsDecoder(bool enabled);

    /* AFSK1200 */

    void setAfskDecoder(bool enabled);

    /* Timers */

    void setAudioFftRate(int fps);
    void meterTimeout();
    void audioFftTimeout();
    void rdsTimeout();
    void asfkTimeout();

    void enableTimers(bool enabled);

private:
    receiver::sptr          rx;                     /*!< The actual receiver DSP controller */
    demodulator::sptr       demod;                  /*!< The actual demodulator DSP controller */

    // Dock management
    ads::CDockManager       *dockMgr;               // Borrowed from MainWindow
    QMenu                   *viewMenu;              // Borrowed from MainWindow
    QAction                 *toolbarAction;
    QAction                 *viewMenuSection;

    // Rx controls
    DockRxOpt               *uiDockRxOpt;           /*!< Dock for the Rx settings */
    ads::CDockWidget        *dockDemod;             /*!< Wrapped dock widget for DockManager */
    bool                    d_have_audio;           /*!< Whether we have audio (i.e. not with demod_off. */
    enum rx_filter_shape    d_filter_shape;         /*!< The las used filter shape */
    bool                    d_offset_follows_hw;    /*!< Whether changing HW freq also changes offset */
    QTimer                  *meter_timer;
    FilterRanges            m_filter_ranges;

    // Audio FFT display
    DockAudio               *uiDockAudio;           /*!< Dock for the Audio display and control */
    ads::CDockWidget        *dockAudio;             /*!< Wrapped dock widget for DockManager */
    std::complex<float>     *d_fftData;
    float                   *d_realFftData;
    QTimer                  *audio_fft_timer;
    QAction                 *stopAudioRecAction;

    // UDP control
    QAction                 *stopUDPStreamAction;

    // RDS
    DockRDS                 *uiDockRDS;
    ads::CDockWidget        *dockRDS;               /*!< Wrapped dock widget for DockManager */
    QTimer                  *rds_timer;
    bool                    dec_rds;

    // ASFK1200
    DockAFSK1200            *uiDockAFSK;
    ads::CDockWidget        *dockAFSK;
    QTimer                  *afsk_timer;

    // Remote control
    DemodulatorControllerRemoteDelegate *remote;
};

#endif // DEMODULATOR_CONTROLLER_H
