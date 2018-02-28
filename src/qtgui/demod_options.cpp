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


/* convert between deemphasis time constant and combo-index */
const double tau_tbl[] = {
    0.0, 25.0e-6, 50.0e-6, 75.0e-6, 100.0e-6, 250.0e-6, 530.0e-6, 1.0e-3
};
const int tau_tbl_maxidx = 7;

double tau_from_index(int index)
{
    if (index < 0 || index > tau_tbl_maxidx)
        return 0.0;

    return tau_tbl[index];
}

int tau_to_index(double tau)
{
    int i;
    for (i = 0; i < tau_tbl_maxidx; i++)
    {
        if (tau < (tau_tbl[i] + 0.5 * (tau_tbl[i+1] - tau_tbl[i])))
            return i;
    }
    return 0;
}

/* convert betweenFM max dev and combo index */
float maxdev_from_index(int index)
{
    switch(index)
    {
    case 0:
        /* Voice 2.5k */
        return 2500.0;
    case 1:
        /* Voice 5k */
        return 5000.0;
    case 2:
        /* APT 17k */
        return 17000.0;
    case 3:
        /* APT 25k (17k but need some margin for Doppler and freq error) */
        return 25000.0;
    default:
        /* Voice 5k */
        qDebug() << "Invalid max_dev index: " << index;
        return 5000.0;
    }
}

int maxdev_to_index(float max_dev)
{
    if (max_dev < 3000.0)
        /* Voice 2.5k */
        return 0;
    else if (max_dev < 10000.0)
        /* Voice 5k */
        return 1;
    else if (max_dev < 20000.0)
        /* APT 17k */
        return 2;
    else
        /* APT 25k */
        return 3;
}

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

int CDemodOptions::currentPage() const
{
    return ui->demodOptions->currentIndex();
}

void CDemodOptions::setCwOffset(int offset)
{
    ui->cwOffsetSpin->setValue(offset);
}

int  CDemodOptions::getCwOffset(void) const
{
    return ui->cwOffsetSpin->value();
}

void CDemodOptions::setMaxDev(float max_dev)
{
    ui->maxdevSelector->setCurrentIndex(maxdev_to_index(max_dev));
}

float CDemodOptions::getMaxDev(void) const
{
    return maxdev_from_index(ui->maxdevSelector->currentIndex());
}

void CDemodOptions::setEmph(double tau)
{
    ui->emphSelector->setCurrentIndex((tau_to_index(tau)));
}

double CDemodOptions::getEmph(void) const
{
    return tau_from_index(ui->emphSelector->currentIndex());
}

void CDemodOptions::on_maxdevSelector_activated(int index)
{
    emit fmMaxdevSelected(maxdev_from_index(index));
}

void CDemodOptions::on_emphSelector_activated(int index)
{
    emit fmEmphSelected(tau_from_index(index));
}

void CDemodOptions::on_dcrCheckBox_toggled(bool checked)
{
    emit amDcrToggled(checked);
}

void CDemodOptions::on_cwOffsetSpin_valueChanged(int value)
{
    emit cwOffsetChanged(value);
}
