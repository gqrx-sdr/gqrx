/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#ifndef DOCKRXOPT_H
#define DOCKRXOPT_H

#include <QDockWidget>
#include <QSettings>
#include <QMenu>
#include "qtgui/agc_options.h"
#include "qtgui/demod_options.h"
#include "qtgui/nb_options.h"
#include "receivers/defines.h"
#include "receivers/modulations.h"

namespace Ui {
    class DockRxOpt;
}


/**
 * @brief Dock window with receiver options.
 * @ingroup UI
 *
 * This dock widget encapsulates the receiver options. The controls
 * are grouped in a tool box that allows packing many controls in little space.
 * The UI itself is in the dockrxopt.ui file.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 */
class DockRxOpt : public QDockWidget
{
    Q_OBJECT

public:

    explicit DockRxOpt(qint64 filterOffsetRange = 90000, QWidget *parent = 0);
    ~DockRxOpt();

    void setFilterOffsetRange(qint64 range_hz);

    void setFilterParam(int lo, int hi);
    void setCurrentFilter(int index);
    int  currentFilter() const;

    void setCurrentFilterShape(int index);
    int  currentFilterShape() const;

    void setHwFreq(qint64 freq_hz);
    void setRxFreqRange(qint64 min_hz, qint64 max_hz);

    void setResetLowerDigits(bool enabled);
    void setInvertScrolling(bool enabled);

    Modulations::idx  currentDemod() const;
    QString currentDemodAsString();

    float currentMaxdev() const;
    double currentEmph() const;
    double currentSquelchLevel() const;

    int     getCwOffset() const;
    void    setCwOffset(int offset);

    double  getSqlLevel(void) const;

    bool    getAgcOn();
    void    setAgcOn(bool on);
    int     getAgcTargetLevel();
    void    setAgcTargetLevel(int level);
    int     getAgcMaxGain();
    void    setAgcMaxGain(int gain);
    int     getAgcAttack();
    void    setAgcAttack(int attack);
    int     getAgcDecay();
    void    setAgcDecay(int decay);
    int     getAgcHang();
    void    setAgcHang(int hang);
    int     getAgcPanning();
    void    setAgcPanning(int panning);
    bool    getAgcPanningAuto();
    void    setAgcPanningAuto(bool panningAuto);

    void    setAmDcr(bool on);
    void    setAmSyncDcr(bool on);
    void    setAmSyncPllBw(float bw);
    void    setFmMaxdev(float max_hz);
    void    setFmEmph(double tau);
    void    setNoiseBlanker(int nbid, bool on, float threshold);

    void    setFreqLock(bool lock);
    bool    getFreqLock();

public slots:
    void setRxFreq(qint64 freq_hz);
    void setCurrentDemod(Modulations::idx demod);
    void setFilterOffset(qint64 freq_hz);
    void setSquelchLevel(double level);

private:
    void updateHwFreq();
    void updateDemodOptPage(Modulations::idx demod);
    unsigned int filterIdxFromLoHi(int lo, int hi) const;

    void modeOffShortcut();
    void modeRawShortcut();
    void modeAMShortcut();
    void modeNFMShortcut();
    void modeWFMmonoShortcut();
    void modeWFMstereoShortcut();
    void modeLSBShortcut();
    void modeUSBShortcut();
    void modeCWLShortcut();
    void modeCWUShortcut();
    void modeWFMoirtShortcut();
    void modeAMsyncShortcut();
    void filterNarrowShortcut();
    void filterNormalShortcut();
    void filterWideShortcut();
    void setAgcPresetFromParams(int decay);

signals:
    /** Signal emitted when receiver frequency has changed */
    void rxFreqChanged(qint64 freq_hz);

    /** Signal emitted when the channel filter frequency has changed. */
    void filterOffsetChanged(qint64 freq_hz);

    /** Signal emitted when new demodulator is selected. */
    void demodSelected(Modulations::idx demod);

    /** Signal emitted when new FM deviation is selected. */
    void fmMaxdevSelected(float max_dev);

    /** Signal emitted when new FM de-emphasis constant is selected. */
    void fmEmphSelected(double tau);

    /** Signal emitted when AM DCR status is toggled. */
    void amDcrToggled(bool enabled);

    /** Signal emitted when AM-Sync DCR status is toggled. */
    void amSyncDcrToggled(bool enabled);

    /** Signal emitted when new AM-Sync PLL BW is selected. */
    void amSyncPllBwSelected(float pll_bw);

    /** Signal emitted when baseband gain has changed. Gain is in dB. */
    //void bbGainChanged(float gain);

    /** Signal emitted when squelch level has changed. Level is in dBFS. */
    void sqlLevelChanged(double level);

    /**
     * Signal emitted when auto squelch level is clicked.
     *
     * @note Need current signal/noise level returned
     */
    double sqlAutoClicked(bool global);

    /** Signal emitted when squelch reset all popup menu item is clicked. */
    void sqlResetAllClicked();

    /** Signal emitted when AGC is togglen ON/OFF. */
    void agcToggled(bool agc_on);

    /** Signal emitted when AGC target level has changed. Level in dB. */
    void agcTargetLevelChanged(int value);

    /** Signal emitted when AGC maximum gain has changed. Gain is in dB.*/
    void agcMaxGainChanged(int gain);

    /** Signal emitted when AGC attack has changed. Decay is in millisec.*/
    void agcAttackChanged(int attack);

    /** Signal emitted when AGC decay has changed. Decay is in millisec.*/
    void agcDecayChanged(int decay);

    /** Signal emitted when AGC hang is changed. Hang is in millisec.*/
    void agcHangChanged(int hang);

    /** Signal emitted when AGC panning is changed. Panning is relative position -100...100 */
    void agcPanningChanged(int panning);

    /** Signal emitted when AGC panning auto mode is changed. */
    void agcPanningAuto(bool panningAuto);

    /** Signal emitted when noise blanker status has changed. */
    void noiseBlankerChanged(int nbid, bool on, float threshold);

    /** Signal emitted when freq lock mode changed. */
    void freqLock(bool lock, bool all);

    void cwOffsetChanged(int offset);

private slots:
    void on_freqSpinBox_valueChanged(double freq);
    void on_filterFreq_newFrequency(qint64 freq);
    void on_filterCombo_activated(int index);
    void on_modeSelector_activated(int index);
    void on_modeButton_clicked();
    void on_agcButton_clicked();
    void on_autoSquelchButton_clicked();
    void on_autoSquelchButton_customContextMenuRequested(const QPoint& pos);
    void menuSquelchAutoAll();
    void on_resetSquelchButton_clicked();
    void menuSquelchResetAll();
    //void on_agcPresetCombo_activated(int index);
    void on_agcPresetCombo_currentIndexChanged(int index);
    void on_sqlSpinBox_valueChanged(double value);
    void on_nb1Button_toggled(bool checked);
    void on_nb2Button_toggled(bool checked);
    void on_nbOptButton_clicked();
    void on_freqLockButton_clicked();
    void on_freqLockButton_customContextMenuRequested(const QPoint& pos);
    void menuFreqLockAll();
    void menuFreqUnlockAll();

    // Signals coming from noise blanker pop-up
    void nbOpt_thresholdChanged(int nbid, double value);

    // Signals coming from demod options pop-up
    void demodOpt_fmMaxdevSelected(float max_dev);
    void demodOpt_fmEmphSelected(double tau);
    void demodOpt_amDcrToggled(bool enabled);
    void demodOpt_cwOffsetChanged(int offset);
    void demodOpt_amSyncDcrToggled(bool enabled);
    void demodOpt_amSyncPllBwSelected(float pll_bw);

    // Signals coming from AGC options popup
    void agcOpt_maxGainChanged(int value);
    void agcOpt_targetLevelChanged(int value);
    void agcOpt_attackChanged(int value);
    void agcOpt_decayChanged(int value);
    void agcOpt_hangChanged(int value);
    void agcOpt_panningChanged(int value);
    void agcOpt_panningAutoChanged(bool value);

private:
    Ui::DockRxOpt *ui;        /** The Qt designer UI file. */
    CDemodOptions *demodOpt;  /** Demodulator options. */
    CAgcOptions   *agcOpt;    /** AGC options. */
    CNbOptions    *nbOpt;     /** Noise blanker options. */
    QMenu         *freqLockButtonMenu;
    QMenu         *squelchButtonMenu;

    bool agc_is_on;

    qint64 hw_freq_hz;   /** Current PLL frequency in Hz. */
};

#endif // DOCKRXOPT_H
