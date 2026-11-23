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
    {"250.3 Hz", 250.3f}
};
static const int CTCSS_TABLE_LEN = sizeof(CTCSS_TABLE) / sizeof(CTCSS_TABLE[0]);

DockTxCtl::DockTxCtl(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockTxCtl),
    d_rx_freq_hz(144800000),
    d_updating(false)
{
    ui->setupUi(this);

    // Populate CTCSS tones
    for (int i = 0; i < CTCSS_TABLE_LEN; ++i) {
        ui->ctcssCombo->addItem(CTCSS_TABLE[i].label, QVariant(CTCSS_TABLE[i].freq));
    }

    ui->txFreqSpinBox->setValue((double)d_rx_freq_hz / 1.0e6);
    updatePttButton(false);
}

DockTxCtl::~DockTxCtl()
{
    delete ui;
}

void DockTxCtl::updatePttButton(bool active)
{
    ui->pttButton->setChecked(active);
    if (active) {
        ui->pttButton->setText(tr("TRANSMIT [TX]"));
        ui->pttButton->setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold; font-size: 13px;");
    } else {
        ui->pttButton->setText(tr("PTT [RX - Listening]"));
        ui->pttButton->setStyleSheet("");
        ui->pwrMeter->setValue(-100);
        ui->pwrMeterLabel->setText(tr("--- dBFS"));
    }
}

void DockTxCtl::setPtt(bool active)
{
    if (ui->pttButton->isChecked() != active) {
        updatePttButton(active);
    }
}

void DockTxCtl::setTxMode(int mode)
{
    d_updating = true;
    if (mode >= 0 && mode < ui->txModeCombo->count()) {
        ui->txModeCombo->setCurrentIndex(mode);
    }
    d_updating = false;
}

void DockTxCtl::setTxFreq(qint64 freq_hz)
{
    d_updating = true;
    ui->txFreqSpinBox->setValue((double)freq_hz / 1.0e6);
    d_updating = false;
}

void DockTxCtl::setTxSplit(bool enabled)
{
    d_updating = true;
    ui->splitCheckBox->setChecked(enabled);
    ui->txFreqSpinBox->setEnabled(enabled);
    d_updating = false;
}

void DockTxCtl::setMicGain(float gain_db)
{
    d_updating = true;
    int ival = qRound(gain_db);
    ui->micGainSlider->setValue(ival);
    ui->micGainLabel->setText(QString("%1 dB").arg(gain_db, 0, 'f', 1));
    d_updating = false;
}

void DockTxCtl::setTxPower(float power_frac)
{
    d_updating = true;
    int ival = qRound(power_frac * 100.0f);
    ui->rfPowerSlider->setValue(ival);
    ui->rfPowerLabel->setText(QString("%1 %").arg(ival));
    d_updating = false;
}

void DockTxCtl::setRxFreq(qint64 freq_hz)
{
    d_rx_freq_hz = freq_hz;
    if (!ui->splitCheckBox->isChecked()) {
        d_updating = true;
        ui->txFreqSpinBox->setValue((double)d_rx_freq_hz / 1.0e6);
        d_updating = false;
    }
}

void DockTxCtl::updateTxLevel(float level_dbfs)
{
    if (ui->pttButton->isChecked()) {
        int val = qBound(-100, (int)level_dbfs, 0);
        ui->pwrMeter->setValue(val);
        ui->pwrMeterLabel->setText(QString("%1 dBFS").arg(level_dbfs, 0, 'f', 1));
    }
}

void DockTxCtl::on_pttButton_clicked(bool checked)
{
    updatePttButton(checked);
    emit pttToggled(checked);
}

void DockTxCtl::on_txModeCombo_currentIndexChanged(int index)
{
    if (!d_updating) {
        emit txModeChanged(index);
    }
}

void DockTxCtl::on_splitCheckBox_toggled(bool checked)
{
    ui->txFreqSpinBox->setEnabled(checked);
    if (!checked) {
        ui->txFreqSpinBox->setValue((double)d_rx_freq_hz / 1.0e6);
    }
    if (!d_updating) {
        emit txSplitChanged(checked);
    }
}

void DockTxCtl::on_txFreqSpinBox_valueChanged(double val_mhz)
{
    if (!d_updating) {
        qint64 freq_hz = (qint64)(val_mhz * 1.0e6);
        emit txFreqChanged(freq_hz);
    }
}

void DockTxCtl::on_syncButton_clicked()
{
    d_updating = true;
    ui->txFreqSpinBox->setValue((double)d_rx_freq_hz / 1.0e6);
    d_updating = false;
    emit txFreqChanged(d_rx_freq_hz);
}

void DockTxCtl::on_micGainSlider_valueChanged(int value)
{
    ui->micGainLabel->setText(QString("%1 dB").arg((float)value, 0, 'f', 1));
    if (!d_updating) {
        emit micGainChanged((float)value);
    }
}

void DockTxCtl::on_rfPowerSlider_valueChanged(int value)
{
    ui->rfPowerLabel->setText(QString("%1 %").arg(value));
    if (!d_updating) {
        emit txPowerChanged((float)value / 100.0f);
    }
}

void DockTxCtl::on_ctcssCombo_currentIndexChanged(int index)
{
    if (!d_updating && index >= 0 && index < CTCSS_TABLE_LEN) {
        emit ctcssChanged(CTCSS_TABLE[index].freq);
    }
}

void DockTxCtl::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("tx");
    if (settings->contains("mode")) {
        setTxMode(settings->value("mode", 0).toInt());
    }
    if (settings->contains("split")) {
        setTxSplit(settings->value("split", false).toBool());
    }
    if (settings->contains("freq")) {
        setTxFreq(settings->value("freq", (qint64)144800000).toLongLong());
    }
    if (settings->contains("mic_gain")) {
        setMicGain(settings->value("mic_gain", 0.0f).toFloat());
    }
    if (settings->contains("rf_power")) {
        setTxPower(settings->value("rf_power", 0.5f).toFloat());
    }
    if (settings->contains("ctcss")) {
        float f = settings->value("ctcss", 0.0f).toFloat();
        for (int i = 0; i < CTCSS_TABLE_LEN; ++i) {
            if (std::abs(CTCSS_TABLE[i].freq - f) < 0.1f) {
                ui->ctcssCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    settings->endGroup();
}

void DockTxCtl::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("tx");
    settings->setValue("mode", ui->txModeCombo->currentIndex());
    settings->setValue("split", ui->splitCheckBox->isChecked());
    settings->setValue("freq", (qint64)(ui->txFreqSpinBox->value() * 1.0e6));
    settings->setValue("mic_gain", (float)ui->micGainSlider->value());
    settings->setValue("rf_power", (float)ui->rfPowerSlider->value() / 100.0f);
    int ctcss_idx = ui->ctcssCombo->currentIndex();
    if (ctcss_idx >= 0 && ctcss_idx < CTCSS_TABLE_LEN) {
        settings->setValue("ctcss", CTCSS_TABLE[ctcss_idx].freq);
    }
    settings->endGroup();
}
