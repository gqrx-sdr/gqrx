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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "qtgui/dockrxopt.h"
#include "qtgui/dockaudio.h"
#include "qtgui/dockfcdctl.h"
#include "qtgui/dockiqrecorder.h"
#include "qtgui/dockfft.h"

#include <receiver.h>


namespace Ui {
    class MainWindow;  /*! The main window UI */
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void setNewFrequency(qint64 freq);

private:

    enum receiver::filter_shape d_filter_shape;
    std::complex<float>* d_fftData;
    double *d_realFftData;

    Ui::MainWindow *ui;
    DockRxOpt      *uiDockRxOpt;
    DockAudio      *uiDockAudio;
    DockFcdCtl     *uiDockFcdCtl;
    DockIqRecorder *uiDockIqRec;
    DockFft        *uiDockFft;

    QTimer   *meter_timer;
    QTimer   *fft_timer;
    receiver *rx;

private slots:
    void setRfGain(float gain);
    void setDcCorr(double dci, double dcq);
    void setIqCorr(double gain, double phase);
    void selectDemod(int index);
    void setFmMaxdev(float max_dev);
    void setFmEmph(double tau);
    void setAmDcrStatus(bool enabled);
    void setSideBand(int sideband);
    void setBasebandGain(float gain);
    void setSqlLevel(double level_db);
    void setAudioGain(float gain);

    /* audio recording */
    void startAudioRec(const QString filename);
    void stopAudioRec();

    /* FFT settings */
    void setFftSize(int size);
    void setFftRate(int fps);
    void setFftYmin(int value);
    void setFftYmax(int value);
    void setFftSplit(int pct_wf);

    void on_plotter_NewDemodFreq(qint64 freq, qint64 delta);   /*! New demod freq (aka. filter offset). */
    void on_plotter_NewFilterFreq(int low, int high);    /*! New filter width */

    /* menu and toolbar actions */
    void on_actionDSP_triggered(bool checked);
    void on_actionIODevices_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();

    /* cyclic processing */
    void meterTimeout();
    void fftTimeout();

};

#endif // MAINWINDOW_H
