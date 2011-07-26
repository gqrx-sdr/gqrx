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
#include <QSettings>
#include <QDebug>
#include "qtgui/ioconfig.h"
#include "mainwindow.h"

/* Qt Designer files */
#include "ui_mainwindow.h"


/* DSP */
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
    QSettings settings;
    QString indev = settings.value("input").toString();
    QString outdev = settings.value("output").toString();
    rx = new receiver(indev.toStdString(), outdev.toStdString());

    rx->set_rf_freq(144500000.0f);

    /* meter timer */
    meter_timer = new QTimer(this);
    connect(meter_timer, SIGNAL(timeout()), this, SLOT(meterTimeout()));

    /* FFT timer & data */
    fft_timer = new QTimer(this);
    connect(fft_timer, SIGNAL(timeout()), this, SLOT(fftTimeout()));
    d_fftData = new std::complex<float>[MAX_FFT_SIZE];
    d_realFftData = new double[MAX_FFT_SIZE];

    /* create dock widgets */
    uiDockRxOpt = new DockRxOpt();
    uiDockAudio = new DockAudio();
    uiDockFcdCtl = new DockFcdCtl(this);

    //addDockWidget(Qt::RightDockWidgetArea, uiDockInput);
    addDockWidget(Qt::RightDockWidgetArea, uiDockFcdCtl);
    addDockWidget(Qt::RightDockWidgetArea, uiDockRxOpt);
    tabifyDockWidget(uiDockFcdCtl, uiDockRxOpt);
    addDockWidget(Qt::RightDockWidgetArea, uiDockAudio);


    /* Add dock widget actions to View menu. By doing it this way all signal/slot
       connections will be established automagially.
    */
    ui->menu_View->addAction(uiDockRxOpt->toggleViewAction());
    ui->menu_View->addAction(uiDockAudio->toggleViewAction());
    ui->menu_View->addAction(uiDockFcdCtl->toggleViewAction());
    ui->menu_View->addSeparator();
    ui->menu_View->addAction(ui->mainToolBar->toggleViewAction());

    /* connect signals and slots */
    connect(ui->freqCtrl, SIGNAL(NewFrequency(qint64)), this, SLOT(setNewFrequency(qint64)));
    connect(uiDockFcdCtl, SIGNAL(dcCorrChanged(double,double)), this, SLOT(setDcCorr(double,double)));
    connect(uiDockFcdCtl, SIGNAL(iqCorrChanged(double,double)), this, SLOT(setIqCorr(double,double)));
    connect(uiDockRxOpt, SIGNAL(demodSelected(int)), this, SLOT(selectDemod(int)));
    connect(uiDockRxOpt, SIGNAL(fmMaxdevSelected(float)), this, SLOT(setFmMaxdev(float)));
    connect(uiDockRxOpt, SIGNAL(fmEmphSelected(double)), this, SLOT(setFmEmph(double)));
    connect(uiDockRxOpt, SIGNAL(sidebandSelected(int)), this, SLOT(setSideBand(int)));
    connect(uiDockRxOpt, SIGNAL(bbGainChanged(float)), this, SLOT(setBasebandGain(float)));
    connect(uiDockRxOpt, SIGNAL(sqlLevelChanged(double)), this, SLOT(setSqlLevel(double)));
    connect(uiDockAudio, SIGNAL(audioGainChanged(float)), this, SLOT(setAudioGain(float)));
    connect(uiDockAudio, SIGNAL(audioRecStarted(QString)), this, SLOT(startAudioRec(QString)));
    connect(uiDockAudio, SIGNAL(audioRecStopped()), this, SLOT(stopAudioRec()));
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
    delete uiDockRxOpt;
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


/*! \brief Start/Stop DSP processing.
 *  \param checked Flag indicating whether DSP processing should be ON or OFF.
 *
 * This slot is executed when the actionDSP is toggled by the user. This can either be
 * via the menu bar or the "power on" button in the main toolbar.
 */
void MainWindow::on_actionDSP_triggered(bool checked)
{
    if (checked) {
        /* start receiver */
        rx->start();

        /* start GUI timers */
        meter_timer->start(100);
        fft_timer->start(100);

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Stop DSP processing"));
        ui->actionDSP->setText(tr("Stop DSP"));
    }
    else {
        /* stop GUI timers */
        meter_timer->stop();
        fft_timer->stop();

        /* stop receiver */
        rx->stop();

        /* update menu text and button tooltip */
        ui->actionDSP->setToolTip(tr("Start DSP processing"));
        ui->actionDSP->setText(tr("Start DSP"));
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


/*! \brief Set new DC offset values.
 *  \param dci I correction.
 *  \param dcq Q correction.
 *
 * The valid range is between -1.0 and 1.0, though hthis is not checked.
 */
void MainWindow::setDcCorr(double dci, double dcq)
{
    qDebug() << "DCI:" << dci << "  DCQ:" << dcq;
    rx->set_dc_corr(dci, dcq);
}


/*! \brief Set new IQ correction values.
 *  \param gain IQ gain correction.
 *  \param phase IQ phase correction.
 *
 * The valid range is between -1.0 and 1.0, though hthis is not checked.
 */
void MainWindow::setIqCorr(double gain, double phase)
{
    qDebug() << "Gain:" << gain << "  Phase:" << phase;
    rx->set_iq_corr(gain, phase);
}


/*! \brief Select new demodulator.
 *  \param demod New demodulator index, see receiver::demod.
 */
void MainWindow::selectDemod(int index)
{
    receiver::demod mode = (receiver::demod)index;

    rx->set_demod(mode);

    switch (mode) {

    case receiver::DEMOD_SSB:
        if (uiDockRxOpt->currentSideBand()) {
            /* USB */
            ui->plotter->SetDemodRanges(0, 500, 600, 10000, false);
            ui->plotter->SetHiLowCutFrequencies(200, 3000);
            rx->set_filter(200.0, 3000.0, receiver::FILTER_SHAPE_NORMAL);
        }
        else {
            /* LSB */
            ui->plotter->SetDemodRanges(-10000, -500, -400, 0, false);
            ui->plotter->SetHiLowCutFrequencies(-3000, -200);
            rx->set_filter(-3000.0, -200.0, receiver::FILTER_SHAPE_NORMAL);
        }
        break;

    case receiver::DEMOD_AM:
        ui->plotter->SetDemodRanges(-15000, -2000, 2000, 15000, true);
        ui->plotter->SetHiLowCutFrequencies(-5000, 5000);
        rx->set_filter(-5000.0, 5000.0, receiver::FILTER_SHAPE_NORMAL);
        break;

    case receiver::DEMOD_FM:
        /** FIXME: add full support for maxdev and de-emphasis */
        ui->plotter->SetDemodRanges(-15000, -2000, 2000, 15000, true);
        ui->plotter->SetHiLowCutFrequencies(-5000, 5000);
        rx->set_filter(-5000.0, 5000.0, receiver::FILTER_SHAPE_NORMAL);
        break;

/** APT:
        ui->plotter->SetDemodRanges(-25000, -5000, 5000, 25000, true);
        ui->plotter->SetHiLowCutFrequencies(-10000, 10000);
        rx->set_filter(-10000.0, 10000.0, receiver::FILTER_SHAPE_NORMAL);
*/

/** FMW:
        ui->plotter->SetDemodRanges(-45000, -10000, 10000, 45000, true);
        ui->plotter->SetHiLowCutFrequencies(-40000, 40000);
        rx->set_filter(-40000.0, 40000.0, receiver::FILTER_SHAPE_NORMAL);
*/

/*    case receiver::DEMOD_B1K:
        ui->plotter->SetDemodRanges(-5000, -500, 500, 5000, true);
        ui->plotter->SetHiLowCutFrequencies(-2000, 2000);
        rx->set_filter(-2000.0, 2000.0, receiver::FILTER_SHAPE_NORMAL);
        break;
*/
    default:
        qDebug() << "Invalid mode selection: " << mode;
        break;

    }

}


/*! \brief New FM deviation selected.
 *  \param max_dev The enw FM deviation.
 */
void MainWindow::setFmMaxdev(float max_dev)
{
    qDebug() << "FM MAX_DEV: " << max_dev;

    /* receiver will check range */
    rx->set_fm_maxdev(max_dev);
}

/*! \brief New FM de-emphasis time consant selected.
 *  \param tau The new time constant
 */
void MainWindow::setFmEmph(double tau)
{
    qDebug() << "FM TAU: " << tau;

    /* receiver will check range */
    rx->set_fm_deemph(tau);
}


/*! \brief AM DCR status changed (slot).
 *  \param enabled Whether DCR is enabled or not.
 */
void MainWindow::setAmDcrStatus(bool enabled)
{
    rx->set_am_dcr(enabled);
}

/*! \brief Set new SSB sideband.
 *  \param sideband The new sideband(= = LSB, 1 = USB)
 */
void MainWindow::setSideBand(int sideband)
{
    receiver::demod mode = (receiver::demod)uiDockRxOpt->currentDemod();

    if (mode != receiver::DEMOD_SSB) {
        // This should not be possible because sideband selector is only
        // visible when demodulator type is SSB
        qDebug() << "Sideband selected but current mode is not SSB";
        return;
    }

    if (sideband) {
        /* USB */
        ui->plotter->SetDemodRanges(0, 500, 600, 10000, false);
        ui->plotter->SetHiLowCutFrequencies(200, 3000);
        rx->set_filter(200.0, 3000.0, receiver::FILTER_SHAPE_NORMAL);
    }
    else {
        /* LSB */
        ui->plotter->SetDemodRanges(-10000, -600, -500, 0, false);
        ui->plotter->SetHiLowCutFrequencies(-3000, -200);
        rx->set_filter(-3000.0, -200.0, receiver::FILTER_SHAPE_NORMAL);
    }

}


/*! \brief BaSEBAND gain changed.
 *  \param value The new BASEBAND gain in dB.
 */
void MainWindow::setBasebandGain(float value)
{
    rx->set_bb_gain(value);
}



/*! \brief Audio gain changed.
 *  \param value The new audio gain in dB.
 */
void MainWindow::setAudioGain(float value)
{
    rx->set_af_gain(value);
}


/*! \brief Squelch level changed.
 *  \param level_db The new squelch level in dBFS.
 */
void MainWindow::setSqlLevel(double level_db)
{
    rx->set_sql_level(level_db);
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


/*! \brief Action: I/O device configurator triggered.
 *
 * This slot is activated when the user selects "I/O Devices" in the
 * menu. It activates the I/O configurator and if the user closes the
 * configurator using the OK button, the new configuration is read and
 * sent to the receiver.
 */
void MainWindow::on_actionIODevices_triggered()
{
    QSettings settings;
    QString cindev = settings.value("input").toString();
    QString coutdev = settings.value("output").toString();


    CIoConfig *ioconf = new CIoConfig();
    int confres = ioconf->exec();

    if (confres == QDialog::Accepted) {
        QString nindev = settings.value("input").toString();
        QString noutdev = settings.value("output").toString();

        // we need to ensure that we don't reconfigure RX
        // with the same device as the already used one because
        // that can crash the receiver when using ALSA :(
        if (cindev != nindev)
            rx->set_input_device(nindev.toStdString());

        if (coutdev != noutdev)
            rx->set_output_device(noutdev.toStdString());
    }

    delete ioconf;
}


/*! \brief Start audio recorder.
 *  \param filename The file name into which audio should be recorded.
 */
void MainWindow::startAudioRec(const QString filename)
{
    rx->start_recording(filename.toStdString());
}


/*! \brief Stop audio recorder. */
void MainWindow::stopAudioRec()
{
    rx->stop_recording();
}




/*! \brief Action: About Qthid
 *
 * This slot is called when the user activates the
 * Help|About menu item (or Gqrx|About on Mac)
 */
void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About Gqrx"),
                       tr("<p>This is Gqrx %1</p>"
                          "<p>Gqrx is a software defined radio receiver for Funcube Dongle and "
                          "Universal Software Radio Peripheral devices by Ettus Research via the UHD driver.</p>"
                          "<p>Gqrx is powered by GNU Radio and the Qt toolkit (see About Qt) and is avaialble "
                          "for Linux, Mac and Windows. You can download the latest version from the "
                          "<a href='http://www.oz9aec.net/'>TBD website</a>.</p>"
                          "<p>"
                          "<a href='http://www.gnuradio.org/'>GNU Radio website</a><br/>"
                          "<a href='http://www.ettus.com/'>Ettus Research (USRP)</a><br/>"
                          "<a href='http://funcubedongle.com/'>Funcube Dongle website</a><br/>"
                          "</p>").arg(VERSION));
}

/*! \brief Action: About Qt
 *
 * This slot is called when the user activates the
 * Help|About Qt menu item (or Gqrx|About Qt on Mac)
 */
void MainWindow::on_actionAboutQt_triggered()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
}
