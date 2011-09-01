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
    ui(new Ui::DockRxOpt)
{
    ui->setupUi(this);
}

DockRxOpt::~DockRxOpt()
{
    delete ui;
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


/*! \brief Baseband gain changed.
 *  \param value The new audio gain value in dB.
 */
void DockRxOpt::on_bbGainDial_valueChanged(int value)
{
    float gain = float(value);

    emit bbGainChanged(gain);
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
