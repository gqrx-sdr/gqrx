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
#include "dockdemod.h"
#include "ui_dockdemod.h"

DockDemod::DockDemod(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockDemod)
{
    ui->setupUi(this);
}

DockDemod::~DockDemod()
{
    delete ui;
}


/*! \brief Select new demodulator.
 *  \param demod Demodulator index corresponding to receiver::demod.
 */
void DockDemod::setCurrentDemod(int demod)
{
    ui->modeSelector->setCurrentIndex(demod);
}


/*! \brief Get current demodulator selection.
 *  \return The current demodulator corresponding to receiver::demod.
 */
int  DockDemod::currentDemod()
{
    return ui->modeSelector->currentIndex();
}


/*! \brief Select new SSB side band.
 *  \param sideband New SSB side band selection (0=LSB, 1=USB).
 */
void DockDemod::setCurrentSideBand(int sideband)
{
    ui->sidebandSelector->setCurrentIndex(sideband);
}


/*! \brief Get current SSB side band selection.
 *  \returns The current SSB side band selection (0=LSB, 1=USB).
 */
int  DockDemod::currentSideBand()
{
    return ui->sidebandSelector->currentIndex();
}


/*! \brief Mode selector activated.
 *  \param New mode selection (see receiver::demod).
 *
 * This slot is activated when the user selects a new demodulator (mode change).
 * It is connected automatically by the UI constructor, and it emits the demodSelected()
 * signal.
 */
void DockDemod::on_modeSelector_activated(int index)
{
    qDebug() << "New mode: " << index;
    emit demodSelected(index);
}


/*! \brief SSB side band selected.
 *  \param The side band band selection (0=LSB, 1=USB).
 *
 * This slot is activated when th euser selects a new side band in SSB mode.
 * It is connected automatically by the UI constructor and it emits the sidebandSelected()
 * signal.
 */
void DockDemod::on_sidebandSelector_activated(int index)
{
    qDebug() << "New side band: " << index;
    emit sidebandSelected(index);
}
