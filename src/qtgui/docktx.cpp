/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QSignalBlocker>
#include "docktx.h"
#include "ui_docktx.h"

DockTx::DockTx(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockTx)
{
    ui->setupUi(this);
}

DockTx::~DockTx()
{
    delete ui;
}

/*! \brief Read the slider positions from the settings. PTT is never restored. */
void DockTx::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    bool ok;
    int  value;

    value = settings->value("tx/power", 0).toInt(&ok);
    if (ok)
        ui->powerSlider->setValue(value);

    value = settings->value("tx/mic_gain", 0).toInt(&ok);
    if (ok)
        ui->micSlider->setValue(value);
}

void DockTx::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    if (ui->powerSlider->value() != 0)
        settings->setValue("tx/power", ui->powerSlider->value());
    else
        settings->remove("tx/power");

    if (ui->micSlider->value() != 0)
        settings->setValue("tx/mic_gain", ui->micSlider->value());
    else
        settings->remove("tx/mic_gain");
}

int DockTx::txPower() const
{
    return ui->powerSlider->value();
}

int DockTx::micGain() const
{
    return ui->micSlider->value();
}

/*! \brief Update the PTT button without emitting txToggled(). */
void DockTx::setTransmitting(bool transmitting)
{
    {
        const QSignalBlocker blocker(ui->pttButton);
        ui->pttButton->setChecked(transmitting);
    }

    ui->pttButton->setStyleSheet(transmitting ? "background-color: red; color: white;"
                                              : "");
    ui->statusLabel->setText(transmitting ? tr("TX") : tr("RX"));
}

/*! \brief Show a status or error message. */
void DockTx::setStatus(const QString &status)
{
    ui->statusLabel->setText(status);
}

void DockTx::on_pttButton_toggled(bool checked)
{
    emit txToggled(checked);
}

void DockTx::on_powerSlider_valueChanged(int value)
{
    ui->powerValueLabel->setText(QString("%1 %").arg(value));
    emit txPowerChanged(value);
}

void DockTx::on_micSlider_valueChanged(int value)
{
    ui->micValueLabel->setText(QString("%1 dB").arg(value));
    emit micGainChanged((float)value);
}
