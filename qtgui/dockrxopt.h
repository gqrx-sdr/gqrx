/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#include "qtgui/demod-options.h"


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

    /*! \brief Mode selector entries. */
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
        MODE_CWU        = 9  /*!< CW using USB filter. */
    };

    explicit DockRxOpt(qint64 filterOffsetRange = 90000, QWidget *parent = 0);
    ~DockRxOpt();

    void setFilterOffset(qint64 freq_hz);
    void setFilterOffsetRange(qint64 range_hz);

    void setFilterParam(int lo, int hi);
    void setCurrentFilter(int index);
    int  currentFilter();

    void setRfFreq(qint64 freq_hz);

    void setCurrentDemod(int demod);
    int  currentDemod();

    float currentMaxdev();

private:
    void updateRxFreq();

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
    void bbGainChanged(float gain);

    /*! \brief Signal emitted when squelch level has changed. Level is in dBFS. */
    void sqlLevelChanged(double level);

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


private slots:
    void on_filterFreq_NewFrequency(qint64 freq);
    void on_filterCombo_activated(int index);
    void on_filterButton_clicked();
    void on_modeSelector_activated(int index);
    void on_modeButton_clicked();
    void on_agcPresetCombo_activated(int index);
    void on_agcHangButton_toggled(bool checked);
    void on_agcGainDial_valueChanged(int value);
    void on_agcThresholdDial_valueChanged(int value);
    void on_agcSlopeDial_valueChanged(int value);
    void on_agcDecayDial_valueChanged(int value);
    void on_sqlSlider_valueChanged(int value);
    void on_nb1Button_toggled(bool checked);
    void on_nb2Button_toggled(bool checked);
    void on_nb1Threshold_valueChanged(double value);
    void on_nb2Threshold_valueChanged(double value);

    /* Signals coming from demod options pop-up */
    void demodOpt_fmMaxdevSelected(float max_dev);
    void demodOpt_fmEmphSelected(double tau);

private:
    Ui::DockRxOpt *ui;        /*! The Qt designer UI file. */
    CDemodOptions *demodOpt;  /*! Demodulator options. */

    bool agc_is_on;

    qint64 rf_freq_hz;   /*! Current RF frequency in Hz. Used to display RX frequency. */
};

#endif // DOCKRXOPT_H
