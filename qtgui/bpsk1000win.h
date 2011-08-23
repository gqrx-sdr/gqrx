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
#ifndef BPSK1000WIN_H
#define BPSK1000WIN_H

#include <QMainWindow>
#include <QVarLengthArray>
#include <QProcess>
#include <QComboBox>
#include "qtgui/arissattlm.h"


namespace Ui {
    class Bpsk1000Win;
}


/*! \brief BPSK1000 decoder window.
 *
 * This is the top level class for decoding BPSK1000 telemetry data from ARISSat-1.
 * It uses the reference demodulator by Phil Karn, KA9Q, see http://www.ka9q.net/
 *
 * The demodulator application is started using QProcess. Data is excahnged via stdin
 * and stdout, which are automatically available when starting external processes
 * using QProcess.
 *
 * Incoming samples from the SDR chain are provided periodically by the main application.
 * These samples 48ksps float format -1.0 ... +1.0 and need to be converted to 16 bit
 * signed integer, see http://wiki.oz9aec.net/index.php/Demod2
 *
 * The decoded data is shown in HEX format in the list view (one packet per line) and
 * optionally dumped to a text file. In the case of ARISSat telementry frames, the data
 * is further decoded and shown at the lower part fo the window.
 *
 * The decoder can also run in offline analysis mode. In this mode incoming samples are
 * ignored and the telemetry display will show the line currently selected in the list
 * view widget.
 */
class Bpsk1000Win : public QMainWindow
{
    Q_OBJECT

public:
    explicit Bpsk1000Win(QWidget *parent = 0);
    ~Bpsk1000Win();
    void process_samples(float *buffer, int length);

protected:
    void closeEvent(QCloseEvent *ev);

signals:
    void windowClosed();  /*! Signal we emit when window is closed. */

private slots:
    void demodStateChanged(QProcess::ProcessState newState);
    void readDemodData();
    void readDemodDebug();

    void profileSelected(int index);
    void on_listView_currentRowChanged(int row);
    //void listViewRowSelected(int row);

    // button actions
    void on_actionClear_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionInfo_triggered();
    void on_actionRealtime_triggered(bool checked);

private:
    void decodeTlm(QByteArray &data);

private:
    Ui::Bpsk1000Win *ui;           /*! Qt Designer form. */
    QComboBox       *profileCombo; /*! Telemetry profile selector. */
    QProcess        *demod;        /*! Demodulator process. */
    QLabel          *numFramesT;   /*! Label on statusbar showing number of telemetry frames. */
    QLabel          *numFramesE;   /*! Label on statusbar showing number of experiment frames. */

    bool    realtime; /*! Weather we are runnign in real time mode. */

    quint64 demodBytes;    /*! Bytes received from demod. */
    quint64 demodFramesT;  /*! Telemetry frames received from demod. */
    quint64 demodFramesE;  /*! Experiment frames received from demod. */

    // telemetry viewers
    ArissatTlm  *tlmArissat;  /*! Arissat-1 telemetry viewer. */
};

#endif // BPSK1000WIN_H
