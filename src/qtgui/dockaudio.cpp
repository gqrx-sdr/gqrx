/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
 *
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
#include <QDir>
#include "dockaudio.h"
#include "ui_dockaudio.h"

DockAudio::DockAudio(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAudio),
    autoSpan(true),
    rx_freq(144000000)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    // Workaround for Mac, see http://stackoverflow.com/questions/3978889/why-is-qhboxlayout-causing-widgets-to-overlap
    // Might be fixed in Qt 5?
    ui->audioPlayButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->audioRecButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui->audioConfButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

#ifdef Q_OS_LINUX
    // buttons can be smaller than 50x32
    ui->audioStreamButton->setMinimumSize(48, 24);
    ui->audioRecButton->setMinimumSize(48, 24);
    ui->audioPlayButton->setMinimumSize(48, 24);
    ui->audioConfButton->setMinimumSize(48, 24);
#endif

    audioOptions = new CAudioOptions(this);

    connect(audioOptions, SIGNAL(newRecDirSelected(QString)), this, SLOT(setNewRecDir(QString)));
    connect(audioOptions, SIGNAL(newUdpHost(QString)), this, SLOT(setNewUdpHost(QString)));
    connect(audioOptions, SIGNAL(newUdpPort(int)), this, SLOT(setNewUdpPort(int)));

    ui->audioSpectrum->setPercent2DScreen(100);
    ui->audioSpectrum->setFreqUnits(1000);
    ui->audioSpectrum->setSampleRate(48000);  // Full bandwidth
    ui->audioSpectrum->setSpanFreq(12000);
    ui->audioSpectrum->setCenterFreq(0);
    ui->audioSpectrum->setFftCenterFreq(6000);
    ui->audioSpectrum->setDemodCenterFreq(0);
    ui->audioSpectrum->setFilterBoxEnabled(false);
    ui->audioSpectrum->setCenterLineEnabled(false);
    ui->audioSpectrum->setMinMaxDB(-110, -20);
    ui->audioSpectrum->setVdivDelta(30);
    ui->audioSpectrum->setHdivDelta(40);
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

void DockAudio::setNewFttData(float *fftData, int size)
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

/*! Set FFT plot color. */
void DockAudio::setFftColor(QColor color)
{
    ui->audioSpectrum->setFftPlotColor(color);
}

/*! Enable/disable filling area under FFT plot. */
void DockAudio::setFftFill(bool enabled)
{
    ui->audioSpectrum->setFftFill(enabled);
}

/*! Public slot to trig audio recording by external events (e.g. satellite AOS).
 *
 * If a recording is already in progress we ignore the event.
 */
void DockAudio::startAudioRecorder(void)
{
    if (ui->audioRecButton->isChecked())
    {
        qDebug() << __func__ << "An audio recording is already in progress";
        return;
    }

    // emulate a button click
    ui->audioRecButton->click();
}

/*! Public slot to stop audio recording by external events (e.g. satellite LOS).
 *
 * The event is ignored if no audio recording is in progress.
 */
void DockAudio::stopAudioRecorder(void)
{
    if (ui->audioRecButton->isChecked())
        ui->audioRecButton->click(); // emulate a button click
    else
        qDebug() << __func__ << "No audio recording in progress";
}

/*! Public slot to set new RX frequency in Hz. */
void DockAudio::setRxFrequency(qint64 freq)
{
    rx_freq = freq;
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

/*! \brief Streaming button clicked.
 *  \param checked Whether streaming is ON or OFF.
 */
void DockAudio::on_audioStreamButton_clicked(bool checked)
{
    if (checked)
        emit audioStreamingStarted(udp_host, udp_port);
    else
        emit audioStreamingStopped();
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
        QString file_name = QDateTime::currentDateTime().toUTC().toString("gqrx_yyyyMMdd_hhmmss");
        last_audio = QString("%1/%2_%3.wav").arg(rec_dir).arg(file_name).arg(rx_freq);

        // emit signal and start timer
        emit audioRecStarted(last_audio);

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
        emit audioPlayStarted(last_audio);

        ui->audioRecLabel->setText(QFileInfo(last_audio).fileName());
        ui->audioPlayButton->setToolTip(tr("Stop audio playback"));
        ui->audioRecButton->setEnabled(false); // prevent recording while we play
    }
    else {
        ui->audioRecLabel->setText("<i>DSP</i>");
        ui->audioPlayButton->setToolTip(tr("Start playback of last recorded audio file"));
        emit audioPlayStopped();

        ui->audioRecButton->setEnabled(true);
    }
}

/*! \brief Configure button clicked. */
void DockAudio::on_audioConfButton_clicked()
{
    audioOptions->show();
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

    if (rec_dir != QDir::homePath())
        settings->setValue("audio/rec_dir", rec_dir);
    else
        settings->remove("audio/rec_dir");

    if ((udp_host != "localhost") && (udp_host != "127.0.0.1"))
        settings->setValue("audio/udp_host", udp_host);
    else
        settings->remove("audio/udp_host");

    if (udp_port != 7355)
        settings->setValue("audio/udp_port", udp_port);
    else
        settings->remove("audio/udp_port");
}

void DockAudio::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    bool conv_ok = false;

    int gain = settings->value("audio/gain", QVariant(-200)).toInt(&conv_ok);
    if (conv_ok)
        setAudioGain(gain);

    // Location of audio recordings
    rec_dir = settings->value("audio/rec_dir", QDir::homePath()).toString();
    audioOptions->setRecDir(rec_dir);

    // Audio streaming host and port
    udp_host = settings->value("audio/udp_host", "localhost").toString();
    udp_port = settings->value("audio/udp_port", 7355).toInt(&conv_ok);
    if (!conv_ok)
        udp_port = 7355;

    audioOptions->setUdpHost(udp_host);
    audioOptions->setUdpPort(udp_port);
}

/*! \brief Slot called when a new valid recording directory has been selected
 *         in the audio conf dialog.
 */
void DockAudio::setNewRecDir(const QString &dir)
{
    rec_dir = dir;
}

/*! \brief Slot called when a new network host has been entered. */
void DockAudio::setNewUdpHost(const QString &host)
{
    if (host.isEmpty())
        udp_host = "localhost";
    else
        udp_host = host;
}

/*! \brief Slot called when a new network port has been entered. */
void DockAudio::setNewUdpPort(int port)
{
    udp_port = port;
}
