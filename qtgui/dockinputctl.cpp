/* -*- c++ -*- */
/*
 * Copyright 2011-2012 Alexandru Csete OZ9AEC.
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
#include "dockinputctl.h"
#include "ui_dockinputctl.h"

DockInputCtl::DockInputCtl(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockInputCtl)
{
    ui->setupUi(this);
}

DockInputCtl::~DockInputCtl()
{
    delete ui;
}


void DockInputCtl::setLnbLo(double freq_mhz)
{
    ui->lnbSpinBox->setValue(freq_mhz);
}

double DockInputCtl::lnbLo()
{
    return ui->lnbSpinBox->value();
}


/*! \brief Set new relative gain.
 *  \param gain The new gain in the range 0.0 .. 1.0 or -1 to enable AGC.
 */
void DockInputCtl::setGain(double gain)
{
    if (gain > 1.0)
    {
        qDebug() << "DockInputCtl::setGain :" << gain;
        gain = 1.0;
    }

    if (gain < 0.0)
    {
        ui->gainButton->setChecked(true);
    }
    else
    {
        ui->gainButton->setChecked(false);
        ui->gainSlider->setValue((int)(gain*100.0));
    }
}

/*! \brief Get current gain.
 *  \returns The relative gain between 0.0 and 1.0 or -1 if HW AGC is enabled.
 */
double DockInputCtl::gain()
{
    double gain = ui->gainButton->isChecked() ? -1.0 : (double)ui->gainSlider->value()/100.0;

    return gain;
}


/*! \brief Set new frequency correction.
 *  \param corr The new frequency correction in PPM.
 */
void DockInputCtl::setFreqCorr(int corr)
{
    ui->freqCorrSpinBox->setValue(corr);
}


/*! \brief Get current frequency correction. */
int DockInputCtl::freqCorr()
{
    return ui->freqCorrSpinBox->value();
}

/*! \brief Enasble/disable I/Q swapping. */
void DockInputCtl::setIqSwap(bool reversed)
{
    ui->iqSwapButton->setChecked(reversed);
}

/*! \brief Get current I/Q swapping. */
bool DockInputCtl::iqSwap(void)
{
    return ui->iqSwapButton->isChecked();
}


/*! \brief Enasble/disable ignoring hardware limits. */
void DockInputCtl::setIgnoreLimits(bool reversed)
{
    ui->ignoreButton->setChecked(reversed);
}

/*! \brief Get current status of whether limits should be ignored or not. */
bool DockInputCtl::ignoreLimits(void)
{
    return ui->ignoreButton->isChecked();
}


/*! \brief LNB LO value has changed. */
void DockInputCtl::on_lnbSpinBox_valueChanged(double value)
{
    emit lnbLoChanged(value);
}


/*! \brief Manual gain value has changed. */
void DockInputCtl::on_gainSlider_valueChanged(int value)
{
    double gain = (double)value/100.0;

    emit gainChanged(gain);
}

/*! \brief Automatic gain control button has been toggled. */
void DockInputCtl::on_gainButton_toggled(bool checked)
{
    double gain = checked ? -1.0 : (double)ui->gainSlider->value()/100.0;
    ui->gainSlider->setEnabled(!checked);

    emit gainChanged(gain);
}


/*! \brief Frequency correction changed.
 *  \param value The new frequency correction in ppm.
 */
void DockInputCtl::on_freqCorrSpinBox_valueChanged(int value)
{
    emit freqCorrChanged(value);
}

/*! \brief I/Q swapping checkbox changed.
 *  \param checked True if I/Q swapping is enabled, false otherwise
 */
void DockInputCtl::on_iqSwapButton_toggled(bool checked)
{
    emit iqSwapChanged(checked);
}

/*! \brief Ignore hardware limits checkbox changed.
 *  \param checked True if hardware limits should be ignored, false otherwise
 *
 * This option exists to allow experimenting with out-of-spec settings.
 */
void DockInputCtl::on_ignoreButton_toggled(bool checked)
{
    emit ignoreLimitsChanged(checked);
}
