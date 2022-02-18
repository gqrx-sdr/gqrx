/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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

/*! \brief Get current max gain slider value. */
int CAgcOptions::maxGain()
{
    return ui->maxGainSlider->value();
}

/*! \brief Set AGC preset. */
void CAgcOptions::setPreset(agc_preset_e preset)
{
    switch (preset)
    {
    case AGC_FAST:
        setAttack(20);
        setDecay(100);
        setHang(0);
        enableAttack(false);
        enableDecay(false);
        enableHang(false);
        break;

    case AGC_MEDIUM:
        setAttack(50);
        setDecay(500);
        setHang(0);
        enableAttack(false);
        enableDecay(false);
        enableHang(false);
        break;

    case AGC_SLOW:
        setAttack(100);
        setDecay(2000);
        setHang(0);
        enableAttack(false);
        enableDecay(false);
        enableHang(false);
        break;

    case AGC_USER:
        enableAttack(true);
        enableDecay(true);
        enableHang(true);
        break;

    case AGC_OFF:
        break;

    default:
        qDebug() << __func__ << "Invalid AGC preset" << preset;
        break;

    }
}

/*! \brief Set new max gain slider value. */
void CAgcOptions::setMaxGain(int value)
{
    ui->maxGainSlider->setValue(value);
    ui->maxGainLabel->setText(QString("%1 dB").arg(ui->maxGainSlider->value()));
}

/*! \brief Get current AGC target level. */
int CAgcOptions::targetLevel()
{
    return ui->targetLevelSlider->value();
}

/*! \brief Set new AGC target level. */
void CAgcOptions::setTargetLevel(int value)
{
    ui->targetLevelSlider->setValue(value);
    ui->targetLevelLabel->setText(QString("%1 dB").arg(ui->targetLevelSlider->value()));
}


/*! \brief Get current attack value. */
int CAgcOptions::attack()
{
    return ui->attackSlider->value();
}

/*! \brief Set new attack value. */
void CAgcOptions::setAttack(int value)
{
    ui->attackSlider->setValue(value);
    ui->attackLabel->setText(QString("%1 ms").arg(ui->attackSlider->value()));
}

/*! \brief Enable or disable AGC attack slider.
 *  \param enabled Whether the slider should be enabled or not.
 *
 * The attack slider is enabled when AGC is in user mode.
 */
void CAgcOptions::enableAttack(bool enabled)
{
    ui->attackSlider->setEnabled(enabled);
    ui->attackLabel->setEnabled(enabled);
    ui->attackTitle->setEnabled(enabled);
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
    ui->decayTitle->setEnabled(enabled);
}

/*! \brief Get current hang value. */
int CAgcOptions::hang()
{
    return ui->hangSlider->value();
}

/*! \brief Set new hang value. */
void CAgcOptions::setHang(int value)
{
    ui->hangSlider->setValue(value);
    ui->hangLabel->setText(QString("%1 ms").arg(ui->hangSlider->value()));
}

/*! \brief Enable or disable AGC hang slider.
 *  \param enabled Whether the slider should be enabled or not.
 *
 * The hang slider is enabled when AGC is in user mode.
 */
void CAgcOptions::enableHang(bool enabled)
{
    ui->hangSlider->setEnabled(enabled);
    ui->hangLabel->setEnabled(enabled);
    ui->hangTitle->setEnabled(enabled);
}


/*! \brief AGC max gain slider value has changed. */
void CAgcOptions::on_maxGainSlider_valueChanged(int value)
{
    ui->maxGainLabel->setText(QString("%1 dB").arg(ui->maxGainSlider->value()));
    emit maxGainChanged(value);
}

/*! \brief AGC target level slider value has changed. */
void CAgcOptions::on_targetLevelSlider_valueChanged(int value)
{
    ui->targetLevelLabel->setText(QString("%1 dB").arg(ui->targetLevelSlider->value()));
    emit targetLevelChanged(value);
}

/*! \brief AGC attack slider value has changed. */
void CAgcOptions::on_attackSlider_valueChanged(int value)
{
    ui->attackLabel->setText(QString("%1 ms").arg(ui->attackSlider->value()));
    emit attackChanged(value);
}

/*! \brief AGC decay slider value has changed. */
void CAgcOptions::on_decaySlider_valueChanged(int value)
{
    ui->decayLabel->setText(QString("%1 ms").arg(ui->decaySlider->value()));
    emit decayChanged(value);
}

/*! \brief AGC hang slider value has changed. */
void CAgcOptions::on_hangSlider_valueChanged(int value)
{
    ui->hangLabel->setText(QString("%1 ms").arg(ui->hangSlider->value()));
    emit hangChanged(value);
}

