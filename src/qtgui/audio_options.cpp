/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2013-2016 Alexandru Csete OZ9AEC.
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
#include <QFileDialog>
#include <QPalette>
#include <QDebug>
#include <iostream>

#include "audio_options.h"
#include "ui_audio_options.h"

CAudioOptions::CAudioOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CAudioOptions)
{
    ui->setupUi(this);

    work_dir = new QDir();

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);
}

CAudioOptions::~CAudioOptions()
{
    delete work_dir;
    delete error_palette;
    delete ui;
}


/**
 * Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CAudioOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();

    // check if we ended up with empty dir, if yes reset to $HOME
    if (ui->recDirEdit->text().isEmpty())
    {
        setRecDir(QDir::homePath());
        emit newRecDirSelected(QDir::homePath());
    }

    if (ui->udpHost->text().isEmpty())
    {
        ui->udpHost->setText("localhost");
    }
}

/** Set initial location of WAV files. */
void CAudioOptions::setRecDir(const QString &dir)
{
    ui->recDirEdit->setText(dir);
}

/** Set new UDP host name or IP. */
void CAudioOptions::setUdpHost(const QString &host)
{
    ui->udpHost->setText(host);
}

/** Set new UDP port. */
void CAudioOptions::setUdpPort(int port)
{
    ui->udpPort->setValue(port);
}

/** Set new UDP stereo setting. */
void CAudioOptions::setUdpStereo(bool stereo)
{
    ui->udpStereo->setChecked(stereo);
}


void CAudioOptions::setFftSplit(int pct_2d)
{
    ui->fftSplitSlider->setValue(pct_2d);
}

int  CAudioOptions::getFftSplit(void) const
{
    return ui->fftSplitSlider->value();
}

void CAudioOptions::on_fftSplitSlider_valueChanged(int value)
{
    emit newFftSplit(value);
}

void CAudioOptions::setPandapterRange(int min, int max)
{
    if (min < max && max <= 0)
        ui->pandRangeSlider->setValues(min, max);
}

void CAudioOptions::getPandapterRange(int * min, int * max) const
{
    *min = ui->pandRangeSlider->minimumValue();
    *max = ui->pandRangeSlider->maximumValue();
}

void CAudioOptions::on_pandRangeSlider_valuesChanged(int min, int max)
{
    if (ui->audioLockButton->isChecked())
        ui->wfRangeSlider->setValues(min, max);

    m_pand_last_modified = true;
    emit newPandapterRange(min, max);
}

void CAudioOptions::setWaterfallRange(int min, int max)
{
    if (min < max && max <= 0)
        ui->wfRangeSlider->setValues(min, max);
}

void CAudioOptions::getWaterfallRange(int * min, int * max) const
{
    *min = ui->wfRangeSlider->minimumValue();
    *max = ui->wfRangeSlider->maximumValue();
}

void CAudioOptions::on_wfRangeSlider_valuesChanged(int min, int max)
{
    if (ui->audioLockButton->isChecked())
        ui->pandRangeSlider->setValues(min, max);

    m_pand_last_modified = false;
    emit newWaterfallRange(min, max);
}

void CAudioOptions::setLockButtonState(bool checked)
{
    ui->audioLockButton->setChecked(checked);
}

bool CAudioOptions::getLockButtonState(void) const
{
    return ui->audioLockButton->isChecked();
}

void CAudioOptions::setPandapterSliderValues(float min, float max)
{
    ui->pandRangeSlider->blockSignals(true);
    ui->pandRangeSlider->setValues((int)min, (int)max);
    if (ui->audioLockButton->isChecked())
        ui->wfRangeSlider->setValues((int) min, (int) max);
    m_pand_last_modified = true;
    ui->pandRangeSlider->blockSignals(false);
}

/** Lock button toggled */
void CAudioOptions::on_audioLockButton_toggled(bool checked)
{
    if (checked) {
        if (m_pand_last_modified)
        {
            int min = ui->pandRangeSlider->minimumValue();
            int max = ui->pandRangeSlider->maximumValue();
            ui->wfRangeSlider->setPositions(min, max);
        }
        else
        {
            int min = ui->wfRangeSlider->minimumValue();
            int max = ui->wfRangeSlider->maximumValue();
            ui->pandRangeSlider->setPositions(min, max);
        }
    }
}


/**
 * Slot called when the recordings directory has changed either
 * because of user input or programmatically.
 */
void CAudioOptions::on_recDirEdit_textChanged(const QString &dir)
{

    if (work_dir->exists(dir))
    {
        ui->recDirEdit->setPalette(QPalette());  // Clear custom color
        emit newRecDirSelected(dir);
    }
    else
    {
        ui->recDirEdit->setPalette(*error_palette);  // indicate error
    }
}

/** Slot called when the user clicks on the "Select" button. */
void CAudioOptions::on_recDirButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
                                                    ui->recDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!dir.isNull())
        ui->recDirEdit->setText(dir);
}

/** UDP host name has changed. */
void CAudioOptions::on_udpHost_textChanged(const QString &text)
{
    if (!text.isEmpty())
        emit newUdpHost(text);
}

/** UDP port number has changed. */
void CAudioOptions::on_udpPort_valueChanged(int port)
{
    emit newUdpPort(port);
}

/** UDP stereo setting has changed. */
void CAudioOptions::on_udpStereo_stateChanged(int state)
{
    emit newUdpStereo(state);
}

/** Zmq host name has changed. */
void CAudioOptions::on_ZmqHost_textChanged(const QString &text)
{
    if (!text.isEmpty())
        emit newZmqHost(text);
}

/** Zmq port number has changed. */
void CAudioOptions::on_ZmqPort_valueChanged(int port)
{
    emit newZmqPort(port);
}

/** Zmq stereo setting has changed. */
void CAudioOptions::on_zmqStream_toggled()
{
    emit newZmqStream();
}
