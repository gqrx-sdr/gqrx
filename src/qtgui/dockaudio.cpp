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
#include <QDir>
#include "dockaudio.h"
#include "ui_dockaudio.h"

#define DEFAULT_FFT_SPLIT 100

DockAudio::DockAudio(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::DockAudio),
    autoSpan(true),
    rx_freq(144000000)
{
    ui->setupUi(this);

#ifdef Q_OS_LINUX
    // buttons can be smaller than 50x32
    ui->audioMuteButton->setMinimumSize(48, 24);
    ui->audioStreamButton->setMinimumSize(48, 24);
    ui->audioRecButton->setMinimumSize(48, 24);
    ui->audioPlayButton->setMinimumSize(48, 24);
    ui->audioConfButton->setMinimumSize(48, 24);
#endif

    audioOptions = new CAudioOptions(this);

    connect(audioOptions, SIGNAL(newFftSplit(int)), ui->audioSpectrum, SLOT(setPercent2DScreen(int)));
    connect(audioOptions, SIGNAL(newPandapterRange(int,int)), this, SLOT(setNewPandapterRange(int,int)));
    connect(audioOptions, SIGNAL(newWaterfallRange(int,int)), this, SLOT(setNewWaterfallRange(int,int)));
    connect(audioOptions, SIGNAL(newRecDirSelected(QString)), this, SLOT(setNewRecDir(QString)));
    connect(audioOptions, SIGNAL(newUdpHost(QString)), this, SLOT(setNewUdpHost(QString)));
    connect(audioOptions, SIGNAL(newUdpPort(int)), this, SLOT(setNewUdpPort(int)));
    connect(audioOptions, SIGNAL(newUdpStereo(bool)), this, SLOT(setNewUdpStereo(bool)));

    connect(ui->audioSpectrum, SIGNAL(pandapterRangeChanged(float,float)), audioOptions, SLOT(setPandapterSliderValues(float,float)));

    ui->audioSpectrum->setFreqUnits(1000);
    ui->audioSpectrum->setSampleRate(48000);  // Full bandwidth
    ui->audioSpectrum->setSpanFreq(12000);
    ui->audioSpectrum->setCenterFreq(0);
    ui->audioSpectrum->setPercent2DScreen(DEFAULT_FFT_SPLIT);
    ui->audioSpectrum->setFftCenterFreq(6000);
    ui->audioSpectrum->setFilterBoxEnabled(false);
    ui->audioSpectrum->setCenterLineEnabled(false);
    ui->audioSpectrum->setBookmarksEnabled(false);
    ui->audioSpectrum->setBandPlanEnabled(false);
    ui->audioSpectrum->setFftRange(-80., 0.);
    ui->audioSpectrum->setVdivDelta(40);
    ui->audioSpectrum->setHdivDelta(40);
    ui->audioSpectrum->setFreqDigits(1);
}

DockAudio::~DockAudio()
{
    removeShortcuts();

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

void DockAudio::setupShortcuts(const size_t idx)
{
    // Remove any existing shortcuts
    removeShortcuts();

    // pre-select demod with a key combo
    int dkey = -1;
    switch (idx) {
    case 0:
        dkey = Qt::CTRL + Qt::Key_1;
        break;
    case 1:
        dkey = Qt::CTRL + Qt::Key_2;
        break;
    case 2:
        dkey = Qt::CTRL + Qt::Key_3;
        break;
    case 3:
        dkey = Qt::CTRL + Qt::Key_4;
        break;
    case 4:
        dkey = Qt::CTRL + Qt::Key_5;
        break;
    case 5:
        dkey = Qt::CTRL + Qt::Key_6;
        break;
    case 6:
        dkey = Qt::CTRL + Qt::Key_7;
        break;
    case 7:
        dkey = Qt::CTRL + Qt::Key_8;
        break;
    case 8:
        dkey = Qt::CTRL + Qt::Key_9;
        break;
    case 9:
        dkey = Qt::CTRL + Qt::Key_0;
        break;
    }

    // Do not set up shortcuts if the demod cannot be pre-selected
    if (dkey < 0) {
        return;
    }

    QShortcut *rec_toggle_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_R), this);
    QShortcut *mute_toggle_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_M), this);
    QShortcut *audio_gain_increase_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_Plus), this);
    QShortcut *audio_gain_decrease_shortcut = new QShortcut(QKeySequence(dkey, Qt::Key_Minus), this);

    shortcutConnections.push_back(QObject::connect(rec_toggle_shortcut, &QShortcut::activated, this, &DockAudio::recordToggleShortcut));
    shortcutConnections.push_back(QObject::connect(mute_toggle_shortcut, &QShortcut::activated, this, &DockAudio::muteToggleShortcut));
    shortcutConnections.push_back(QObject::connect(audio_gain_increase_shortcut, &QShortcut::activated, this, &DockAudio::increaseAudioGainShortcut));
    shortcutConnections.push_back(QObject::connect(audio_gain_decrease_shortcut, &QShortcut::activated, this, &DockAudio::decreaseAudioGainShortcut));

    // Store all the shortcut pointers so we can remove them
    shortcuts.push_back(rec_toggle_shortcut);
    shortcuts.push_back(mute_toggle_shortcut);
    shortcuts.push_back(audio_gain_increase_shortcut);
    shortcuts.push_back(audio_gain_decrease_shortcut);
}

void DockAudio::removeShortcuts()
{
    for (int i = 0; i < shortcutConnections.size(); ++i)
    {
        disconnect(shortcutConnections[i]);
    }
    shortcutConnections.clear();
    for (int i = 0; i < shortcuts.size(); ++i)
    {
        delete shortcuts[i];
    }
    shortcuts.clear();
}

/*! Public slot to trig audio recording by external events (e.g. satellite AOS).
 *
 * If a recording is already in progress we ignore the event.
 */
void DockAudio::startAudioRecorder(void)
{
    if (ui->audioRecButton->isChecked())
    {
        qInfo() << __func__ << "An audio recording is already in progress";
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
        qInfo() << __func__ << "No audio recording in progress";
}

/*! Public slot to set new RX frequency in Hz. */
void DockAudio::setRxFrequency(qint64 freq)
{
    // qInfo() << "DockAudio::setRxFrequency =" << freq;
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
    float gain = float(value) / 10.0;

    // update dB label
    ui->audioGainDbLabel->setText(QString("%1 dB").arg(gain, 5, 'f', 1));
    if (!ui->audioMuteButton->isChecked())
        emit audioGainChanged(gain);
}

/*! \brief Streaming button clicked.
 *  \param checked Whether streaming is ON or OFF.
 */
void DockAudio::on_audioStreamButton_clicked(bool checked)
{
    if (checked)
        emit audioStreamingStarted(udp_host, udp_port, udp_stereo);
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
        // FIXME: option to use local time
        // use toUTC() function compatible with older versions of Qt.
        QString file_name = QDateTime::currentDateTime().toUTC().toString("gqrx_yyyyMMdd_hhmmss");
        last_audio = QString("%1/%2_%3.wav").arg(rec_dir).arg(file_name).arg(rx_freq);
        QFileInfo info(last_audio);

        // emit signal and start timer
        emit audioRecStarted(last_audio);

        ui->audioRecLabel->setText(info.fileName());
        ui->audioRecButton->setToolTip(tr("Stop audio recorder"));
        ui->audioPlayButton->setEnabled(false); /* prevent playback while recording */
    }
    else {
        ui->audioRecLabel->setText("<i>DSP</i>");
        ui->audioRecButton->setToolTip(tr("Start audio recorder"));
        emit audioRecStopped();

        ui->audioPlayButton->setEnabled(true);
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
    muted = checked;

    if (checked)
    {
        emit audioGainChanged(-INFINITY);
    }
    else
    {
        int value = ui->audioGainSlider->value();
        float gain = float(value) / 10.0;
        emit audioGainChanged(gain);
    }
}

void DockAudio::setAudioStreamButtonState(bool checked)
{
    ui->audioStreamButton->setChecked(checked);
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

void DockAudio::saveSettings(std::shared_ptr<QSettings> settings, size_t idx)
{
    int     ival, fft_min, fft_max;

    if (!settings)
        return;

    settings->beginGroup("audio");
    settings->beginGroup(QString("%0").arg(idx));

    settings->setValue("gain", audioGain());

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

    if (rec_dir != QDir::homePath())
        settings->setValue("rec_dir", rec_dir);
    else
        settings->remove("rec_dir");

    if (udp_host.isEmpty())
        settings->remove("udp_host");
    else
        settings->setValue("udp_host", udp_host);

    if (udp_port != 7355)
        settings->setValue("udp_port", udp_port);
    else
        settings->remove("udp_port");

    if (udp_stereo != false)
        settings->setValue("udp_stereo", udp_stereo);
    else
        settings->remove("udp_stereo");

    if (muted != false)
        settings->setValue("muted", muted);
    else
        settings->remove("muted");

    settings->endGroup(); // idx
    settings->endGroup(); // audio
}

void DockAudio::readSettings(std::shared_ptr<QSettings> settings, size_t idx)
{
    int     bool_val, ival, fft_min, fft_max;
    bool    conv_ok = false;

    if (!settings)
        return;

    auto configVersion = settings->value("configversion").toInt(&conv_ok);

    settings->beginGroup("audio");

    // Migrate v3 settings for 1st demod only
    if (configVersion < 4 && idx == 0)
    {
        QStringList v3Keys({
            "gain",
            "fft_split",
            "pandapter_min_db",
            "pandapter_max_db",
            "waterfall_min_db",
            "waterfall_max_db",
            "db_ranges_locked",
            "rec_dir",
            "udp_host",
            "udp_port",
            "udp_stereo",
            "muted",
        });
        for (auto &key : v3Keys)
        {
            if (settings->contains(key)) {
                settings->setValue("0/" + key, settings->value(key));
                settings->remove(key);
            }
        }
    }

    settings->beginGroup(QString("%0").arg(idx));

    ival = settings->value("gain", QVariant(-60)).toInt(&conv_ok);
    if (conv_ok)
        setAudioGain(ival);

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

    // Location of audio recordings
    rec_dir = settings->value("rec_dir", QDir::homePath()).toString();
    audioOptions->setRecDir(rec_dir);

    // Audio streaming host, port and stereo setting
    udp_host = settings->value("udp_host", "localhost").toString();
    udp_port = settings->value("udp_port", 7355 + (int)idx).toInt(&conv_ok);
    if (!conv_ok)
        udp_port = 7355 + idx;
    udp_stereo = settings->value("udp_stereo", false).toBool();

    audioOptions->setUdpHost(udp_host);
    audioOptions->setUdpPort(udp_port);
    audioOptions->setUdpStereo(udp_stereo);

    muted = settings->value("muted", false).toBool();
    ui->audioMuteButton->setChecked(muted);
    on_audioMuteButton_clicked(muted);

    settings->endGroup(); // idx
    settings->endGroup(); // audio
}

void DockAudio::setNewPandapterRange(int min, int max)
{
    ui->audioSpectrum->setPandapterRange(min, max);
}

void DockAudio::setNewWaterfallRange(int min, int max)
{
    ui->audioSpectrum->setWaterfallRange(min, max);
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

/*! \brief Slot called when the mono/stereo streaming setting changes. */
void DockAudio::setNewUdpStereo(bool enabled)
{
    udp_stereo = enabled;
}

void DockAudio::recordToggleShortcut() {
    ui->audioRecButton->click();
}

void DockAudio::muteToggleShortcut() {
    ui->audioMuteButton->click();
}

void DockAudio::increaseAudioGainShortcut() {
	ui->audioGainSlider->triggerAction(QSlider::SliderPageStepAdd);
}

void DockAudio::decreaseAudioGainShortcut() {
	ui->audioGainSlider->triggerAction(QSlider::SliderPageStepSub);
}
