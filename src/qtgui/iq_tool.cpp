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
    rec_bytes_per_sample = 8;
    fmt = receiver::FILE_FORMAT_CF;
    rec_fmt = receiver::FILE_FORMAT_CF;
    sample_rate = 192000;
    rec_len = 0;
    center_freq = 1e8;

    recdir = new QDir(QDir::homePath(), "*.raw");
    recdir->setNameFilters(recdir->nameFilters() << "*.sigmf-data");

    error_palette = new QPalette();
    error_palette->setColor(QPalette::Text, Qt::red);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutFunction()));
    connect(ui->formatCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(on_formatCombo_currentIndexChanged(int)));
    ui->formatCombo->addItem("gr_complex cf", receiver::FILE_FORMAT_CF);
    ui->formatCombo->addItem("int 32", receiver::FILE_FORMAT_CS32L);
    ui->formatCombo->addItem("short 16", receiver::FILE_FORMAT_CS16L);
    ui->formatCombo->addItem("char 8", receiver::FILE_FORMAT_CS8);
    ui->formatCombo->addItem("uint 32", receiver::FILE_FORMAT_CS32LU);
    ui->formatCombo->addItem("ushort 16", receiver::FILE_FORMAT_CS16LU);
    ui->formatCombo->addItem("uchar 8", receiver::FILE_FORMAT_CS8U);
    ui->formatCombo->addItem("SIGMF", receiver::FILE_FORMAT_SIGMF);

    ui->bufferStats->hide();
    ui->sizeStats->hide();
}

CIqTool::~CIqTool()
{
    timer->stop();
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

/*! \brief Show/hide/enable/disable GUI controls */

void CIqTool::switchControlsState(bool recording, bool playback)
{
    ui->recButton->setEnabled(!playback);

    ui->playButton->setEnabled(!recording);
    ui->slider->setEnabled(!recording);

    ui->repeat->setEnabled(!(recording || playback));
    ui->listWidget->setEnabled(!(recording || playback));
    ui->recDirEdit->setEnabled(!(recording || playback));
    ui->recDirButton->setEnabled(!(recording || playback));
    ui->formatCombo->setEnabled(!(recording || playback));
    ui->repeat->setEnabled(!(recording || playback));
    ui->buffersSpinBox->setEnabled(!(recording || playback));
    if (recording || playback)
    {
        ui->formatLabel->hide();
        ui->buffersLabel->hide();
        ui->formatCombo->hide();
        ui->buffersSpinBox->hide();
        ui->bufferStats->show();
        ui->sizeStats->show();
    }
    else
    {
        ui->formatLabel->show();
        ui->buffersLabel->show();
        ui->formatCombo->show();
        ui->buffersSpinBox->show();
        ui->bufferStats->hide();
        ui->sizeStats->hide();
    }
}

/*! \brief Start/stop playback */
void CIqTool::on_playButton_clicked(bool checked)
{
    is_playing = checked;

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
            on_listWidget_currentTextChanged(current_file);
            switchControlsState(false, true);

            emit startPlayback(recdir->absoluteFilePath(current_file),
                               (float)sample_rate, center_freq, fmt,
                               ui->buffersSpinBox->value(),
                               ui->repeat->checkState() == Qt::Checked);
        }
    }
    else
    {
        emit stopPlayback();
        switchControlsState(false, false);
        ui->slider->setValue(0);
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
    switchControlsState(false, false);
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
    is_recording = checked;

    if (checked)
    {
        switchControlsState(true, false);
        emit startRecording(recdir->path(), rec_fmt, ui->buffersSpinBox->value());

        refreshDir();
        ui->listWidget->setCurrentRow(ui->listWidget->count()-1);
    }
    else
    {
        switchControlsState(false, false);
        emit stopRecording();
    }
}

/*! Public slot to start IQ recording by external events (e.g. remote control).
 *
 * If a recording is already in progress we ignore the event.
 */
void CIqTool::startIqRecorder(void)
{
    if (ui->recButton->isChecked())
    {
        qDebug() << __func__ << "An IQ recording is already in progress";
        return;
    }

    // emulate a button click
    ui->recButton->click();
}

/*! Public slot to stop IQ recording by external events (e.g. remote control).
 *
 * The event is ignored if no recording is in progress.
 */
void CIqTool::stopIqRecorder(void)
{
    if (ui->recButton->isChecked())
        ui->recButton->click(); // emulate a button click
    else
        qDebug() << __func__ << "No IQ recording in progress";
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
    ui->recButton->setChecked(false);
    on_recButton_clicked(false);
}

/*! \brief Update GUI ta match current recorder state.
 *
 * This slot can be periodically activated to show recording progress
 */
void CIqTool::updateStats(bool hasFailed, int buffersUsed, size_t fileSize)
{
    if(is_recording)
    {
        if (hasFailed)
        {
            QMessageBox msg_box;
            msg_box.setIcon(QMessageBox::Critical);
            msg_box.setText(tr("IQ recording failed."));
            msg_box.exec();
        }
        else
        {
            if(o_buffersUsed!=buffersUsed)
                ui->bufferStats->setText(QString("Buffer: %1%").arg(buffersUsed));
            if(o_fileSize != fileSize)
                ui->sizeStats->setText(QString("Size: %1 bytes").arg(fileSize));
            o_buffersUsed = buffersUsed;
            o_fileSize = fileSize;
        }
    }
    if(is_playing)
    {
        if(o_buffersUsed!=buffersUsed)
            ui->bufferStats->setText(QString("Buffer: %1%").arg(buffersUsed));
        if(o_fileSize != fileSize)
            ui->sizeStats->setText(QString("Pos: %1 bytes").arg(fileSize));
        o_buffersUsed = buffersUsed;
        o_fileSize = fileSize;
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
    timer->stop();
    hide();
    event->ignore();
}

/*! \brief Catch window show events. */
void CIqTool::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
    refreshDir();
    refreshTimeWidgets();
    timer->start(1000);
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
    settings->setValue("baseband/rec_fmt", rec_fmt);
    settings->setValue("baseband/rec_buffers", ui->buffersSpinBox->value());
}

void CIqTool::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    // Location of baseband recordings
    QString dir = settings->value("baseband/rec_dir", QDir::homePath()).toString();
    ui->recDirEdit->setText(dir);
    int found = ui->formatCombo->findData(settings->value("baseband/rec_fmt", receiver::FILE_FORMAT_CF));
    if(found == -1)
    {
        rec_fmt = receiver::FILE_FORMAT_CF;
        rec_bytes_per_sample = 8;
    }
    else
    {
        rec_bytes_per_sample = receiver::sample_size_from_format((enum receiver::file_formats)ui->formatCombo->itemData(found).toInt());
        ui->formatCombo->setCurrentIndex(found);
    }
    ui->buffersSpinBox->setValue(settings->value("baseband/rec_buffers", 1).toInt());
}



/*! \brief Slot called when the recordings directory has changed either
 *         because of user input or programmatically.
 */
void CIqTool::on_recDirEdit_textChanged(const QString &dir)
{
    if (recdir->exists(dir))
    {
        ui->recDirEdit->setPalette(QPalette());  // Clear custom color
        recdir->setPath(dir);
        recdir->cd(dir);
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
    refreshDir();

    if (is_playing)
    {
        int val = o_fileSize / (sample_rate * bytes_per_sample);
        if (val < ui->slider->maximum())
        {
            ui->slider->blockSignals(true);
            ui->slider->setValue(val);
            ui->slider->blockSignals(false);
            refreshTimeWidgets();
        }
    }
    if (is_recording)
        refreshTimeWidgets();
}

void CIqTool::on_formatCombo_currentIndexChanged(int index)
{
    rec_fmt = (enum receiver::file_formats)ui->formatCombo->currentData().toInt();
    rec_bytes_per_sample = receiver::sample_size_from_format(rec_fmt);
}

/*! \brief Refresh list of files in current working directory. */
void CIqTool::refreshDir()
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

    QString fmt_str = "";
    QStringList list = filename.split('_');

    if (list.size() < 5)
        return;

    // gqrx_yymmdd_hhmmss_freq_samprate_fc.raw
    sr = list.at(4).toLongLong(&sr_ok);
    center = list.at(3).toLongLong(&center_ok);

    fmt_str = list.at(5);
    list = fmt_str.split('.');
    fmt_str = list.at(0);

    if (sr_ok)
        sample_rate = sr;
    if (center_ok)
        center_freq = center;
    if(fmt_str.compare("fc") == 0)
    {
        bytes_per_sample = 8;
        fmt = receiver::FILE_FORMAT_CF;
        if(list.at(1).compare("sigmf-data") == 0)
            fmt = receiver::FILE_FORMAT_SIGMF;
    }
    if(fmt_str.compare("32") == 0)
    {
        bytes_per_sample = 8;
        fmt = receiver::FILE_FORMAT_CS32L;
    }
    if(fmt_str.compare("16") == 0)
    {
        bytes_per_sample = 4;
        fmt = receiver::FILE_FORMAT_CS16L;
    }
    if(fmt_str.compare("8") == 0)
    {
        bytes_per_sample = 2;
        fmt = receiver::FILE_FORMAT_CS8;
    }
    if(fmt_str.compare("32u") == 0)
    {
        bytes_per_sample = 8;
        fmt = receiver::FILE_FORMAT_CS32LU;
    }
    if(fmt_str.compare("16u") == 0)
    {
        bytes_per_sample = 4;
        fmt = receiver::FILE_FORMAT_CS16LU;
    }
    if(fmt_str.compare("8u") == 0)
    {
        bytes_per_sample = 2;
        fmt = receiver::FILE_FORMAT_CS8U;
    }
}
