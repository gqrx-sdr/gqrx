/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Gqrx Contributors.
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
#ifndef DOCKTX_H
#define DOCKTX_H

#include <QDockWidget>
#include <QSettings>
#include <QGridLayout>
#include <QList>
#include <QLabel>
#include <QSlider>
#include <QString>

#include "qtgui/dockinputctl.h"  /* gain_t, gain_list_t */

namespace Ui {
    class DockTx;
}

/*! \brief Dock widget containing TX controls.
 *
 *  Provides:
 *  - PTT push-to-talk button (large, toggleable, keyboard shortcut F7)
 *  - Modulation mode selector (NFM / AM / USB / LSB)
 *  - TX hardware gain sliders (device-dependent, populated at runtime)
 *  - Microphone gain slider (-20 to +20 dB)
 *  - TX frequency offset (split mode) spin box
 */
class DockTx : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockTx(QWidget *parent = nullptr);
    ~DockTx();

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings) const;

    /** Populate TX hardware gain sliders from device capability list. */
    void setTxGainStages(gain_list_t &gains);

    /** Update PTT button visual state without emitting signals. */
    void setPttState(bool active);

    /** Show or hide the control area (hides when TX not supported by device). */
    void setTxEnabled(bool enabled);

    int  modulation() const;
    float micGain()   const;
    double txOffset() const;

signals:
    /** Emitted when the PTT button is pressed or released. */
    void pttToggled(bool active);

    /** Emitted when the modulation combo changes (value = tx_modulation enum). */
    void modulationChanged(int mod);

    /** Emitted when a TX hardware gain slider changes. */
    void txGainChanged(QString name, double value);

    /** Emitted when the microphone gain slider changes (dB). */
    void micGainChanged(float db);

    /** Emitted when the TX offset spin box changes. */
    void txOffsetChanged(double hz);

private slots:
    void on_pttButton_toggled(bool checked);
    void on_modCombo_currentIndexChanged(int index);
    void on_micGainSlider_valueChanged(int value);
    void on_txOffsetSpin_valueChanged(double value);
    void txGainSliderValueChanged(int value);

private:
    void clearTxGainWidgets();
    void updateMicLabel(int slider_value);

    Ui::DockTx        *ui;
    QGridLayout       *d_txGainLayout;   /*!< Dynamic layout for TX hardware gains. */
    QList<QSlider *>   d_txGainSliders;
    QList<QLabel *>    d_txGainLabels;
    QList<QLabel *>    d_txValueLabels;
    gain_list_t        d_gains;
};

#endif // DOCKTX_H

