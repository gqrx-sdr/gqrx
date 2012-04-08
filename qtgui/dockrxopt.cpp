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

    /* demodulator options dialog */
    demodOpt = new CDemodOptions(this);
    demodOpt->setCurrentPage(CDemodOptions::PAGE_FM_OPT);
}

DockRxOpt::~DockRxOpt()
{
    delete ui;
    delete demodOpt;
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


/*! \brief Select new filter preset.
 *  \param index Index of the new filter preset (0=wide, 1=normal, 2=narrow).
 */
void DockRxOpt::setCurrentFilter(int index)
{
    ui->filterCombo->setCurrentIndex(index);
}

/*! \brief Get current filter preset.
 *  \param The current filter preset (0=wide, 1=normal, 2=narrow).
 */
int  DockRxOpt::currentFilter()
{
    return ui->filterCombo->currentIndex();
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


float DockRxOpt::currentMaxdev()
{
    qDebug() << __FILE__ << __FUNCTION__ << "FIXME";
    return 5000.0;
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

/*! \brief New filter preset selected.
 *
 * Instead of implementing a new signal, we simply emit demodSelected() since demodulator
 * and filter preset are tightly coupled.
 */
void DockRxOpt::on_filterCombo_activated(int index)
{
    Q_UNUSED(index);

    qDebug() << "New filter preset:" << ui->filterCombo->currentText();
    emit demodSelected(ui->modeSelector->currentIndex());
}

/*! \brief Filter shape (TBC).
 */
void DockRxOpt::on_filterButton_clicked()
{

}

/*! \brief Mode selector activated.
 *  \param New mode selection.
 *
 * This slot is activated when the user selects a new demodulator (mode change).
 * It is connected automatically by the UI constructor, and it emits the demodSelected()
 * signal.
 *
 * Note that the modes listed in the selector are different from those defined by
 * receiver::demod (we want to list LSB/USB separately but they have identical demods).
 */
void DockRxOpt::on_modeSelector_activated(int index)
{
    qDebug() << "New mode: " << index;

    if (!index)
    {
        qDebug() << "Raw I/Q not implemented (fallback to FM)";
        ui->modeSelector->setCurrentIndex(2);

        return;
    }

    /* update demodulator option widget */
    if (index == 2)
        demodOpt->setCurrentPage(CDemodOptions::PAGE_FM_OPT);
    else
        demodOpt->setCurrentPage(CDemodOptions::PAGE_NO_OPT);

    emit demodSelected(index);
}

/*! \brief Show demodulator options.
 */
void DockRxOpt::on_modeButton_clicked()
{
    demodOpt->show();
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
