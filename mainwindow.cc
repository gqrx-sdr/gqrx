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
#include "receiver.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QString("gqrx %1").arg(VERSION));

    /* frequency control widget */
    ui->freqCtrl->Setup(10, (quint64) 50e6, (quint64) 2e9, 1, UNITS_MHZ);
    ui->freqCtrl->SetFrequency(144500000);
    ui->rxFreqLabel->setText("144.500000 MHz");

    d_filter_shape = receiver::FILTER_SHAPE_NORMAL;

    /* create receiver object */
    rx = new receiver("hw:1");

    rx->set_rf_freq(144500000.0f);

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* FFT timer & data */
    fft_timer = new QTimer(this);
    connect(fft_timer, SIGNAL(timeout()), this, SLOT(fftTimeout()));
    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new double[MAX_FFT_SIZE];


    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
}

MainWindow::~MainWindow()
{
    /* stop and delete timers */
    meter_timer->stop();
    delete meter_timer;

    fft_timer->stop();
    delete fft_timer;

    /* clean up the rest */
    delete ui;
    delete rx;
    delete [] d_fftData;
    delete [] d_realFftData;
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

    /* set receiver frequency */
    rx->set_rf_freq((float) freq);

    /* update pandapter */
    ui->plotter->SetCenterFreq(freq);

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
        fft_timer->start(100);

        /* update button label */
        ui->rxStartStopButton->setText(tr("Stop"));
    }
    else {
        /* stop GUI timers */
        meter_timer->stop();
        fft_timer->stop();

        /* stop receiver */
        rx->stop();

        ui->rxStartStopButton->setText(tr("Start"));
    }
}


/* CPlotter::NewDemodFreq() is emitted */
void MainWindow::on_plotter_NewDemodFreq(qint64 freq, qint64 delta)
{
    double rx_freq_mhz;

    //qDebug() << "New demod freq: " << freq << "  Delta: " << delta;
    rx->set_filter_offset((double) delta);

    rx_freq_mhz = ((double)freq) / 1.0e6;
    ui->rxFreqLabel->setText(QString("%1 MHz").arg(rx_freq_mhz, 11, 'f', 6, ' '));
}



/* CPlotter::NewfilterFreq() is emitted */
void MainWindow::on_plotter_NewFilterFreq(int low, int high)
{
    receiver::status retcode;


    /* parameter correctness will be checked in receiver class */
    retcode = rx->set_filter((double) low, (double) high, d_filter_shape);

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

/*! \brief FFT plot timeout. */
void MainWindow::fftTimeout()
{
    int fftsize;
    int i;
    std::complex<float> pt;             /* a single FFT point used in calculations */
    std::complex<float> scaleFactor;    /* normalizing factor (fftsize cast to complex) */
    double min=0.0,max=-120.0,avg=0.0;


    rx->get_fft_data(d_fftData, fftsize);

    scaleFactor = std::complex<float>((float)fftsize);


    /* Normalize, calculcate power and shift the FFT */
    for (i = 0; i < fftsize; i++) {

        /*if (i < fftsize/2) {
            d_realFftData[i] = 10.0*log10((d_fftData[fftsize/2+i].imag() * d_fftData[fftsize/2+i].imag() +
                                           d_fftData[fftsize/2+i].real() * d_fftData[fftsize/2+i].real()) + 1.0e-20);
        }
        else {
            d_realFftData[i] = 10.0*log10((d_fftData[i-fftsize/2].imag() * d_fftData[i-fftsize/2].imag() +
                                           d_fftData[i-fftsize/2].real() * d_fftData[i-fftsize/2].real()) + 1.0e-20);

        }*/

        /* normalize and shift */
        if (i < fftsize/2) {
            pt = d_fftData[fftsize/2+i] / scaleFactor;
        }
        else {
            pt = d_fftData[i-fftsize/2] / scaleFactor;
        }

        /* calculate power in dBFS */
        d_realFftData[i] = 10.0 * log10(pt.imag()*pt.imag() + pt.real()*pt.real() + 1.0e-20);

        if (d_realFftData[i] < min)
            min = d_realFftData[i];

        if (d_realFftData[i] > max)
            max = d_realFftData[i];

        avg = (avg+d_realFftData[i]) / 2.0;

        //d_realFftData[i] = -110.0;

    }

    ui->plotter->SetNewFttData(d_realFftData, fftsize);

    //qDebug() << "FFT size: " << fftsize;
    //qDebug() << "FFT[0]=" << d_realFftData[0] << "  FFT[MID]=" << d_realFftData[fftsize/2];
    //qDebug() << "MIN:" << min << "  AVG:" << avg << "  MAX:" << max;


}
