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
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QDebug>
#include "bpsk1000win.h"
#include "ui_bpsk1000win.h"


Bpsk1000Win::Bpsk1000Win(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Bpsk1000Win),
    demodBytes(0), demodFramesT(0), demodFramesB1(0), demodFramesB2(0),
    demodFramesB3(0), demodFramesB4(0)
{
    ui->setupUi(this);

    /* select font for text viewer */
#ifdef Q_OS_MAC
    ui->textView->setFont(QFont("Monaco", 12));
#else
    ui->textView->setFont(QFont("Monospace", 11));
#endif

    /* Add right-aligned info button */
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(spacer);
    ui->toolBar->addAction(ui->actionInfo);

    /* start demodulator */
    demod = new QProcess(this);
    connect(demod, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(demodStateChanged(QProcess::ProcessState)));
    connect(demod, SIGNAL(readyReadStandardOutput()), this, SLOT(readDemodData()));
    demod->start("./demod", QIODevice::ReadWrite);
}

Bpsk1000Win::~Bpsk1000Win()
{
    qDebug() << "BPSK1000 decoder destroyed.";

    demod->close();
    delete demod;

    delete ui;
}


/*! \brief Process new set of samples. */
void Bpsk1000Win::process_samples(float *buffer, int length)
{
    qint16 *int_buffer;  // input samples converted to int
    const char *ch_ptr;  // input samples cast to char

    // only process input if demod is running
    if (demod->state() != QProcess::Running)
        return;

    int_buffer = new qint16[length];
    ch_ptr = reinterpret_cast<const char *>(int_buffer);

    /* convert input samples to signed int */
    for (int i=0; i < length; i++) {
        int_buffer[i] = (qint16)(buffer[i]*32767.);
    }

    //in_pipe->write(ch_ptr, length*sizeof(qint16));
    demod->write(ch_ptr, length*sizeof(qint16));

    delete [] int_buffer;
}


/*! \brief Catch window close events and emit signal so that main application can destroy us. */
void Bpsk1000Win::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev);

    emit windowClosed();
}


/*! \brief Demodulator process state changed.
 *  \param newState The new state of the demodulator proces.
 */
void Bpsk1000Win::demodStateChanged(QProcess::ProcessState newState)
{
    QString demodState[] = {"Not Running", "Starting", "Running"};

    ui->statusBar->showMessage(demodState[newState]);
    qDebug() << "Demodulator process state changed to:" << demodState[newState];
}


/*! \brief Read data from the demodulator.
 *
 * This slot is connected to the readyReadStandardOutput() signal of the
 * demodulator process. It reads the available data from the standard output
 * of the demodulator and executes the apropriate post processing and display
 * functions.
 */
void Bpsk1000Win::readDemodData()
{
    QByteArray data;

    demodBytes += demod->bytesAvailable();
    qDebug() << "Bytes from demod: " << demod->bytesAvailable();
    data = demod->readAllStandardOutput();

    ui->textView->appendPlainText(QString(data.toHex()));
}


/*! \brief User clicked on the Clear button. */
void Bpsk1000Win::on_actionClear_triggered()
{
    ui->textView->clear();
}


/*! \brief User clicked on the Save button. */
void Bpsk1000Win::on_actionSave_triggered()
{
    /* empty text view has blockCount = 1 */
    if (ui->textView->blockCount() < 2) {
        QMessageBox::warning(this, tr("Gqrx error"), tr("Nothing to save."),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    QDir::homePath(),
                                                    tr("Text Files (*.txt)"));

    if (fileName.isEmpty()) {
        qDebug() << "Save cancelled by user";
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Error creating file: " << fileName;
        return;
    }

    QTextStream out(&file);
    out << ui->textView->toPlainText();
    file.close();
}


/*! \brief User clicked Info button. */
void Bpsk1000Win::on_actionInfo_triggered()
{
    QMessageBox::about(this, tr("About BPSK1000 Decoder"),
                       tr("<p>Gqrx BPSK1000 Decoder %1</p>"
                          "<p>The Gqrx BPSK1000 decoder taps directly into the SDR signal path "
                          "eliminating the need to mess with virtual or real audio cables. "
                          "It can be sued to decode telemetry data from ARISSat-1.</p>"
                          "<p>The decoder is based on the reference demodulator by <a href='http://www.ka9q.net/'>Phil Karn KA9Q</a>.</p>"
                          ).arg(VERSION));

}
