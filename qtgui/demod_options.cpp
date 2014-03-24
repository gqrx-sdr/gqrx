/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include "demod_options.h"
#include "ui_demod_options.h"

CDemodOptions::CDemodOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CDemodOptions)
{
    ui->setupUi(this);
}

CDemodOptions::~CDemodOptions()
{
    delete ui;
}


/*! \brief Catch window close events.
 *
 * This method is called when the user closes the demod options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CDemodOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void CDemodOptions::setCurrentPage(int index)
{
    if (index < PAGE_NUM)
        ui->demodOptions->setCurrentIndex(index);
}

int CDemodOptions::currentPage()
{
    return ui->demodOptions->currentIndex();
}


void CDemodOptions::on_maxdevSelector_activated(int index)
{
    float max_dev;

    switch(index)
    {
    case 0:
        /* Voice 2.5k */
        max_dev = 2500.0;
        break;
    case 1:
        /* Voice 5k */
        max_dev = 5000.0;
        break;
    case 2:
        /* APT 17k */
        max_dev = 17000.0;
        break;
    case 3:
        /* Broadcast FM 75k */
        max_dev = 75000.0;
        break;
    default:
        /* Voice 5k */
        qDebug() << "Invalid max_dev index: " << index;
        max_dev = 5000.0;
        break;
    }

    emit fmMaxdevSelected(max_dev);
}

void CDemodOptions::on_emphSelector_activated(int index)
{
    double tau_tbl[] = {0.0, 25.0, 50.0, 75.0, 100.0, 250.0, 530.0, 1000.0};
    double tau;

    if ((index < 0) || (index > 7)) {
        qDebug() << "Invalid tau selection index: " << index;
        return;
    }

    tau = tau_tbl[index] * 1.0e-6;
    emit fmEmphSelected(tau);
}

void CDemodOptions::on_dcrCheckBox_toggled(bool checked)
{
    emit amDcrToggled(checked);
}
