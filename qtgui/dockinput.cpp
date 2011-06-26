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
#include "dockinput.h"
#include "ui_dockinput.h"

DockInput::DockInput(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockInput)
{
    ui->setupUi(this);
}

DockInput::~DockInput()
{
    delete ui;
}


/*! \brief DC offset changed (I). */
void DockInput::on_fcdDciSpinBox_valueChanged(double dci)
{
    double dcq = ui->fcdDcqSpinBox->value();

    emit fcdDcCorrChanged(dci, dcq);
}


/*! \brief DC offset changed (Q). */
void DockInput::on_fcdDcqSpinBox_valueChanged(double dcq)
{
    double dci = ui->fcdDciSpinBox->value();

    emit fcdDcCorrChanged(dci, dcq);
}


/*! \brief IQ phase changed. */
void DockInput::on_fcdIqPhaseSpinBox_valueChanged(double phase)
{
    double gain = ui->fcdIqGainSpinBox->value();

    emit fcdIqCorrChanged(gain, phase);
}


/*! \brief IQ gain changed. */
void DockInput::on_fcdIqGainSpinBox_valueChanged(double phase)
{
    double gain = ui->fcdIqGainSpinBox->value();

    emit fcdIqCorrChanged(gain, phase);
}
