/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
 * Copyright 2014 Alexandru Csete OZ9AEC.
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
#include <QDebug>
#include "iq_tool.h"
#include "ui_iq_tool.h"


CIqTool::CIqTool(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CIqTool)
{
    ui->setupUi(this);

    is_recording = false;
    is_playing = false;
    bytes_per_sample = 8;
    sample_rate = 192000;
}

CIqTool::~CIqTool()
{
    delete ui;
}


/*! \brief Start/stop recording */
void CIqTool::on_recButton_clicked(bool checked)
{
    if (checked)
    {
        if (is_recording)
        {
            qDebug() << "An I/Q recording is already in progress";
            return;
        }
        is_recording = true;

        // get a unique filename

        // add new file to list

        // disable buttons
        ui->playButton->setEnabled(false);
        ui->locationButton->setEnabled(false);
        ui->locationEntry->setEnabled(false);

        // emit recording signal
        recfile = "/home/alc/gqrx_iq_2014.02.28_23:00:00_14236000_192000_fc.raw";
        emit start_recording(recfile);

        // start monitoring thread


    }
    else
    {
        if (!is_recording)
        {
            qDebug() << "No I/Q recording is in progress";
            return;
        }
        is_recording = false;

        // emit stop signal
        emit stop_recording();

        // stop monitoring thread

        // re-enable buttons
        ui->playButton->setEnabled(true);
        ui->locationButton->setEnabled(true);
        ui->locationEntry->setEnabled(true);
    }

}

/*! \brief Cancel a recording.
 *
 * This slot can be activated to cancel an ongoing recording. Cancelling an
 * ongoing recording will stop the recording and delete the recorded file, if
 * any.
 *
 * This slot should be used to signal that a recording could not be started.
 */
void CIqTool::cancelRecording()
{
    if (is_recording)
    {
        // update button and object states
        is_recording = false;
        ui->recButton->setChecked(false);
        ui->playButton->setEnabled(true);
        ui->locationButton->setEnabled(true);
        ui->locationEntry->setEnabled(true);

        // delete file

        // show an error message to user
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.setText(tr("There was an error starting the I/Q recorder.\n"
                           "Check write permissions for the selected location."));
        msg_box.exec();
    }
    else
    {
        qDebug() << "Error: No recording to cancel";
    }
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CIqTool::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}
