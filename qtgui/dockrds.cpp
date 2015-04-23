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
#include <QVariant>
#include "dockrds.h"
#include "ui_dockrds.h"

DockRDS::DockRDS(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockRDS)
{
    ui->setupUi(this);
 }

DockRDS::~DockRDS()
{
    delete ui;
}

void DockRDS::updateRDS(QString text, int type)
{
    /* type 0 = PI
     * type 1 = PS
     * type 2 = PTY
     * type 3 = flagstring: TP, TA, MuSp, MoSt, AH, CMP, stPTY
     * type 4 = RadioText
     * type 5 = ClockTime
     * type 6 = Alternative Frequencies */

    if (type==0) {
        ui->program_information->setText(text);
    } else if (type==1) {
        ui->station_name->setText(text);
    } else if (type==2) {
        ui->program_type->setText(text);
    } else if (type==3) {
        std::string str = text.toStdString();
        std::string out="";
        if (str.at(0)=='1') out.append("TP ");
        if (str.at(1)=='1') out.append("TA ");
        if (str.at(2)=='0') out.append("Speech ");
        if (str.at(2)=='1') out.append("Music ");
        if (str.at(3)=='0') out.append("Stereo ");
        if (str.at(3)=='1') out.append("Mono ");
        if (str.at(4)=='1') out.append("AH ");
        if (str.at(5)=='1') out.append("CMP ");
        if (str.at(6)=='1') out.append("stPTY ");
        ui->flags->setText(QString::fromStdString(out));
    } else if (type==4) {
        ui->radiotext->setText(text);
    } else if (type==5) {
        ui->clocktime->setText(text);
    } else if (type==6) {
        ui->alt_freq->setText(text);
    }
}

void DockRDS::showEnabled()
{
    ui->rdsCheckbox->setText("Enabled");
}

void DockRDS::showDisabled()
{
    ui->rdsCheckbox->setText("Disabled");
    ui->program_information->setText("");
    ui->station_name->setText("");
    ui->program_type->setText("");
    ui->flags->setText("");
    ui->radiotext->setText("");
    ui->clocktime->setText("");
    ui->alt_freq->setText("");
}

void DockRDS::setDisabled()
{
 ui->rdsCheckbox->setDisabled(true);

 ui->rdsCheckbox->blockSignals(true);
 ui->rdsCheckbox->setChecked(false);
 ui->rdsCheckbox->blockSignals(false);
}

void DockRDS::setEnabled()
{
 ui->rdsCheckbox->setDisabled(false);
}

/*! \brief Enable/disable RDS decoder */
void DockRDS::on_rdsCheckbox_toggled(bool checked)
{
    emit rdsDecoderToggled(checked);
}
