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
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* frequency control widget */
    ui->freqCtrl->Setup(10, (quint64) 50e6, (quint64) 2e9, 1, UNITS_MHZ);
    ui->freqCtrl->SetFrequency(144500000);

    /* create receiver object */
    rx = new receiver("hw:1");

    rx->set_rf_freq(144500000.0f);

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete rx;
}


/*! \brief Slot for receiving frequency change signals.
 *  \param[in] freq The new frequency.
 *
 * This slot is connected to the CFreqCtrl::NewFrequency() signal and is used
 * to set new RF frequency.
 */
void MainWindow::setNewFrequency(qint64 freq)
{
    rx->set_rf_freq((float) freq);
}


void MainWindow::on_rxStartStopButton_toggled(bool checked)
{
    if (checked) {
        /* start receiver and update button label */
        rx->start();
        ui->rxStartStopButton->setText(tr("Stop"));
    }
    else {
        /* stop receiver and update label */
        rx->stop();
        ui->rxStartStopButton->setText(tr("Start"));
    }
}

/* temporary slot for receiving slider signals (tune within passband) */
void MainWindow::on_tuningSlider_valueChanged(int value)
{
    qDebug() << "TUNE: " << value;
    rx->set_filter_offset((double) value);
}


/*! \brief Audio gain changed.
 *  \param value The new audio gain.
 */
void MainWindow::on_audioGainSlider_valueChanged(int value)
{
    rx->set_af_gain(((float)value) / 10.0);
}
