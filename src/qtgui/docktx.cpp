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
#include "dsp/tx_telemetry.h"
#include "dsp/tx_util.h"

DockTx::DockTx(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockTx)
{
    ui->setupUi(this);

    ui->ctcssCombo->addItem(tr("Off"), 0.0);
    for (float tone : ctcss_tones())
        ui->ctcssCombo->addItem(QString("%1 Hz").arg(tone, 0, 'f', 1), (double)tone);

    ui->dcsCombo->addItem(tr("Off"), 0);
    for (int code : dcs_codes())
    {
        QString name = QString("%1").arg(code, 3, 8, QChar('0'));
        ui->dcsCombo->addItem(name + " N", (code & 0x1FF) << 1);
        ui->dcsCombo->addItem(name + " I", ((code & 0x1FF) << 1) | 1);
    }
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

    double split = settings->value("tx/split_khz", 0.0).toDouble(&ok);
    if (ok)
        ui->splitSpin->setValue(split);

    float tone = settings->value("tx/ctcss", 0.0).toFloat(&ok);
    if (ok)
    {
        int idx = ui->ctcssCombo->findData((double)tone);
        ui->ctcssCombo->setCurrentIndex(idx > 0 ? idx : 0);
    }

    /* CTCSS and DCS are exclusive, CTCSS wins */
    int code = settings->value("tx/dcs", 0).toInt(&ok);
    if (ok && ctcss() == 0.0f)
    {
        bool inv = settings->value("tx/dcs_inv", false).toBool();
        int idx = ui->dcsCombo->findData(((code & 0x1FF) << 1) | (inv ? 1 : 0));
        ui->dcsCombo->setCurrentIndex(idx > 0 ? idx : 0);
    }
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

    if (ui->splitSpin->value() != 0.0)
        settings->setValue("tx/split_khz", ui->splitSpin->value());
    else
        settings->remove("tx/split_khz");

    if (ctcss() != 0.0f)
        settings->setValue("tx/ctcss", ctcss());
    else
        settings->remove("tx/ctcss");

    if (dcs() != 0)
    {
        settings->setValue("tx/dcs", dcs());
        settings->setValue("tx/dcs_inv", dcsInverted());
    }
    else
    {
        settings->remove("tx/dcs");
        settings->remove("tx/dcs_inv");
    }
}

int DockTx::txPower() const
{
    return ui->powerSlider->value();
}

double DockTx::txSplit() const
{
    return ui->splitSpin->value() * 1.0e3;
}

float DockTx::ctcss() const
{
    return ui->ctcssCombo->currentData().toFloat();
}

/*! \brief The selected DCS code (9-bit value), 0 if off. */
int DockTx::dcs() const
{
    return ui->dcsCombo->currentData().toInt() >> 1;
}

bool DockTx::dcsInverted() const
{
    return ui->dcsCombo->currentData().toInt() & 1;
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

/*! \brief Show what the device reports while transmitting, "–" otherwise.
 *
 * Forward power, ALC and SWR are "–" unless the backend exposes them.
 */
void DockTx::setTelemetry(bool valid, double rate, double freq, double gain,
                          double gain_min, double gain_max,
                          std::optional<double> fwd_power,
                          std::optional<double> alc,
                          std::optional<double> swr)
{
    if (!valid)
    {
        fwd_power = alc = swr = std::nullopt;
    }

    ui->rateValueLabel->setText(QString::fromStdString(tx_telemetry_rate_text(valid, rate)));
    ui->freqValueLabel->setText(QString::fromStdString(tx_telemetry_freq_text(valid, freq)));
    ui->gainValueLabel->setText(QString::fromStdString(
        tx_telemetry_gain_text(valid, gain, gain_min, gain_max)));
    ui->fwdValueLabel->setText(QString::fromStdString(tx_telemetry_fwd_text(fwd_power)));
    ui->alcValueLabel->setText(QString::fromStdString(tx_telemetry_alc_text(alc)));
    ui->swrValueLabel->setText(QString::fromStdString(tx_telemetry_swr_text(swr)));
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

void DockTx::on_splitSpin_valueChanged(double value)
{
    emit txSplitChanged(value * 1.0e3);
}

void DockTx::on_ctcssCombo_currentIndexChanged(int)
{
    if (ctcss() != 0.0f && dcs() != 0)
    {
        {
            const QSignalBlocker blocker(ui->dcsCombo);
            ui->dcsCombo->setCurrentIndex(0);
        }
        emit dcsChanged(0, false);
    }
    emit ctcssChanged(ctcss());
}

void DockTx::on_dcsCombo_currentIndexChanged(int)
{
    if (dcs() != 0 && ctcss() != 0.0f)
    {
        {
            const QSignalBlocker blocker(ui->ctcssCombo);
            ui->ctcssCombo->setCurrentIndex(0);
        }
        emit ctcssChanged(0.0f);
    }
    emit dcsChanged(dcs(), dcsInverted());
}
