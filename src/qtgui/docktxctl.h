/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024-2026 Gqrx Contributors.
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
#include "dsp/tx_modulator.h"

namespace Ui {
    class DockTxCtl;
}

/*! \brief Dedicated dock widget for transmitter controls.
 *  \ingroup UI
 *
 * Encapsulates the PTT button, TX mode selector, split frequency control,
 * CTCSS sub-tone selector, microphone gain, and RF power sliders.
 */
class DockTxCtl : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockTxCtl(QWidget *parent = nullptr);
    ~DockTxCtl() override;

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings);

public slots:
    void setPtt(bool active);
    void setTxMode(int mode);
    void setTxFreq(qint64 freq_hz);
    void setRxFreq(qint64 freq_hz);
    void setTxSplit(bool enabled);
    void setSplitOffset(double offset_hz);
    void setMicGain(float gain_db);
    void setTxPower(double power_db);
    void setCtcssFreq(float freq_hz);
    void updateTxLevel(float level_dbfs);

signals:
    void pttToggled(bool active);
    void txModeChanged(int mode);
    void txFreqChanged(qint64 freq_hz);
    void txSplitToggled(bool enabled);
    void splitOffsetChanged(double offset_hz);
    void micGainChanged(float gain_db);
    void txPowerChanged(double power_db);
    void ctcssChanged(float freq_hz);

private slots:
    void onPttButtonClicked();
    void onModeIndexChanged(int index);
    void onSplitCheckBoxToggled(bool checked);
    void onSplitOffsetSpinBoxChanged(double val);
    void onPowerSliderChanged(int value);
    void onMicGainSliderChanged(int value);
    void onCtcssIndexChanged(int index);

private:
    Ui::DockTxCtl *ui;
    bool d_ptt_active;
    qint64 d_rx_freq;
    qint64 d_tx_freq;
    double d_split_offset;
    bool d_split_enabled;
};

#endif // DOCKTXCTL_H
