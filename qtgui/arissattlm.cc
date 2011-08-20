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
#include <QString>
#include <QTime>
#include <QDebug>
#include "arissattlm.h"
#include "ui_arissattlm.h"
#include "tlm/arissat/ss_types_common.h"


ArissatTlm::ArissatTlm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ArissatTlm)
{
    ui->setupUi(this);
}

ArissatTlm::~ArissatTlm()
{
    delete ui;
}


/*! \brief Process new data. */
void ArissatTlm::processData(QByteArray &data)
{
    QByteArray ba(data);
    ss_telem_t tlm;

    // remove first two bytes which appear to be some header
    ba.remove(0, 2);

    // check first byte: 'T'
    if (!ba.startsWith('T')) {
        qDebug() << "Data does not start with 'T'" << ba[0];
        return;
    }
    if (ba.size() != 371) {
        qDebug() << "Data length is not 371 bytes";
        return;
    }

    // extract the frame counter
    memcpy(&frameCounter, ba.data()+1, 4);

    // now copy the ss_telem_t structure
    memcpy(&tlm, ba.data()+27, 340);

    showMissionData(tlm);
}


/*! \brief Display main mission data. */
void ArissatTlm::showMissionData(ss_telem_t &tlm)
{
    // MET
    quint32 days = tlm.mission_time / 86400;
    QTime   t0(0,0), met;
    int met_sec = (int) (tlm.mission_time - days*86400);
    met = t0.addSecs(met_sec);
    ui->metLabel->setText(QString(" %1d %2 ").arg(days).arg(met.toString("HH:mm:ss")));

    // telemetry frame counter
    ui->frameCountLabel->setText(QString::number(frameCounter));

    // Mission mode
    // TODO: colors, see http://www.qtcentre.org/threads/195-Setting-text-color-on-QLabel
    switch (tlm.mission_mode) {
    case EMERGENCY_PWR:
        ui->modeLabel->setText(tr("Emergency Power"));
        break;
    case LOW_PWR:
        ui->modeLabel->setText(tr("Low Power"));
        break;
    case HIGH_PWR:
        ui->modeLabel->setText(tr("High Power"));
        break;
    case TXINHIBIT_PWR:
        ui->modeLabel->setText(tr("TX Inhibit"));
        break;
    default:
        ui->modeLabel->setText(tr("Invalid mode"));
        break;
    }

}
