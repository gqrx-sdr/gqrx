/* -*- c++ -*- */
/*
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
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
#include <QDateTime>
#include "dockaudio.h"
#include "ui_dockaudio.h"

DockAudio::DockAudio(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAudio),
    autoSpan(true)
{
    ui->setupUi(this);

#ifdef Q_WS_MAC
    // Workaround for Mac, see http://stackoverflow.com/questions/3978889/why-is-qhboxlayout-causing-widgets-to-overlap
    // Might be fixed in Qt 5?
    ui->audioPlayButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->audioRecButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->audioRecConfButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

    ui->audioSpectrum->setPercent2DScreen(100);
    ui->audioSpectrum->setFreqUnits(1000);
    ui->audioSpectrum->setSampleRate(48000);  // Full bandwidth
    ui->audioSpectrum->setSpanFreq(12000);
    ui->audioSpectrum->setCenterFreq(0);
    ui->audioSpectrum->setFftCenterFreq(6000);
    ui->audioSpectrum->setDemodCenterFreq(0);
    ui->audioSpectrum->setFilterBoxEnabled(false);
    ui->audioSpectrum->setCenterLineEnabled(false);
    ui->audioSpectrum->setMinMaxDB(-120, 0);
    ui->audioSpectrum->setFontSize(8);
    ui->audioSpectrum->setVdivDelta(20);
    ui->audioSpectrum->setHdivDelta(25);
    ui->audioSpectrum->setFreqDigits(1);
}

DockAudio::~DockAudio()
{
    delete ui;
}

void DockAudio::setFftRange(quint64 minf, quint64 maxf)
{
    if (autoSpan)
    {
        qint32 span = (qint32)(maxf - minf);
        quint64 fc = minf + (maxf - minf)/2;

        ui->audioSpectrum->setFftCenterFreq(fc);
        ui->audioSpectrum->setSpanFreq(span);
        ui->audioSpectrum->setCenterFreq(0);
    }
}

void DockAudio::setNewFttData(double *fftData, int size)
{
    ui->audioSpectrum->setNewFttData(fftData, size);
}

/*! \brief Set new audio gain.
 *  \param gain the new audio gain in tens of dB (0 dB = 10)
 */
void DockAudio::setAudioGain(int gain)
{
    ui->audioGainSlider->setValue(gain);
}


/*! \brief Get current audio gain.
 *  \returns The current audio gain in tens of dB (0 dB = 10).
 */
int  DockAudio::audioGain()
{
    return ui->audioGainSlider->value();
}


/*! \brief Audio gain changed.
 *  \param value The new audio gain value in tens of dB (because slider uses int)
 */
void DockAudio::on_audioGainSlider_valueChanged(int value)
{
    float gain = float(value) / 10.0;

    // update dB label
    ui->audioGainDbLabel->setText(QString("%1 dB").arg(gain));
    emit audioGainChanged(gain);
}


/*! \brief Record button clicked.
 *  \param checked Whether recording is ON or OFF.
 *
 * We use the clicked signal instead of the toggled which allows us to change the
 * state programatically using toggle() without triggering the signal.
 */
void DockAudio::on_audioRecButton_clicked(bool checked)
{
    if (checked) {
        // FIXME: option to use local time
        // use toUTC() function compatible with older versions of Qt.
        lastAudio = QDateTime::currentDateTime().toUTC().toString("gqrx-yyyyMMdd-hhmmss.'wav'");

        // emit signal and start timer
        emit audioRecStarted(lastAudio);

        ui->audioRecButton->setToolTip(tr("Stop audio recorder"));
        ui->audioPlayButton->setEnabled(false); /* prevent playback while recording */
    }
    else {
        ui->audioRecButton->setToolTip(tr("Start audio recorder"));
        emit audioRecStopped();

        ui->audioPlayButton->setEnabled(true);
    }
}

/*! \brief Playback button clicked.
 *  \param checked Whether playback is ON or OFF.
 *
 * We use the clicked signal instead of the toggled which allows us to change the
 * state programatically using toggle() without triggering the signal.
 */
void DockAudio::on_audioPlayButton_clicked(bool checked)
{
    if (checked) {

        // emit signal and start timer
        emit audioPlayStarted(lastAudio);

        ui->audioPlayButton->setToolTip(tr("Stop audio playback"));
        ui->audioRecButton->setEnabled(false); // prevent recording while we play
    }
    else {
        ui->audioPlayButton->setToolTip(tr("Start playback of last recorded audio file"));
        emit audioPlayStopped();

        ui->audioRecButton->setEnabled(true);
    }
}


/*! \brief Set status of audio record button. */
void DockAudio::setAudioRecButtonState(bool checked)
{
    if (checked == ui->audioRecButton->isChecked()) {
        /* nothing to do */
        return;
    }

    // toggle the button and set the state of the other buttons accordingly
    ui->audioRecButton->toggle();
    bool isChecked = ui->audioRecButton->isChecked();

    ui->audioRecButton->setToolTip(isChecked ? tr("Stop audio recorder") : tr("Start audio recorder"));
    ui->audioPlayButton->setEnabled(!isChecked);
    //ui->audioRecConfButton->setEnabled(!isChecked);
}

/*! \brief Set status of audio record button. */
void DockAudio::setAudioPlayButtonState(bool checked)
{
    if (checked == ui->audioPlayButton->isChecked()) {
        // nothing to do
        return;
    }

    // toggle the button and set the state of the other buttons accordingly
    ui->audioPlayButton->toggle();
    bool isChecked = ui->audioPlayButton->isChecked();

    ui->audioPlayButton->setToolTip(isChecked ? tr("Stop audio playback") : tr("Start playback of last recorded audio file"));
    ui->audioRecButton->setEnabled(!isChecked);
    //ui->audioRecConfButton->setEnabled(!isChecked);
}

void DockAudio::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->setValue("audio/gain", audioGain());
}

void DockAudio::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    bool conv_ok = false;

    int gain = settings->value("audio/gain", QVariant( -200 ) ).toInt(&conv_ok);
    if (conv_ok) {
        setAudioGain(gain);
    }
}
