/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx Contributors.
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
#ifndef DOCKTXCTL_H
#define DOCKTXCTL_H

#include <QDockWidget>
#include <QSettings>

namespace Ui {
    class DockTxCtl;
}

/*! \brief Dock widget for transmitter controls.
 *  \ingroup UI
 *
 * Encapsulates the PTT button, TX mode selector, split frequency control,
 * CTCSS sub-tone selector, microphone gain, and RF power sliders.
 */
class DockTxCtl : public QDockWidget
{
    Q_OBJECT

public:
    enum tx_mode_idx {
        MODE_NFM = 0,
        MODE_AM  = 1,
        MODE_USB = 2,
        MODE_LSB = 3,
        MODE_CW  = 4
    };

    explicit DockTxCtl(QWidget *parent = 0);
    ~DockTxCtl();

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

public slots:
    void setPtt(bool active);
    void setTxMode(int mode);
    void setTxFreq(qint64 freq_hz);
    void setTxSplit(bool enabled);
    void setMicGain(float gain_db);
    void setTxPower(float power_frac);
    void setRxFreq(qint64 freq_hz);
    void updateTxLevel(float level_dbfs);

signals:
    void pttToggled(bool active);
    void txModeChanged(int mode);
    void txFreqChanged(qint64 freq_hz);
    void txSplitChanged(bool enabled);
    void micGainChanged(float gain_db);
    void txPowerChanged(float power_frac);
    void ctcssChanged(float freq_hz);

private slots:
    void on_pttButton_clicked(bool checked);
    void on_txModeCombo_currentIndexChanged(int index);
    void on_splitCheckBox_toggled(bool checked);
    void on_txFreqSpinBox_valueChanged(double val_mhz);
    void on_syncButton_clicked();
    void on_micGainSlider_valueChanged(int value);
    void on_rfPowerSlider_valueChanged(int value);
    void on_ctcssCombo_currentIndexChanged(int index);

private:
    Ui::DockTxCtl *ui;
    qint64 d_rx_freq_hz;
    bool d_updating;

    void updatePttButton(bool active);
};

#endif // DOCKTXCTL_H
