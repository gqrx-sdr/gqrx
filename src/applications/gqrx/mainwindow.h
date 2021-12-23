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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <vector>

#include <QColor>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QSvgWidget>

#include <DockManager.h>

#include "qtgui/basebandview.h"
#include "qtgui/dockbookmarks.h"
#include "qtgui/dockdxc.h"
#include "qtgui/dockinputctl.h"
#include "qtgui/dockfft.h"
#include "qtgui/dockiqtool.h"

#include "applications/gqrx/recentconfig.h"
#include "applications/gqrx/remote_control.h"
#include "applications/gqrx/receiver.h"
#include "applications/gqrx/demodulator_controller.h"

namespace Ui {
    class MainWindow;  /*! The main window UI */
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& cfgfile, bool edit_conf, QWidget *parent = nullptr);
    ~MainWindow() override;

    bool loadConfig(const QString& cfgfile, bool check_crash, bool restore_mainwindow);
    bool saveConfig(const QString& cfgfile);
    void storeGuiSettings();
    void storeSession();

    bool configOk; /*!< Main app uses this flag to know whether we should abort or continue. */

public slots:
    void setNewFrequency(qint64 rx_freq);
    void setNewFrequencyWithOffsetFollow(qint64 rx_freq);

private:
    void setNewFrequency(qint64 rx_freq, bool offsetFollow);

    Ui::MainWindow *ui;

    std::shared_ptr<QSettings>  m_settings;  /*!< Application wide settings. */
    QString                     m_cfg_dir;   /*!< Default config dir, e.g. XDG_CONFIG_HOME. */
    QString                     m_last_dir;
    RecentConfig                *m_recent_config; /* Menu File Recent config */

    qint64 d_lnb_lo;  /* LNB LO in Hz. */
    qint64 d_hw_freq;
    qint64 d_hw_freq_start{};
    qint64 d_hw_freq_stop{};

    /* Data for the Baseband FFT display */
    std::complex<float> *d_fftData;
    float               *d_realFftData;
    float               *d_iirFftData;
    float                d_fftAvg;      /*!< FFT averaging parameter set by user (not the true gain). */

    /* Demodulators */
    std::vector<DemodulatorController::sptr> demodCtrls;

    /* Views */
    BasebandView        *uiBaseband;
    QTimer              *iq_fft_timer;

    /* dock widgets & management */
    ads::CDockManager               *uiDockManager;
    std::vector<ads::CDockWidget*>  uiDockWidgets;      // Widgets of Dock* type, wrapped for manager
    DockBookmarks                   *uiDockBookmarks;
    DockDXCluster                   *uiDockDXCluster;
    DockFft                         *uiDockFft;
    DockIQTool                      *uiDockIQTool;
    DockInputCtl                    *uiDockInputCtl;
    QMenu                           *receiversMenu;

    /* Input/receiver controller */
    receiver::sptr      rx;

    /* Network remote control */
    RemoteControl       *remote;
    QList<QMetaObject::Connection> remoteConnections;

    std::map<QString, QVariant> devList;

    // dummy widget to enforce linking to QtSvg
    QSvgWidget      *qsvg_dummy;

private:
    void updateHWFrequencyRange(bool ignore_limits);
    void updateFrequencyRange();
    void updateGainStages(bool read_from_device);
    void showSimpleTextFile(const QString &resource_path,
                            const QString &window_title);
    /* key shortcut */
    void frequencyFocusShortcut();

private slots:
    /* RecentConfig */
    void loadConfigSlot(const QString &cfgfile);

    /* RF Input controls */
    void setLnbLo(double freq_mhz);
    void setAntenna(const QString& antenna);
    void setGain(const QString& name, double gain);
    void setAutoGain(bool enabled);
    void setFreqCorr(double ppm);
    void setIqSwap(bool reversed);
    void setDcCancel(bool enabled);
    void setIqBalance(bool enabled);
    void setIgnoreLimits(bool ignore_limits);

    /* Receivers controls */
    void addDemodulator();
    void removeDemodulator(size_t idx);
    void disconnectRemote();
    void connectRemote(size_t demodIdx);

    void setFreqCtrlReset(bool enabled);
    void setInvertScrolling(bool enabled);
    void setOffsetFollowsHw(bool enabled);

    /* I/Q playback and recording */
    void startIqRecording(const QString& recdir);
    void stopIqRecording();
    void startIqPlayback(const QString& filename, float samprate);
    void stopIqPlayback();
    void seekIqFile(qint64 seek_pos);

    /* FFT settings */
    void setIqFftSize(int size);
    void setIqFftRate(int fps);
    void setIqFftWindow(int type);
    void setIqFftSplit(int pct_wf);
    void setIqFftAvg(float avg);
    void setAudioFftRate(int fps);
    void setFftColor(const QColor& color);
    void setFftFill(bool enable);
    void setPeakDetection(bool enabled);
    void setFftPeakHold(bool enable);
    void setWfTimeSpan(quint64 span_ms);
    void setWfSize();

    /* FFT plot */
    void on_newDemodFreq(size_t idx, qint64 freq, qint64 delta);    /*! New demod freq (aka. filter offset). */
    void on_newFilterFreq(size_t idx, int low, int high);           /*! New filter width */

    /* Bookmarks */
    void onDemodulatorBookmark(size_t idx);
    void onBookmarkActivated(qint64 freq, const QString& demod, int bandwidth);

    /* DXC Spots */
    void updateClusterSpots();

    /* menu and toolbar actions */
    void on_actionDSP_triggered(bool checked);
    int  on_actionIoConfig_triggered();
    void on_actionLoadSettings_triggered();
    void on_actionSaveSettings_triggered();
    void on_actionSaveWaterfall_triggered();
    void on_actionFullScreen_triggered(bool checked);
    void on_actionRemoteControl_triggered(bool checked);
    void on_actionRemoteConfig_triggered();
    void on_actionUserGroup_triggered();
    void on_actionNews_triggered();
    void on_actionRemoteProtocol_triggered();
    void on_actionKbdShortcuts_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();
    void on_actionAddDemodulator_triggered();

    /* window close signals */
    int  firstTimeConfig();

    /* cyclic processing */
    void iqFftTimeout();

    /* event handling */
    virtual void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
