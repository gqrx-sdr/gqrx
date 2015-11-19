/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
 * Copyright 2015 Timothy Reaves
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


namespace Ui {
    class DockRxOpt;
}


/*! \brief Dock window with receiver options.
 *  \ingroup UI
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

    /*! \brief Mode selector entries.
     *  \note If you change this enum, remember to update the TCP interface.
     *  \note Keep in same order as the Strings in ModulationStrings, see DockRxOpt.cpp constructor.
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
        MODE_WFM_STEREO_OIRT = 10 /*!< Broadcast FM (stereo oirt). */

    };

    explicit DockRxOpt(qint64 filterOffsetRange = 90000, QWidget *parent = 0);
    ~DockRxOpt();

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

    void setFilterOffsetRange(qint64 range_hz);

    void setFilterParam(int lo, int hi);
    void setCurrentFilter(int index);
    int  currentFilter();

    void setCurrentFilterShape(int index);
    int  currentFilterShape();

    void setHwFreq(qint64 freq_hz);

    int  currentDemod();
    QString currentDemodAsString();

    float currentMaxdev();

    static QStringList ModulationStrings;
    static QString GetStringForModulationIndex(int iModulationIndex);
    static int GetEnumForModulationString(QString param);
    static bool IsModulationValid(QString strModulation);

public slots:
    void setCurrentDemod(int demod);
    void setFilterOffset(qint64 freq_hz);

private:
    void updateHwFreq();
    void addDigitToFrequency(const QString digit);

signals:
    /*! \brief Signal emitted when the channel filter frequency has changed. */
    void filterOffsetChanged(qint64 freq_hz);

    /*! \brief Signal emitted when new demodulator is selected. */
    void demodSelected(int demod);

    /*! \brief Signal emitted when new FM deviation is selected. */
    void fmMaxdevSelected(float max_dev);

    /*! \brief Signal emitted when new FM de-emphasis constant is selected. */
    void fmEmphSelected(double tau);

    /*! \brief Signal emitted when AM DCR status is toggled. */
    void amDcrToggled(bool enabled);

    /*! \brief Signal emitted when baseband gain has changed. Gain is in dB. */
    //void bbGainChanged(float gain);

    /*! \brief Signal emitted when squelch level has changed. Level is in dBFS. */
    void sqlLevelChanged(double level);

    /*! \brief Signal emitted when auto squelch level is clicked.
     *
     * \note Need current signal/noise level returned
     */
    double sqlAutoClicked();

    /*! \brief Signal emitted when AGC is togglen ON/OFF. */
    void agcToggled(bool agc_on);

    /*! \brief Signal emitted when AGC hang is toggled. */
    void agcHangToggled(bool use_hang);

    /*! \brief Signal emitted when AGC threshold has changed. Threshold in dB. */
    void agcThresholdChanged(int value);

    /*! \brief Signal emitted when AGC slope has changed. Slope is in dB.*/
    void agcSlopeChanged(int slope);

    /*! \brief Signal emitted when AGC decay has changed. Decay is in millisec.*/
    void agcDecayChanged(int decay);

    /*! \brief Signal emitted when AGC manual gain has changed. Gain is in dB.*/
    void agcGainChanged(int gain);

    /*! \brief Signal emitted when noise blanker status has changed. */
    void noiseBlankerChanged(int nbid, bool on, float threshold);

    /*! \brief Signal emitted when a direct frequency - in hertx - has been entered. */
    void frequencySelected(const qint64 newFrequency);

private slots:
    void on_filterFreq_newFrequency(qint64 freq);
    void on_filterCombo_activated(int index);
    void on_modeSelector_activated(int index);
    void on_modeButton_clicked();
    void on_agcButton_clicked();
    void on_autoSquelchButton_clicked();
    void on_agcPresetCombo_activated(int index);
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

    // Signals coming from AGC options popup
    void agcOpt_hangToggled(bool checked);
    void agcOpt_gainChanged(int value);
    void agcOpt_thresholdChanged(int value);
    void agcOpt_slopeChanged(int value);
    void agcOpt_decayChanged(int value);

    // The direct band  & numeric buttons for direct frequency tuning.
    void on_button160m_clicked(void);
    void on_button80m_clicked(void);
    void on_button60m_clicked(void);
    void on_button40m_clicked(void);
    void on_button30m_clicked(void);
    void on_button20m_clicked(void);
    void on_button17m_clicked(void);
    void on_button15m_clicked(void);
    void on_button12m_clicked(void);
    void on_button10m_clicked(void);
    void on_button6m_clicked(void);
    void on_button2m_clicked(void);
    void on_button0_clicked(void);
    void on_button1_clicked(void);
    void on_button2_clicked(void);
    void on_button3_clicked(void);
    void on_button4_clicked(void);
    void on_button5_clicked(void);
    void on_button6_clicked(void);
    void on_button7_clicked(void);
    void on_button8_clicked(void);
    void on_button9_clicked(void);
    void on_buttonDot_clicked(void);
    void on_buttonDelete_clicked(void);
    void on_buttonGo_clicked(void);
    void on_buttonHz_clicked(void);
    void on_buttonKHz_clicked(void);
    void on_buttonMHz_clicked(void);

private:
    Ui::DockRxOpt *ui;        /*! The Qt designer UI file. */
    CDemodOptions *demodOpt;  /*! Demodulator options. */
    CAgcOptions   *agcOpt;    /*! AGC options. */
    CNbOptions    *nbOpt;     /*! Noise blanker options. */

    bool agc_is_on;

    qint64 hw_freq_hz;   /*! Current PLL frequency in Hz. */
    qint64 frequencyMultiplier; /*! The multiplier for manually entered frequency values. */
};

#endif // DOCKRXOPT_H
