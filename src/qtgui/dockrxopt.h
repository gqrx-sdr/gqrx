/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#include "qtgui/agc_options.h"
#include "qtgui/demod_options.h"
#include "qtgui/nb_options.h"

#define FILTER_PRESET_WIDE      0
#define FILTER_PRESET_NORMAL    1
#define FILTER_PRESET_NARROW    2
#define FILTER_PRESET_USER      3

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

    /**
     * Mode selector entries.
     *
     * @note If you change this enum, remember to update the TCP interface.
     * @note Keep in same order as the Strings in ModulationStrings, see
     *       DockRxOpt.cpp constructor.
     */
    enum rxopt_mode_idx {
        MODE_OFF        = 0, /*!< Demodulator completely off. */
        MODE_RAW        = 1, /*!< Raw I/Q passthrough. */
        MODE_AM         = 2, /*!< Amplitude modulation. */
        MODE_NFM        = 3, /*!< Narrow band FM. */
        MODE_WFM_MONO   = 4, /*!< Broadcast FM (mono). */
        MODE_WFM_STEREO = 5, /*!< Broadcast FM (stereo). */
        MODE_LSB        = 6, /*!< Lower side band. */
        MODE_USB        = 7, /*!< Upper side band. */
        MODE_CWL        = 8, /*!< CW using LSB filter. */
        MODE_CWU        = 9, /*!< CW using USB filter. */
        MODE_WFM_STEREO_OIRT = 10, /*!< Broadcast FM (stereo oirt). */
        MODE_LAST       = 11
    };

    explicit DockRxOpt(qint64 filterOffsetRange = 90000, QWidget *parent = 0);
    ~DockRxOpt();

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

    void setFilterOffsetRange(qint64 range_hz);

    void setFilterParam(int lo, int hi);
    void setCurrentFilter(int index);
    int  currentFilter() const;

    void setCurrentFilterShape(int index);
    int  currentFilterShape() const;

    void setHwFreq(qint64 freq_hz);
    void setRxFreqRange(qint64 min_hz, qint64 max_hz);

    int  currentDemod() const;
    QString currentDemodAsString();

    float currentMaxdev() const;
    double currentEmph() const;
    double currentSquelchLevel() const;

    void    getFilterPreset(int mode, int preset, int * lo, int * hi) const;
    int     getCwOffset() const;

    double  getSqlLevel(void) const;

    static QStringList ModulationStrings;
    static QString GetStringForModulationIndex(int iModulationIndex);
    static int GetEnumForModulationString(QString param);
    static bool IsModulationValid(QString strModulation);

public slots:
    void setRxFreq(qint64 freq_hz);
    void setCurrentDemod(int demod);
    void setFilterOffset(qint64 freq_hz);
    void setSquelchLevel(double level);

private:
    void updateHwFreq();
    void updateDemodOptPage(int demod);
    unsigned int filterIdxFromLoHi(int lo, int hi) const;

signals:
    /** Signal emitted when receiver frequency has changed */
    void rxFreqChanged(qint64 freq_hz);

    /** Signal emitted when the channel filter frequency has changed. */
    void filterOffsetChanged(qint64 freq_hz);

    /** Signal emitted when new demodulator is selected. */
    void demodSelected(int demod);

    /** Signal emitted when new FM deviation is selected. */
    void fmMaxdevSelected(float max_dev);

    /** Signal emitted when new FM de-emphasis constant is selected. */
    void fmEmphSelected(double tau);

    /** Signal emitted when AM DCR status is toggled. */
    void amDcrToggled(bool enabled);

    /** Signal emitted when baseband gain has changed. Gain is in dB. */
    //void bbGainChanged(float gain);

    /** Signal emitted when squelch level has changed. Level is in dBFS. */
    void sqlLevelChanged(double level);

    /**
     * Signal emitted when auto squelch level is clicked.
     *
     * @note Need current signal/noise level returned
     */
    double sqlAutoClicked();

    /** Signal emitted when AGC is togglen ON/OFF. */
    void agcToggled(bool agc_on);

    /** Signal emitted when AGC hang is toggled. */
    void agcHangToggled(bool use_hang);

    /** Signal emitted when AGC threshold has changed. Threshold in dB. */
    void agcThresholdChanged(int value);

    /** Signal emitted when AGC slope has changed. Slope is in dB.*/
    void agcSlopeChanged(int slope);

    /** Signal emitted when AGC decay has changed. Decay is in millisec.*/
    void agcDecayChanged(int decay);

    /** Signal emitted when AGC manual gain has changed. Gain is in dB.*/
    void agcGainChanged(int gain);

    /** Signal emitted when noise blanker status has changed. */
    void noiseBlankerChanged(int nbid, bool on, float threshold);

    void cwOffsetChanged(int offset);

private slots:
    void on_freqSpinBox_valueChanged(double freq);
    void on_filterFreq_newFrequency(qint64 freq);
    void on_filterCombo_activated(int index);
    void on_modeSelector_activated(int index);
    void on_modeButton_clicked();
    void on_agcButton_clicked();
    void on_autoSquelchButton_clicked();
    void on_resetSquelchButton_clicked();
    //void on_agcPresetCombo_activated(int index);
    void on_agcPresetCombo_currentIndexChanged(int index);
    void on_sqlSpinBox_valueChanged(double value);
    void on_nb1Button_toggled(bool checked);
    void on_nb2Button_toggled(bool checked);
    void on_nbOptButton_clicked();

    // Signals coming from noise blanker pop-up
    void nbOpt_thresholdChanged(int nbid, double value);

    // Signals coming from demod options pop-up
    void demodOpt_fmMaxdevSelected(float max_dev);
    void demodOpt_fmEmphSelected(double tau);
    void demodOpt_amDcrToggled(bool enabled);
    void demodOpt_cwOffsetChanged(int offset);

    // Signals coming from AGC options popup
    void agcOpt_hangToggled(bool checked);
    void agcOpt_gainChanged(int value);
    void agcOpt_thresholdChanged(int value);
    void agcOpt_slopeChanged(int value);
    void agcOpt_decayChanged(int value);

private:
    Ui::DockRxOpt *ui;        /** The Qt designer UI file. */
    CDemodOptions *demodOpt;  /** Demodulator options. */
    CAgcOptions   *agcOpt;    /** AGC options. */
    CNbOptions    *nbOpt;     /** Noise blanker options. */

    bool agc_is_on;

    qint64 hw_freq_hz;   /** Current PLL frequency in Hz. */
};

#endif // DOCKRXOPT_H
