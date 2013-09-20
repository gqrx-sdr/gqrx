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
#include "dockinputctl.h"
#include "ui_dockinputctl.h"

DockInputCtl::DockInputCtl(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockInputCtl)
{
    ui->setupUi(this);

    // gain options dialog
    gainOpt = new CGainOptions(this);
}

DockInputCtl::~DockInputCtl()
{
    delete ui;
    delete gainOpt;
}

void DockInputCtl::readSettings(QSettings *settings)
{
    bool conv_ok;

    setFreqCorr(settings->value("input/corr_freq", 0).toInt(&conv_ok));
    emit freqCorrChanged(ui->freqCorrSpinBox->value());

    setIqSwap(settings->value("input/swap_iq", false).toBool());
    emit iqSwapChanged(ui->iqSwapButton->isChecked());

    setDcCancel(settings->value("input/dc_cancel", false).toBool());
    emit dcCancelChanged(ui->dcCancelButton->isChecked());

    setIqBalance(settings->value("input/iq_balance", false).toBool());
    emit iqBalanceChanged(ui->iqBalanceButton->isChecked());

    qint64 lnb_lo = settings->value("input/lnb_lo", 0).toLongLong(&conv_ok);
    setLnbLo(((double)lnb_lo)/1.0e6);
    emit lnbLoChanged(ui->lnbSpinBox->value());

    bool ignore_limits = settings->value("input/ignore_limits", false).toBool();
    setIgnoreLimits(ignore_limits);
    emit ignoreLimitsChanged(ignore_limits);

    double gain = settings->value("input/gain", -1).toDouble(&conv_ok);
    setGain(gain);
    emit gainChanged(gain);

    // Ignore antenna selection if there is only one option
    if (ui->antSelector->count() > 1)
    {
        QString ant = settings->value("input/antenna", "").toString();
        setAntenna(ant);
    }

}

void DockInputCtl::saveSettings(QSettings *settings)
{
    qint64 lnb_lo = (qint64)(ui->lnbSpinBox->value()*1.e6);
    if (lnb_lo)
        settings->setValue("input/lnb_lo", lnb_lo);
    else
        settings->remove("input/lnb_lo");

    double dblval = gain();
    settings->setValue("input/gain", dblval);

    if (freqCorr())
        settings->setValue("input/corr_freq", freqCorr());
    else
        settings->remove("input/corr_freq");

    if (iqSwap())
        settings->setValue("input/swap_iq", true);
    else
        settings->remove("input/swap_iq");

    if (dcCancel())
        settings->setValue("input/dc_cancel", true);
    else
        settings->remove("input/dc_cancel");

    if (iqBalance())
        settings->setValue("input/iq_balance", true);
    else
        settings->remove("input/iq_balance");

    if (ignoreLimits())
        settings->setValue("input/ignore_limits", true);
    else
        settings->remove("input/ignore_limits");

    // save antenna selection if there is more than one option
    if (ui->antSelector->count() > 1)
        settings->setValue("input/antenna", ui->antSelector->currentText());
    else
        settings->remove("input/antenna");
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

/*! \brief Enable automatic DC removal. */
void DockInputCtl::setDcCancel(bool enabled)
{
    ui->dcCancelButton->setChecked(enabled);
}

/*! \brief Get current DC remove status. */
bool DockInputCtl::dcCancel(void)
{
    return ui->dcCancelButton->isChecked();
}

/*! \brief Enable automatic IQ balance. */
void DockInputCtl::setIqBalance(bool enabled)
{
    ui->iqBalanceButton->setChecked(enabled);
}

/*! \brief Get current IQ balance status. */
bool DockInputCtl::iqBalance(void)
{
    return ui->iqBalanceButton->isChecked();
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

/*! \brief Populate antenna selector combo box with strings. */
void DockInputCtl::setAntennas(std::vector<std::string> &antennas)
{
    ui->antSelector->clear();
    for (std::vector<std::string>::iterator it = antennas.begin(); it != antennas.end(); ++it)
    {
        ui->antSelector->addItem(QString(it->c_str()));
    }
}

/*! \brief Select antenna. */
void DockInputCtl::setAntenna(const QString &antenna)
{
    int index = ui->antSelector->findText(antenna, Qt::MatchExactly);
    if (index != -1)
        ui->antSelector->setCurrentIndex(index);
}

/*! \brief Set gain stages.
 *  \param gain_list A list containing the gain stages for this device.
 */
void DockInputCtl::setGainStages(gain_list_t &gain_list)
{
    gainOpt->setGainStages(gain_list);
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

/*! \brief Gain options buttion clicked. Show dialog. */
void DockInputCtl::on_gainOptButton_pressed()
{
    gainOpt->show();
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

/*! \brief DC removal checkbox changed.
 *  \param checked True if DC removal is enabled, false otherwise
 */
void DockInputCtl::on_dcCancelButton_toggled(bool checked)
{
    emit dcCancelChanged(checked);
}

/*! \brief IQ balance checkbox changed.
 *  \param checked True if automatic IQ balance is enabled, false otherwise
 */
void DockInputCtl::on_iqBalanceButton_toggled(bool checked)
{
    emit iqBalanceChanged(checked);
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

/*! \brief Antenna selection has changed. */
void DockInputCtl::on_antSelector_currentIndexChanged(const QString &antenna)
{
    emit antennaSelected(antenna);
}
