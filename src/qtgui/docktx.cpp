/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Gqrx contributors.
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
#include <QDebug>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSettings>
#include <QSlider>

#include "qtgui/docktx.h"
#include "ui_docktx.h"

#define CONF_TX_PREFIX "tx/"

/* Standard EIA CTCSS tones (Hz). */
static const float ctcss_tones[] = {
    67.0, 69.3, 71.9, 74.4, 77.0, 79.7, 82.5, 85.4,
    88.5, 91.5, 94.8, 97.4, 100.0, 103.5, 107.2, 110.9,
    114.8, 118.8, 123.0, 127.3, 131.8, 136.5, 141.3, 146.2,
    151.4, 156.7, 159.8, 162.2, 165.5, 167.9, 171.3, 173.8,
    177.3, 179.9, 183.5, 186.2, 189.9, 192.8, 196.6, 199.5,
    203.5, 206.5, 210.7, 218.1, 225.7, 229.1, 233.6, 241.8,
    250.3
};

DockTx::DockTx(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockTx)
{
    ui->setupUi(this);

    /* CTCSS tones; index 0 is "None" */
    ui->ctcssCombo->addItem("None");
    for (float tone : ctcss_tones)
    {
        ui->ctcssCombo->addItem(QString::number(tone, 'f', 1) + " Hz");
    }
    ui->ctcssCombo->setCurrentIndex(0);
}

DockTx::~DockTx()
{
    delete ui;
}

void DockTx::readSettings(QSettings *settings)
{
    settings->beginGroup(CONF_TX_PREFIX);

    int mode = settings->value("mode", 0).toInt();
    if (mode < 0 || mode > 3)
        mode = 0;
    ui->modSelector->setCurrentIndex(mode);

    int power = settings->value("power", 100).toInt();
    if (power < 0 || power > 100)
        power = 100;
    ui->powerSlider->setValue(power);

    int mic = settings->value("mic_gain", 0).toInt();
    if (mic < -20 || mic > 20)
        mic = 0;
    ui->micSlider->setValue(mic);

    int ctcss = settings->value("ctcss", 0).toInt();
    if (ctcss < 0 || ctcss >= ui->ctcssCombo->count())
        ctcss = 0;
    ui->ctcssCombo->setCurrentIndex(ctcss);

    double offset = settings->value("offset_khz", 0.0).toDouble();
    ui->offsetSpinBox->setValue(offset);

    settings->endGroup();
}

void DockTx::saveSettings(QSettings *settings) const
{
    settings->beginGroup(CONF_TX_PREFIX);

    settings->setValue("mode", ui->modSelector->currentIndex());
    settings->setValue("power", ui->powerSlider->value());
    settings->setValue("mic_gain", ui->micSlider->value());
    settings->setValue("ctcss", ui->ctcssCombo->currentIndex());
    settings->setValue("offset_khz", ui->offsetSpinBox->value());

    settings->endGroup();
}

void DockTx::setPtt(bool ptt)
{
    ui->pttButton->setChecked(ptt);
}

bool DockTx::ptt(void) const
{
    return ui->pttButton->isChecked();
}

qint64 DockTx::offsetHz(void) const
{
    return (qint64)(ui->offsetSpinBox->value() * 1000.0);
}

int DockTx::modIndex(void) const
{
    return ui->modSelector->currentIndex();
}

double DockTx::power(void) const
{
    return ui->powerSlider->value() / 100.0;
}

float DockTx::micGain(void) const
{
    return (float)ui->micSlider->value();
}

float DockTx::ctcssFreq(void) const
{
    int index = ui->ctcssCombo->currentIndex();

    if (index <= 0)
        return 0.0f;

    return ctcss_tones[index - 1];
}

void DockTx::on_pttButton_toggled(bool checked)
{
    /* visual feedback: red while transmitting */
    ui->pttButton->setStyleSheet(
        checked ? "background-color: red; color: white;" : "");
    ui->pttButton->setText(checked ? "ON AIR" : "PTT");

    emit pttToggled(checked);
}

void DockTx::on_modSelector_currentIndexChanged(int index)
{
    /* CTCSS only applies to NFM */
    ui->ctcssCombo->setEnabled(index == 0);

    emit txModeChanged(index);
}

void DockTx::on_powerSlider_valueChanged(int value)
{
    ui->powerValue->setText(QString("%1 %").arg(value));
    emit rfPowerChanged(value / 100.0);
}

void DockTx::on_micSlider_valueChanged(int value)
{
    ui->micValue->setText(QString("%1 dB").arg(value));
    emit micGainChanged((float)value);
}

void DockTx::on_ctcssCombo_currentIndexChanged(int index)
{
    float freq = 0.0f;

    if (index > 0)
        freq = ctcss_tones[index - 1];

    emit ctcssChanged(freq);
}

void DockTx::on_offsetSpinBox_valueChanged(double value)
{
    emit txOffsetChanged((qint64)(value * 1000.0));
}
