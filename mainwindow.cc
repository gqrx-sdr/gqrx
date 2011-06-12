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
    ui->rxFreqLabel->setText("144.500000 MHz");

    /* create receiver object */
    rx = new receiver("hw:1");

    rx->set_rf_freq(144500000.0f);

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
}

MainWindow::~MainWindow()
{
    /* stop and delete timers */
    meter_timer->stop();
    delete meter_timer;

    /* clean up the rest */
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
    double rx_freq_mhz;

    rx->set_rf_freq((float) freq);

    rx_freq_mhz = (freq + rx->get_filter_offset()) / 1.0e6;
    ui->rxFreqLabel->setText(QString("%1 MHz").arg(rx_freq_mhz, 11, 'f', 6, ' '));
}


void MainWindow::on_rxStartStopButton_toggled(bool checked)
{
    if (checked) {
        /* start receiver */
        rx->start();

        /* start GUI timers */
        meter_timer->start(100);

        /* update button label */
        ui->rxStartStopButton->setText(tr("Stop"));
    }
    else {
        /* stop GUI timers */
        meter_timer->stop();

        /* stop receiver */
        rx->stop();

        ui->rxStartStopButton->setText(tr("Start"));
    }
}


/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_NewDemodFreq(qint64 f)
{
    qDebug() << "New demod freq: " << f;
}


/* CPlotter::NewLowCutFreq() is emitted */
void MainWindow::on_plotter_NewLowCutFreq(int f)
{
    qDebug() << "New low cut freq: " << f;
}


/* CPlotter::NewLowHighFreq() is emitted */
void MainWindow::on_plotter_NewHighCutFreq(int f)
{
    qDebug() << "New high cut freq: " << f;
}


/* temporary slot for receiving slider signals (tune within passband) */
void MainWindow::on_tuningSlider_valueChanged(int value)
{
    double rx_freq_mhz;

    //qDebug() << "TUNE: " << value;
    rx->set_filter_offset((double) value);

    rx_freq_mhz = (value + rx->get_rf_freq()) / 1.0e6;
    ui->rxFreqLabel->setText(QString("%1 MHz").arg(rx_freq_mhz, 11, 'f', 6, ' '));

}


/*! \brief Audio gain changed.
 *  \param value The new audio gain.
 */
void MainWindow::on_audioGainSlider_valueChanged(int value)
{
    rx->set_af_gain(((float)value) / 10.0);
}


/*! \brief Signal strength meter timeout */
void MainWindow::meterTimeout()
{
    float level;

    level = rx->get_signal_pwr(true);
    ui->rxSigLabel->setText(QString("%1 dBFS").arg(level, 7, 'f', 2, ' '));
    ui->sMeter->setLevel(level);
}
