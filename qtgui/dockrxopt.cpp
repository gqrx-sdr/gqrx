/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#include "dockrxopt.h"
#include "ui_dockrxopt.h"

DockRxOpt::DockRxOpt(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockRxOpt),
    agc_is_on(true),
    rf_freq_hz(144500000)
{
    ui->setupUi(this);

    /** FIXME: BW should be parameter */
    ui->filterFreq->Setup(7, -45000, 45000, 1, UNITS_KHZ);
    ui->filterFreq->SetFrequency(0);
}

DockRxOpt::~DockRxOpt()
{
    delete ui;
}

/*! \brief Set value of channel filter offset selector.
 *  \param freq_hz The frequency in Hz
 */
void DockRxOpt::setFilterOffset(qint64 freq_hz)
{
    ui->filterFreq->SetFrequency(freq_hz);
    updateRxFreq();
}


/*! \brief Set new RF frequency
 *  \param freq_hz The frequency in Hz
 *
 * RF frequency is the frequency to which the device device is tuned to
 * The actual RX frequency is the sum of the RF frequency and the filter
 * offset.
 */
void DockRxOpt::setRfFreq(qint64 freq_hz)
{
    rf_freq_hz = freq_hz;
    updateRxFreq();
}


/*! \brief Update RX frequency label. */
void DockRxOpt::updateRxFreq()
{
    double rx_freq_mhz = (rf_freq_hz + ui->filterFreq->GetFrequency()) / 1.0e6;
    ui->rxFreq->setText(QString("%1 MHz").arg(rx_freq_mhz, 11, 'f', 6, ' '));
}


/*! \brief Select new demodulator.
 *  \param demod Demodulator index corresponding to receiver::demod.
 */
void DockRxOpt::setCurrentDemod(int demod)
{
    ui->modeSelector->setCurrentIndex(demod);
}


/*! \brief Get current demodulator selection.
 *  \return The current demodulator corresponding to receiver::demod.
 */
int  DockRxOpt::currentDemod()
{
    return ui->modeSelector->currentIndex();
}


/*! \brief Select new SSB side band.
 *  \param sideband New SSB side band selection (0=LSB, 1=USB).
 */
void DockRxOpt::setCurrentSideBand(int sideband)
{
    ui->sidebandSelector->setCurrentIndex(sideband);
}


/*! \brief Get current SSB side band selection.
 *  \returns The current SSB side band selection (0=LSB, 1=USB).
 */
int  DockRxOpt::currentSideBand()
{
    return ui->sidebandSelector->currentIndex();
}


/*! \brief Select new value in max_dev selector.
 *  \param maxdev The new value to select.
 */
void DockRxOpt::setCurrentMaxdev(float maxdev)
{
    if (maxdev < 4000.0) {
        /* select 2500 */
        ui->maxdevSelector->setCurrentIndex(0);
    }
    else if (maxdev < 10000.0) {
        /* select 5k */
        ui->maxdevSelector->setCurrentIndex(1);
    }
    else if (maxdev < 35000.0) {
        /* select 17k */
        ui->maxdevSelector->setCurrentIndex(2);
    }
    else {
        /* select 75k */
        ui->maxdevSelector->setCurrentIndex(3);
    }
}


float DockRxOpt::currentMaxdev()
{
    float max_dev;
    int index = ui->maxdevSelector->currentIndex();

    switch (index) {
    case 0:
        max_dev = 2500.0;
        break;

    case 1:
        max_dev = 5000.0;
        break;

    case 2:
        max_dev = 17000.0;
        break;

    case 3:
        max_dev = 75000.0;
        break;

    default:
        qDebug() << "Invalid max_dev index: " << index;
        max_dev = 5000.0;
        break;

    }

    return max_dev;
}


/*! \brief Channel filter offset has changed
 *  \param freq The new filter offset in Hz
 *
 * This slot is activated when a new filter offset has been selected either
 * usig the mouse or using the keyboard.
 */
void DockRxOpt::on_filterFreq_NewFrequency(qint64 freq)
{
    qDebug() << "New filter offset:" << freq << "Hz";
    updateRxFreq();

    emit filterOffsetChanged(freq);
}

/*! \brief Filter shape (TBC).
 */
void DockRxOpt::on_filterButton_clicked()
{

}

/*! \brief Mode selector activated.
 *  \param New mode selection (see receiver::demod).
 *
 * This slot is activated when the user selects a new demodulator (mode change).
 * It is connected automatically by the UI constructor, and it emits the demodSelected()
 * signal.
 */
void DockRxOpt::on_modeSelector_activated(int index)
{
    qDebug() << "New mode: " << index;
    emit demodSelected(index);
}


/*! \brief New FM maximum deviation selected.
 *  \param index The index of the selcted item (fixed max_dev options)
 */
void DockRxOpt::on_maxdevSelector_activated(int index)
{
    float max_dev;

    switch (index) {
    case 0:
        max_dev = 2500.0;
        break;

    case 1:
        max_dev = 5000.0;
        break;

    case 2:
        max_dev = 17000.0;
        break;

    case 3:
        max_dev = 75000.0;
        break;

    default:
        qDebug() << "Invalid max_dev index: " << index;
        max_dev = 5000.0;
        break;

    }

    emit fmMaxdevSelected(max_dev);

}


/*! \brief New FM de-emphasis time constant selected.
 *  \param index The index of the new selection (fixed tau options).
 */
void DockRxOpt::on_emphSelector_activated(int index)
{
    double tau_tbl[] = { 0.0, 25.0, 50.0, 75.0, 100.0, 250.0, 530.0, 1000.0};
    double tau;

    if ((index < 0) || (index > 7)) {
        qDebug() << "Invalid tau selection index: " << index;
        return;
    }

    tau = tau_tbl[index] * 1.0e-6;
    emit fmEmphSelected(tau);
}


/*! \brief AM DCR checkbox state toggled
 *  \param checked Whether the checkbox is checked or not.
 */
void DockRxOpt::on_dcr_toggled(bool checked)
{
    emit amDcrToggled(checked);
}


/*! \brief SSB side band selected.
 *  \param The side band band selection (0=LSB, 1=USB).
 *
 * This slot is activated when the user selects a new side band in SSB mode.
 * It is connected automatically by the UI constructor and it emits the sidebandSelected()
 * signal.
 */
void DockRxOpt::on_sidebandSelector_activated(int index)
{
    qDebug() << "New side band: " << index;
    emit sidebandSelected(index);
}

/*! \brief AGC preset has changed.
 *
 * 0: Fast    decay = 500 msec
 * 1: Medium  decay = 1500 msec
 * 2: Slow    decay = 3000 msec
 * 3: User    decay = set by user
 * 4: Off     Only fixed gain
 */
void DockRxOpt::on_agcPresetCombo_activated(int index)
{
    qDebug() << "NEW AGC preset:" << index;

    switch (index) {
    case 0:
        if (!agc_is_on) {
            emit agcToggled(true);
            agc_is_on = true;
        }
        // decay
        ui->agcDecayDial->setValue(100);
        emit agcDecayChanged(100);
        ui->agcDecayDial->setEnabled(false);
        // slope
        ui->agcSlopeDial->setValue(2);
        emit agcSlopeChanged(2);
        ui->agcSlopeDial->setEnabled(false);
        break;
    case 1:
        if (!agc_is_on) {
            emit agcToggled(true);
            agc_is_on = true;
        }
        // decay
        ui->agcDecayDial->setValue(800);
        emit agcDecayChanged(800);
        ui->agcDecayDial->setEnabled(false);
        // slope
        ui->agcSlopeDial->setValue(2);
        emit agcSlopeChanged(2);
        ui->agcSlopeDial->setEnabled(false);
        break;
    case 2:
        if (!agc_is_on) {
            emit agcToggled(true);
            agc_is_on = true;
        }
        // decay
        ui->agcDecayDial->setValue(2000);
        emit agcDecayChanged(2000);
        ui->agcDecayDial->setEnabled(false);
        // slope
        ui->agcSlopeDial->setValue(2);
        emit agcSlopeChanged(2);
        ui->agcSlopeDial->setEnabled(false);
        break;
    case 3:
        if (!agc_is_on) {
            emit agcToggled(true);
            agc_is_on = true;
        }
        ui->agcDecayDial->setEnabled(true);
        ui->agcSlopeDial->setEnabled(true);
        break;
    case 4:
        if (agc_is_on) {
            emit agcToggled(false);
            agc_is_on = false;
        }
        break;
    default:
        qDebug() << "Invalid AGC preset:" << index;
    }
}

void DockRxOpt::on_agcHangButton_toggled(bool checked)
{
    emit agcHangToggled(checked);
}

/*! \brief AGC threshold ("knee") changed.
 *  \param value The new AGC threshold in dB.
 */
void DockRxOpt::on_agcThresholdDial_valueChanged(int value)
{
    qDebug() << "AGC threshold:" << value;
    emit agcThresholdChanged(value);
}

/*! \brief AGC slope factor changed.
 *  \param value The new slope factor in dB.
 */
void DockRxOpt::on_agcSlopeDial_valueChanged(int value)
{
    qDebug() << "AGC slope:" << value;
    emit agcSlopeChanged(value);
}

/*! \brief AGC decay changed.
 *  \param value The new slope factor in dB.
 */
void DockRxOpt::on_agcDecayDial_valueChanged(int value)
{
    qDebug() << "AGC decay:" << value;
    emit agcDecayChanged(value);
}

/*! \brief AGC manual gain changed.
 *  \param gain The new gain in dB.
 */
void DockRxOpt::on_agcGainDial_valueChanged(int gain)
{
    qDebug() << "AGC manual gain:" << gain;
    emit agcGainChanged(gain);
}

/*! \brief Squelch level change.
 *  \param value The new squelch level in tens of dB (because slider uses int).
 */
void DockRxOpt::on_sqlSlider_valueChanged(int value)
{
    double level = double(value) / 10.0;

    //ui->sqlValueLabel->setText(QString("%1 dB").arg(level));
    emit sqlLevelChanged(level);
}
