/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2012-2013 Alexandru Csete OZ9AEC.
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
#include <QString>
#include "agc_options.h"
#include "ui_agc_options.h"

CAgcOptions::CAgcOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CAgcOptions)
{
    ui->setupUi(this);
}

CAgcOptions::~CAgcOptions()
{
    delete ui;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the dialog window using the
 * window close icon. We catch the event and hide the dialog but keep it
 * around for later use.
 */
void CAgcOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

/*! \brief Get current gain slider value. */
int CAgcOptions::gain()
{
    return ui->gainSlider->value();
}

/*! \brief Set AGC preset. */
void CAgcOptions::setPreset(agc_preset_e preset)
{
    switch (preset)
    {
    case AGC_FAST:
        setDecay(100);
        enableDecay(false);
        setSlope(0);
        enableSlope(false);
        enableGain(false);
        break;

    case AGC_MEDIUM:
        setDecay(500);
        enableDecay(false);
        setSlope(0);
        enableSlope(false);
        enableGain(false);
        break;

    case AGC_SLOW:
        setDecay(2000);
        enableDecay(false);
        setSlope(0);
        enableSlope(false);
        enableGain(false);
        break;

    case AGC_USER:
        enableDecay(true);
        enableSlope(true);
        enableGain(false);
        break;

    case AGC_OFF:
        enableGain(true);
        break;

    default:
        qDebug() << __func__ << "Invalid AGC preset" << preset;
        break;

    }
}

/*! \brief Set new gain slider value. */
void CAgcOptions::setGain(int value)
{
    ui->gainSlider->setValue(value);
    ui->gainLabel->setText(QString("%1 dB").arg(ui->gainSlider->value()));
}

/*! \brief Enable or disable gain slider.
 *  \param enabled Whether the slider should be enabled or not.
 *
 * The gain slider is enabled when AGC is OFF to provide manual gain
 * control. It is disabled when AGC is ON.
 */
void CAgcOptions::enableGain(bool enabled)
{
    ui->gainLabel->setEnabled(enabled);
    ui->gainSlider->setEnabled(enabled);
    ui->label1->setEnabled(enabled);
}

/*! \brief Get current AGC threshold. */
int CAgcOptions::threshold()
{
    return ui->thresholdSlider->value();
}

/*! \brief Set new AGC threshold. */
void CAgcOptions::setThreshold(int value)
{
    ui->thresholdSlider->setValue(value);
    ui->thresholdLabel->setText(QString("%1 dB").arg(ui->thresholdSlider->value()));
}

/*! \brief Get current AGC slope. */
int CAgcOptions::slope()
{
    return ui->slopeSlider->value();
}

/*! \brief Set new AGC slope. */
void CAgcOptions::setSlope(int value)
{
    ui->slopeSlider->setValue(value);
    ui->slopeLabel->setText(QString("%1 dB").arg(ui->slopeSlider->value()));
}

/*! \brief Enable or disable AGC slope slider.
 *  \param enabled Whether the slider should be enabled or not.
 *
 * The slope slider is enabled when AGC is in user mode.
 */
void CAgcOptions::enableSlope(bool enabled)
{
    ui->slopeSlider->setEnabled(enabled);
    ui->slopeLabel->setEnabled(enabled);
    ui->label3->setEnabled(enabled);
}

/*! \brief Get current decay value. */
int CAgcOptions::decay()
{
    return ui->decaySlider->value();
}

/*! \brief Set new decay value. */
void CAgcOptions::setDecay(int value)
{
    ui->decaySlider->setValue(value);
    ui->decayLabel->setText(QString("%1 ms").arg(ui->decaySlider->value()));
}

/*! \brief Enable or disable AGC decay slider.
 *  \param enabled Whether the slider should be enabled or not.
 *
 * The decay slider is enabled when AGC is in user mode.
 */
void CAgcOptions::enableDecay(bool enabled)
{
    ui->decaySlider->setEnabled(enabled);
    ui->decayLabel->setEnabled(enabled);
    ui->label4->setEnabled(enabled);
}

/*! \brief Get current state of AGC hang button. */
bool CAgcOptions::hang()
{
    return ui->hangButton->isChecked();
}

/*! \brief Set state og AGC hang button. */
void CAgcOptions::setHang(bool checked)
{
    ui->hangButton->setChecked(checked);
}



/*! \brief AGC gain slider value has changed. */
void CAgcOptions::on_gainSlider_valueChanged(int gain)
{
    ui->gainLabel->setText(QString("%1 dB").arg(ui->gainSlider->value()));
    emit gainChanged(gain);
}

/*! \brief AGC threshold slider value has changed. */
void CAgcOptions::on_thresholdSlider_valueChanged(int threshold)
{
    ui->thresholdLabel->setText(QString("%1 dB").arg(ui->thresholdSlider->value()));
    emit thresholdChanged(threshold);
}

/*! \brief AGC slope slider value has changed. */
void CAgcOptions::on_slopeSlider_valueChanged(int slope)
{
    ui->slopeLabel->setText(QString("%1 dB").arg(ui->slopeSlider->value()));
    emit slopeChanged(slope);
}

/*! \brief AGC decay slider value has changed. */
void CAgcOptions::on_decaySlider_valueChanged(int decay)
{
    ui->decayLabel->setText(QString("%1 ms").arg(ui->decaySlider->value()));
    emit decayChanged(decay);
}

/*! \brief AGC hang button has been toggled. */
void CAgcOptions::on_hangButton_toggled(bool checked)
{
    ui->hangButton->setText(checked ? tr("Enabled") : tr("Disabled"));
    emit hangChanged(checked);
}
