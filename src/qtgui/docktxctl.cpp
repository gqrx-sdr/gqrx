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
#include "qtgui/docktxctl.h"
#include "ui_docktxctl.h"

static const struct {
    const char *label;
    float freq;
} CTCSS_TABLE[] = {
    {"None", 0.0f},
    {"67.0 Hz", 67.0f},
    {"69.3 Hz", 69.3f},
    {"71.9 Hz", 71.9f},
    {"74.4 Hz", 74.4f},
    {"77.0 Hz", 77.0f},
    {"79.7 Hz", 79.7f},
    {"82.5 Hz", 82.5f},
    {"85.4 Hz", 85.4f},
    {"88.5 Hz", 88.5f},
    {"91.5 Hz", 91.5f},
    {"94.8 Hz", 94.8f},
    {"97.4 Hz", 97.4f},
    {"100.0 Hz", 100.0f},
    {"103.5 Hz", 103.5f},
    {"107.2 Hz", 107.2f},
    {"110.9 Hz", 110.9f},
    {"114.8 Hz", 114.8f},
    {"118.8 Hz", 118.8f},
    {"123.0 Hz", 123.0f},
    {"127.3 Hz", 127.3f},
    {"131.8 Hz", 131.8f},
    {"136.5 Hz", 136.5f},
    {"141.3 Hz", 141.3f},
    {"146.2 Hz", 146.2f},
    {"151.4 Hz", 151.4f},
    {"156.7 Hz", 156.7f},
    {"162.2 Hz", 162.2f},
    {"167.9 Hz", 167.9f},
    {"173.8 Hz", 173.8f},
    {"179.9 Hz", 179.9f},
    {"186.2 Hz", 186.2f},
    {"192.8 Hz", 192.8f},
    {"203.5 Hz", 203.5f},
    {"210.7 Hz", 210.7f},
    {"218.1 Hz", 218.1f},
    {"225.7 Hz", 225.7f},
    {"233.6 Hz", 233.6f},
    {"241.8 Hz", 241.8f},
    {"250.3 Hz", 250.3f},
    {"254.1 Hz", 254.1f}
};

static const int CTCSS_TABLE_LEN = sizeof(CTCSS_TABLE) / sizeof(CTCSS_TABLE[0]);

DockTxCtl::DockTxCtl(QWidget *parent)
    : QDockWidget(parent),
      ui(new Ui::DockTxCtl),
      d_ptt_active(false),
      d_rx_freq(145500000),
      d_tx_freq(145500000),
      d_split_offset(0.0),
      d_split_enabled(false)
{
    ui->setupUi(this);

    // Populate CTCSS combo
    for (int i = 0; i < CTCSS_TABLE_LEN; ++i) {
        ui->comboCtcss->addItem(QString(CTCSS_TABLE[i].label), QVariant(CTCSS_TABLE[i].freq));
    }

    // Connect signals/slots
    connect(ui->btnPtt, SIGNAL(clicked()), this, SLOT(onPttButtonClicked()));
    connect(ui->comboMode, SIGNAL(currentIndexChanged(int)), this, SLOT(onModeIndexChanged(int)));
    connect(ui->checkSplit, SIGNAL(toggled(bool)), this, SLOT(onSplitCheckBoxToggled(bool)));
    connect(ui->spinSplitOffset, SIGNAL(valueChanged(double)), this, SLOT(onSplitOffsetSpinBoxChanged(double)));
    connect(ui->sliderPower, SIGNAL(valueChanged(int)), this, SLOT(onPowerSliderChanged(int)));
    connect(ui->sliderMicGain, SIGNAL(valueChanged(int)), this, SLOT(onMicGainSliderChanged(int)));
    connect(ui->comboCtcss, SIGNAL(currentIndexChanged(int)), this, SLOT(onCtcssIndexChanged(int)));

    setPtt(false);
}

DockTxCtl::~DockTxCtl()
{
    delete ui;
}

void DockTxCtl::setPtt(bool active)
{
    d_ptt_active = active;
    ui->btnPtt->setChecked(active);
    if (active) {
        ui->btnPtt->setText("TRANSMITTING [PTT]");
        ui->btnPtt->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; font-weight: bold; border-radius: 4px; padding: 6px; }");
        ui->lblTxStatus->setText("<font color='#d32f2f'><b>TX ACTIVE</b></font>");
    } else {
        ui->btnPtt->setText("PUSH TO TALK (PTT)");
        ui->btnPtt->setStyleSheet("QPushButton { background-color: #388e3c; color: white; font-weight: bold; border-radius: 4px; padding: 6px; }");
        ui->lblTxStatus->setText("<font color='#388e3c'><b>STANDBY</b></font>");
    }
}

void DockTxCtl::onPttButtonClicked()
{
    bool new_state = !d_ptt_active;
    setPtt(new_state);
    emit pttToggled(new_state);
}

void DockTxCtl::onModeIndexChanged(int index)
{
    emit txModeChanged(index);
}

void DockTxCtl::setTxMode(int mode)
{
    if (mode >= 0 && mode < ui->comboMode->count()) {
        ui->comboMode->setCurrentIndex(mode);
    }
}

void DockTxCtl::onSplitCheckBoxToggled(bool checked)
{
    d_split_enabled = checked;
    ui->spinSplitOffset->setEnabled(checked);
    emit txSplitToggled(checked);
}

void DockTxCtl::setTxSplit(bool enabled)
{
    d_split_enabled = enabled;
    ui->checkSplit->setChecked(enabled);
    ui->spinSplitOffset->setEnabled(enabled);
}

void DockTxCtl::onSplitOffsetSpinBoxChanged(double val)
{
    d_split_offset = val * 1000.0; // kHz to Hz
    emit splitOffsetChanged(d_split_offset);
}

void DockTxCtl::setSplitOffset(double offset_hz)
{
    d_split_offset = offset_hz;
    ui->spinSplitOffset->setValue(offset_hz / 1000.0);
}

void DockTxCtl::setRxFreq(qint64 freq_hz)
{
    d_rx_freq = freq_hz;
    if (!d_split_enabled) {
        d_tx_freq = freq_hz;
        ui->lblTxFreq->setText(QString("%1 MHz").arg(freq_hz / 1e6, 0, 'f', 4));
    }
}

void DockTxCtl::setTxFreq(qint64 freq_hz)
{
    d_tx_freq = freq_hz;
    ui->lblTxFreq->setText(QString("%1 MHz").arg(freq_hz / 1e6, 0, 'f', 4));
    emit txFreqChanged(freq_hz);
}

void DockTxCtl::onPowerSliderChanged(int value)
{
    double power_pct = static_cast<double>(value);
    ui->lblPowerVal->setText(QString("%1 %").arg(value));
    emit txPowerChanged(power_pct);
}

void DockTxCtl::setTxPower(double power_db)
{
    ui->sliderPower->setValue(static_cast<int>(power_db));
    ui->lblPowerVal->setText(QString("%1 %").arg(static_cast<int>(power_db)));
}

void DockTxCtl::onMicGainSliderChanged(int value)
{
    float gain_db = static_cast<float>(value);
    ui->lblMicGainVal->setText(QString("%1 dB").arg(value));
    emit micGainChanged(gain_db);
}

void DockTxCtl::setMicGain(float gain_db)
{
    ui->sliderMicGain->setValue(static_cast<int>(gain_db));
    ui->lblMicGainVal->setText(QString("%1 dB").arg(static_cast<int>(gain_db)));
}

void DockTxCtl::onCtcssIndexChanged(int index)
{
    if (index >= 0 && index < CTCSS_TABLE_LEN) {
        emit ctcssChanged(CTCSS_TABLE[index].freq);
    }
}

void DockTxCtl::setCtcssFreq(float freq_hz)
{
    for (int i = 0; i < CTCSS_TABLE_LEN; ++i) {
        if (std::abs(CTCSS_TABLE[i].freq - freq_hz) < 0.1f) {
            ui->comboCtcss->setCurrentIndex(i);
            break;
        }
    }
}

void DockTxCtl::updateTxLevel(float level_dbfs)
{
    if (d_ptt_active) {
        ui->barTxLevel->setValue(static_cast<int>(std::max(0.0f, level_dbfs + 100.0f)));
    } else {
        ui->barTxLevel->setValue(0);
    }
}

void DockTxCtl::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("transmitter");
    setTxMode(settings->value("mode", 2).toInt()); // default NFM
    setTxPower(settings->value("power", 50.0).toDouble());
    setMicGain(settings->value("mic_gain", 0.0).toFloat());
    setTxSplit(settings->value("split", false).toBool());
    setSplitOffset(settings->value("split_offset", 600000.0).toDouble()); // default +600 kHz
    setCtcssFreq(settings->value("ctcss", 0.0).toFloat());
    settings->endGroup();
}

void DockTxCtl::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("transmitter");
    settings->setValue("mode", ui->comboMode->currentIndex());
    settings->setValue("power", ui->sliderPower->value());
    settings->setValue("mic_gain", ui->sliderMicGain->value());
    settings->setValue("split", d_split_enabled);
    settings->setValue("split_offset", d_split_offset);
    settings->setValue("ctcss", ui->comboCtcss->currentData().toFloat());
    settings->endGroup();
}
