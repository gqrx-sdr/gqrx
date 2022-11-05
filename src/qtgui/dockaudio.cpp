/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#include <cmath>
#include <QDebug>
#include <QDateTime>
#include <QShortcut>
#include <QDir>
#include "dockaudio.h"
#include "ui_dockaudio.h"

#define DEFAULT_FFT_SPLIT 100

DockAudio::DockAudio(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAudio),
    autoSpan(true),
    rx_freq(144000000)
{
    ui->setupUi(this);

    audioOptions = new CAudioOptions(this);

    connect(audioOptions, SIGNAL(newFftSplit(int)), ui->audioSpectrum, SLOT(setPercent2DScreen(int)));
    connect(audioOptions, SIGNAL(newPandapterRange(int,int)), this, SLOT(pandapterRange_changed(int,int)));
    connect(audioOptions, SIGNAL(newWaterfallRange(int,int)), this, SLOT(waterfallRange_changed(int,int)));
    connect(audioOptions, SIGNAL(newRecDirSelected(QString)), this, SLOT(recDir_changed(QString)));
    connect(audioOptions, SIGNAL(newUdpHost(QString)), this, SLOT(udpHost_changed(QString)));
    connect(audioOptions, SIGNAL(newUdpPort(int)), this, SLOT(udpPort_changed(int)));
    connect(audioOptions, SIGNAL(newUdpStereo(bool)), this, SLOT(udpStereo_changed(bool)));
    connect(audioOptions, SIGNAL(newSquelchTriggered(bool)), this, SLOT(squelchTriggered_changed(bool)));
    connect(audioOptions, SIGNAL(newRecMinTime(int)), this, SLOT(recMinTime_changed(int)));
    connect(audioOptions, SIGNAL(newRecMaxGap(int)), this, SLOT(recMaxGap_changed(int)));
    connect(audioOptions, SIGNAL(copyRecSettingsToAllVFOs()), this, SLOT(copyRecSettingsToAllVFOs_clicked()));

    connect(ui->audioSpectrum, SIGNAL(pandapterRangeChanged(float,float)), audioOptions, SLOT(setPandapterSliderValues(float,float)));

    ui->audioSpectrum->setFreqUnits(1000);
    ui->audioSpectrum->setSampleRate(48000);  // Full bandwidth
    ui->audioSpectrum->setSpanFreq(12000);
    ui->audioSpectrum->setCenterFreq(0);
    ui->audioSpectrum->setPercent2DScreen(DEFAULT_FFT_SPLIT);
    ui->audioSpectrum->setFftCenterFreq(6000);
    ui->audioSpectrum->setDemodCenterFreq(0);
    ui->audioSpectrum->setFilterBoxEnabled(false);
    ui->audioSpectrum->setCenterLineEnabled(false);
    ui->audioSpectrum->setBookmarksEnabled(false);
    ui->audioSpectrum->enableBandPlan(false);
    ui->audioSpectrum->setFftRange(-80., 0.);
    ui->audioSpectrum->setVdivDelta(40);
    ui->audioSpectrum->setFreqDigits(1);
    ui->audioSpectrum->setRunningState(true);

    QShortcut *rec_toggle_shortcut = new QShortcut(QKeySequence(Qt::Key_R), this);
    QShortcut *mute_toggle_shortcut = new QShortcut(QKeySequence(Qt::Key_M), this);
    QShortcut *audio_gain_increase_shortcut1 = new QShortcut(QKeySequence(Qt::Key_Plus), this);
    QShortcut *audio_gain_decrease_shortcut1 = new QShortcut(QKeySequence(Qt::Key_Minus), this);

    QObject::connect(rec_toggle_shortcut, &QShortcut::activated, this, &DockAudio::recordToggleShortcut);
    QObject::connect(mute_toggle_shortcut, &QShortcut::activated, this, &DockAudio::muteToggleShortcut);
    QObject::connect(audio_gain_increase_shortcut1, &QShortcut::activated, this, &DockAudio::increaseAudioGainShortcut);
    QObject::connect(audio_gain_decrease_shortcut1, &QShortcut::activated, this, &DockAudio::decreaseAudioGainShortcut);
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

void DockAudio::setNewFftData(float *fftData, int size)
{
    ui->audioSpectrum->setNewFftData(fftData, size);
}

void DockAudio::setInvertScrolling(bool enabled)
{
    ui->audioSpectrum->setInvertScrolling(enabled);
}

/*! \brief Set new audio gain.
 *  \param gain the new audio gain in tens of dB (0 dB = 10)
 */
void DockAudio::setAudioGain(int gain)
{
    ui->audioGainSlider->setValue(gain);
}

/*! \brief Set audio muted
 *  \param muted true if audio should be muted
 */
void DockAudio::setAudioMuted(bool muted)
{
    ui->audioMuteButton->setChecked(!muted);
    ui->audioMuteButton->click();
}

/*! \brief Get current audio gain.
 *  \returns The current audio gain in tens of dB (0 dB = 10).
 */
int  DockAudio::audioGain()
{
    return ui->audioGainSlider->value();
}

/*! \brief Set audio gain slider state.
 *  \param state new slider state.
 */
void DockAudio::setGainEnabled(bool state)
{
    ui->audioGainSlider->setEnabled(state);
}

/*! Set FFT plot color. */
void DockAudio::setFftColor(QColor color)
{
    ui->audioSpectrum->setFftPlotColor(color);
}

/*! Enable/disable filling area under FFT plot. */
void DockAudio::setFftFill(bool enabled)
{
    ui->audioSpectrum->enableFftFill(enabled);
}

bool DockAudio::getSquelchTriggered()
{
    return squelch_triggered;
}

void DockAudio::setSquelchTriggered(bool mode)
{
    squelch_triggered = mode;
    audioOptions->setSquelchTriggered(mode);
}

void DockAudio::setRecDir(const QString &dir)
{
    rec_dir = dir;
    audioOptions->setRecDir(dir);
}

void DockAudio::setRecMinTime(int time_ms)
{
    recMinTime = time_ms;
    audioOptions->setRecMinTime(time_ms);
}

void DockAudio::setRecMaxGap(int time_ms)
{
    recMaxGap = time_ms;
    audioOptions->setRecMaxGap(time_ms);
}

/*! Public slot to set new RX frequency in Hz. */
void DockAudio::setRxFrequency(qint64 freq)
{
    rx_freq = freq;
}

void DockAudio::setWfColormap(const QString &cmap)
{
    ui->audioSpectrum->setWfColormap(cmap);
}

/*! \brief Audio gain changed.
 *  \param value The new audio gain value in tens of dB (because slider uses int)
 */
void DockAudio::on_audioGainSlider_valueChanged(int value)
{
    float gain = float(value) / 10.0f;

    // update dB label
    ui->audioGainDbLabel->setText(QString("%1 dB").arg((double)gain, 5, 'f', 1));
    emit audioGainChanged(gain);
}

/*! \brief Streaming button clicked.
 *  \param checked Whether streaming is ON or OFF.
 */
void DockAudio::on_audioStreamButton_clicked(bool checked)
{
    if (checked)
        emit audioStreamingStarted();
    else
        emit audioStreamingStopped();
}

/*! \brief Record button clicked.
 *  \param checked Whether recording is ON or OFF.
 *
 * We use the clicked signal instead of the toggled which allows us to change the
 * state programmatically using toggle() without triggering the signal.
 */
void DockAudio::on_audioRecButton_clicked(bool checked)
{
    if (checked) {

        // emit signal and start timer
        emit audioRecStart();
    }
    else {
        emit audioRecStop();
    }
}

/*! \brief Playback button clicked.
 *  \param checked Whether playback is ON or OFF.
 *
 * We use the clicked signal instead of the toggled which allows us to change the
 * state programmatically using toggle() without triggering the signal.
 */
void DockAudio::on_audioPlayButton_clicked(bool checked)
{
    if (checked) {
        QFileInfo info(last_audio);

        if(info.exists()) {
            // emit signal and start timer
            emit audioPlayStarted(last_audio);

            ui->audioRecLabel->setText(info.fileName());
            ui->audioPlayButton->setToolTip(tr("Stop audio playback"));
            ui->audioRecButton->setEnabled(false); // prevent recording while we play
        }
        else {
            ui->audioPlayButton->setChecked(false);
            ui->audioPlayButton->setEnabled(false);
        }
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

/*! \brief Mute audio. */
void DockAudio::on_audioMuteButton_clicked(bool checked)
{
    emit audioMuteChanged(checked);
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

void DockAudio::setAudioStreamState(const std::string & host,int port,bool stereo, bool running)
{
    audioOptions->setUdpHost(udp_host = QString::fromStdString(host));
    audioOptions->setUdpPort(udp_port = port);
    audioOptions->setUdpStereo(udp_stereo = stereo);
    setAudioStreamButtonState(running);
}

/*! \brief Set status of audio record button. */
void DockAudio::setAudioStreamButtonState(bool checked)
{
    if (checked == ui->audioStreamButton->isChecked()) {
        /* nothing to do */
        return;
    }

    // toggle the button and set the state of the other buttons accordingly
    ui->audioStreamButton->toggle();
    bool isChecked = ui->audioStreamButton->isChecked();

    ui->audioStreamButton->setToolTip(isChecked ? tr("Stop audio streaming") : tr("Start audio streaming"));
    //TODO: disable host/port controls
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
    int     ival, fft_min, fft_max;

    if (!settings)
        return;

    settings->beginGroup("audio");

    ival = audioOptions->getFftSplit();
    if (ival != DEFAULT_FFT_SPLIT)
        settings->setValue("fft_split", ival);
    else
        settings->remove("fft_split");

    audioOptions->getPandapterRange(&fft_min, &fft_max);
    if (fft_min != -80)
        settings->setValue("pandapter_min_db", fft_min);
    else
        settings->remove("pandapter_min_db");
    if (fft_max != 0)
        settings->setValue("pandapter_max_db", fft_max);
    else
        settings->remove("pandapter_max_db");

    audioOptions->getWaterfallRange(&fft_min, &fft_max);
    if (fft_min != -80)
        settings->setValue("waterfall_min_db", fft_min);
    else
        settings->remove("waterfall_min_db");
    if (fft_max != 0)
        settings->setValue("waterfall_max_db", fft_max);
    else
        settings->remove("waterfall_max_db");

    if (audioOptions->getLockButtonState())
        settings->setValue("db_ranges_locked", true);
    else
        settings->remove("db_ranges_locked");

    settings->endGroup();
}

void DockAudio::readSettings(QSettings *settings)
{
    int     bool_val, ival, fft_min, fft_max;
    bool    conv_ok = false;

    if (!settings)
        return;

    settings->beginGroup("audio");

    ival = settings->value("fft_split", DEFAULT_FFT_SPLIT).toInt(&conv_ok);
    if (conv_ok)
        audioOptions->setFftSplit(ival);

    fft_min = settings->value("pandapter_min_db", QVariant(-80)).toInt(&conv_ok);
    if (!conv_ok)
        fft_min = -80;
    fft_max = settings->value("pandapter_max_db", QVariant(0)).toInt(&conv_ok);
    if (!conv_ok)
        fft_max = 0;
    audioOptions->setPandapterRange(fft_min, fft_max);

    fft_min = settings->value("waterfall_min_db", QVariant(-80)).toInt(&conv_ok);
    if (!conv_ok)
        fft_min = -80;
    fft_max = settings->value("waterfall_max_db", QVariant(0)).toInt(&conv_ok);
    if (!conv_ok)
        fft_max = 0;
    audioOptions->setWaterfallRange(fft_min, fft_max);

    bool_val = settings->value("db_ranges_locked", false).toBool();
    audioOptions->setLockButtonState(bool_val);

    settings->endGroup();
}

void DockAudio::pandapterRange_changed(int min, int max)
{
    ui->audioSpectrum->setPandapterRange(min, max);
}

void DockAudio::waterfallRange_changed(int min, int max)
{
    ui->audioSpectrum->setWaterfallRange(min, max);
}

/*! \brief Slot called when a new valid recording directory has been selected
 *         in the audio conf dialog.
 */
void DockAudio::recDir_changed(const QString &dir)
{
    rec_dir = dir;
    emit recDirChanged(dir);
}

/*! \brief Slot called when a new network host has been entered. */
void DockAudio::udpHost_changed(const QString &host)
{
    if (host.isEmpty())
        udp_host = "localhost";
    else
        udp_host = host;
    emit udpHostChanged(udp_host);
}

/*! \brief Slot called when a new network port has been entered. */
void DockAudio::udpPort_changed(int port)
{
    udp_port = port;
    emit udpPortChanged(port);
}

/*! \brief Slot called when the mono/stereo streaming setting changes. */
void DockAudio::udpStereo_changed(bool enabled)
{
    udp_stereo = enabled;
    emit udpStereoChanged(enabled);
}

/*! \brief Slot called when audio recording is started after clicking rec or being triggered by squelch. */
void DockAudio::audioRecStarted(const QString filename)
{
    last_audio = filename;
    QFileInfo info(last_audio);
    ui->audioRecLabel->setText(info.fileName());
    ui->audioRecButton->setToolTip(tr("Stop audio recorder"));
    ui->audioPlayButton->setEnabled(false); /* prevent playback while recording */
    setAudioRecButtonState(true);
}

void DockAudio::audioRecStopped()
{
    ui->audioRecLabel->setText("<i>DSP</i>");
    ui->audioRecButton->setToolTip(tr("Start audio recorder"));
    ui->audioPlayButton->setEnabled(true);
    setAudioRecButtonState(false);
}


void DockAudio::squelchTriggered_changed(bool enabled)
{
    squelch_triggered = enabled;
    ui->audioRecButton->setStyleSheet(enabled?"color: rgb(255,0,0)":"");
    emit recSquelchTriggeredChanged(enabled);
}

void DockAudio::recMinTime_changed(int time_ms)
{
    recMinTime = time_ms;
    emit recMinTimeChanged(time_ms);
}

void DockAudio::recMaxGap_changed(int time_ms)
{
    recMaxGap = time_ms;
    emit recMaxGapChanged(time_ms);
}


void DockAudio::recordToggleShortcut() {
    ui->audioRecButton->click();
}

void DockAudio::muteToggleShortcut() {
    ui->audioMuteButton->click();
}

void DockAudio::increaseAudioGainShortcut() {
    if(ui->audioGainSlider->isEnabled())
        ui->audioGainSlider->triggerAction(QSlider::SliderPageStepAdd);
}

void DockAudio::decreaseAudioGainShortcut() {
    if(ui->audioGainSlider->isEnabled())
        ui->audioGainSlider->triggerAction(QSlider::SliderPageStepSub);
}

void DockAudio::copyRecSettingsToAllVFOs_clicked()
{
    emit copyRecSettingsToAllVFOs();
}
