/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
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
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFileSystemWatcher>
#include <QPalette>
#include <QString>
#include <QStringList>
#include <QScrollBar>

#include <math.h>

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
    rec_len = 0;
    center_freq = 1e8;

    //ui->recDirEdit->setText(QDir::currentPath());

    recdir = new QDir(QDir::homePath(), "*.raw", QDir::Time | QDir::Reversed);

    watcher = new QFileSystemWatcher();
    connect(watcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(refreshDir(void)));

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutFunction()));
}

CIqTool::~CIqTool()
{
    timer->stop();
    delete watcher;
    delete timer;
    delete ui;
    delete recdir;
    delete error_palette;

}

/*! \brief Set new sample rate. */
void CIqTool::setSampleRate(qint64 sr)
{
    sample_rate = sr;

    if (!current_file.isEmpty())
    {
        // Get duration of selected recording and update label
        QFileInfo info(*recdir, current_file);
        rec_len = (int)(info.size() / (sample_rate * bytes_per_sample));
        refreshTimeWidgets();
    }
}


/*! \brief Slot activated when the user selects a file. */
void CIqTool::on_listWidget_currentTextChanged(const QString &currentText)
{

    current_file = currentText;
    QFileInfo info(*recdir, current_file);

    parseFileName(currentText);
    rec_len = (int)(info.size() / (sample_rate * bytes_per_sample));

    // Get duration of selected recording and update label
    refreshTimeWidgets();
}


/*! \brief Start/stop playback */
void CIqTool::on_playButton_clicked(bool checked)
{
    if (checked)
    {
        if (current_file.isEmpty())
        {
            QMessageBox msg_box;
            msg_box.setIcon(QMessageBox::Critical);
            if (ui->listWidget->count() == 0)
            {
                msg_box.setText(tr("There are no I/Q files in the current directory."));
            }
            else
            {
                msg_box.setText(tr("Please select a file to play."));
            }
            msg_box.exec();

            ui->playButton->setChecked(false); // will not trig clicked()
        }
        else
        {
            is_playing = true;
            ui->recButton->setEnabled(false);
            ui->recDirEdit->setEnabled(false);
            ui->recDirButton->setEnabled(false);
            ui->listWidget->setEnabled(false);
            emit startPlayback(recdir->absoluteFilePath(current_file),
                               (float)sample_rate, center_freq);
            timer->start(1000);
        }
    }
    else
    {
        cancelPlayback();
    }
}

/*! \brief Cancel playback.
 *
 * This slot can be activated to cancel an ongoing playback.
 *
 * This slot should be used to signal that a playback could not be started.
 */
void CIqTool::cancelPlayback()
{
    if (!is_playing)
        return;

    timer->stop();
    emit stopPlayback();
    ui->slider->setValue(0);
    ui->playButton->setChecked(false);
    ui->recButton->setEnabled(true);
    ui->recDirEdit->setEnabled(true);
    ui->recDirButton->setEnabled(true);
    ui->listWidget->setEnabled(true);
    is_playing = false;
}


/*! \brief Slider value (seek position) has changed. */
void CIqTool::on_slider_valueChanged(int value)
{
    refreshTimeWidgets();

    qint64 seek_pos = (qint64)(value)*sample_rate;
    emit seek(seek_pos);
}


/*! \brief Start/stop recording */
void CIqTool::on_recButton_clicked(bool checked)
{
    if (checked)
    {
        is_recording = true;
        ui->playButton->setEnabled(false);
        ui->recDirEdit->setEnabled(false);
        ui->recDirButton->setEnabled(false);
        ui->listWidget->setEnabled(false);
        emit startRecording(recdir->path());
        timer->start(1000);

        // Highlight bottom entry, presumably the current recording
        refreshDir();
        ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
        recording_file = (QString)ui->listWidget->currentItem()->text();
    }
    else
    {
        cancelRecording();
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
    if (!is_recording)
        return;

    timer->stop();
    recording_file = QString("");
    emit stopRecording();
    ui->recButton->setChecked(false);
    ui->playButton->setEnabled(true);
    ui->recDirEdit->setEnabled(true);
    ui->recDirButton->setEnabled(true);
    ui->listWidget->setEnabled(true);
    is_recording = false;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void CIqTool::closeEvent(QCloseEvent *event)
{
    watcher->removePath(recdir->absolutePath());
    hide();
    event->ignore();
}

/*! \brief Catch window show events. */
void CIqTool::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
    watcher->addPath(recdir->absolutePath());
    refreshDir();
    refreshTimeWidgets();
}


void CIqTool::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    // Location of baseband recordings
    QString dir = recdir->path();
    if (dir != QDir::homePath())
        settings->setValue("baseband/rec_dir", dir);
    else
        settings->remove("baseband/rec_dir");

}

void CIqTool::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    // Location of baseband recordings
    QString dir = settings->value("baseband/rec_dir", QDir::homePath()).toString();
    ui->recDirEdit->setText(dir);
}


/*! \brief Slot called when the recordings directory has changed either
 *         because of user input or programmatically.
 */
void CIqTool::on_recDirEdit_textChanged(const QString &dir)
{
    if (!dir.isEmpty() && recdir->exists(dir))
    {
        watcher->removePath(recdir->absolutePath());
        ui->recDirEdit->setPalette(QPalette());  // Clear custom color
        recdir->setPath(dir);
        recdir->cd(dir);
        refreshDir();
        watcher->addPath(recdir->absolutePath());
        //emit newRecDirSelected(dir);
    }
    else
    {
        ui->recDirEdit->setPalette(*error_palette);  // indicate error
    }
}

/*! \brief Slot called when the user clicks on the "Select" button. */
void CIqTool::on_recDirButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a directory"),
                                                    ui->recDirEdit->text(),
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);

    if (!dir.isNull())
        ui->recDirEdit->setText(dir);
}

void CIqTool::timeoutFunction(void)
{
    if (is_playing)
    {
        // advance slider with one second
        int val = ui->slider->value();
        if (val < ui->slider->maximum())
        {
            ui->slider->blockSignals(true);
            ui->slider->setValue(val+1);
            ui->slider->blockSignals(false);
            refreshTimeWidgets();
        }
    }
    if (is_recording) {
        // Update recording length
        QFileInfo info(*recdir, recording_file);
        rec_len = (int)(info.size() / (sample_rate * bytes_per_sample));
        refreshTimeWidgets();
    }
}

/*! \brief Refresh list of files in current working directory. */
void CIqTool::refreshDir(void)
{
    int selection = ui->listWidget->currentRow();
    QScrollBar * sc = ui->listWidget->verticalScrollBar();
    int lastScroll = sc->sliderPosition();

    recdir->refresh();
    QStringList files = recdir->entryList();

    ui->listWidget->blockSignals(true);
    ui->listWidget->clear();
    ui->listWidget->insertItems(0, files);
    ui->listWidget->setCurrentRow(selection);
    sc->setSliderPosition(lastScroll);
    ui->listWidget->blockSignals(false);

    if (is_recording)
    {
        // update rec_len; if the file being recorded is the one selected
        // in the list, the length will update periodically
        QFileInfo info(*recdir, current_file);
        rec_len = (int)(info.size() / (sample_rate * bytes_per_sample));
    }
}

/*! \brief Refresh time labels and slider position
 *
 * \note Safe for recordings > 24 hours
 */
void CIqTool::refreshTimeWidgets(void)
{
    ui->slider->setMaximum(rec_len);

    // duration
    int len = rec_len;
    int lh, lm, ls;
    lh = len / 3600;
    len = len % 3600;
    lm = len / 60;
    ls = len % 60;

    // current position
    int pos = ui->slider->value();
    int ph, pm, ps;
    ph = pos / 3600;
    pos = pos % 3600;
    pm = pos / 60;
    ps = pos % 60;

    ui->timeLabel->setText(QString("%1:%2:%3 / %4:%5:%6")
                           .arg(ph, 2, 10, QChar('0'))
                           .arg(pm, 2, 10, QChar('0'))
                           .arg(ps, 2, 10, QChar('0'))
                           .arg(lh, 2, 10, QChar('0'))
                           .arg(lm, 2, 10, QChar('0'))
                           .arg(ls, 2, 10, QChar('0')));
}


/*! \brief Extract sample rate and offset frequency from file name */
void CIqTool::parseFileName(const QString &filename)
{
    bool   sr_ok;
    qint64 sr;
    bool   center_ok;
    qint64 center;

    QStringList list = filename.split('_');

    if (list.size() < 5)
        return;

    // gqrx_yymmdd_hhmmss_freq_samprate_fc.raw
    sr = list.at(4).toLongLong(&sr_ok);
    center = list.at(3).toLongLong(&center_ok);

    if (sr_ok)
        sample_rate = sr;
    if (center_ok)
        center_freq = center;
}
