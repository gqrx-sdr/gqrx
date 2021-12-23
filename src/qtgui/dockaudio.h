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
#ifndef DOCKAUDIO_H
#define DOCKAUDIO_H

#include <memory>

#include <QColor>
#include <QFrame>
#include <QSettings>
#include <QShortcut>

#include "audio_options.h"

namespace Ui {
    class DockAudio;
}


/*! \brief Dock window with audio options.
 *  \ingroup UI
 *
 * This dock widget encapsulates the audio options.
 * The UI itself is in the dockaudio.ui file.
 *
 * This class also provides the signal/slot API necessary to connect
 * the encapsulated widgets to the rest of the application.
 */
class DockAudio : public QFrame
{
    Q_OBJECT

public:
    explicit DockAudio(QWidget *parent = 0);
    ~DockAudio();

    void setFftRange(quint64 minf, quint64 maxf);
    void setNewFftData(float *fftData, int size);
    void setInvertScrolling(bool enabled);
    int  fftRate() const { return 10; }

    void setAudioGain(int gain);
    int  audioGain();

    void setAudioStreamButtonState(bool checked);
    void setAudioRecButtonState(bool checked);
    void setAudioPlayButtonState(bool checked);

    void setFftColor(QColor color);
    void setFftFill(bool enabled);

    void setupShortcuts(const size_t idx);
    void removeShortcuts();

    void saveSettings(std::shared_ptr<QSettings> settings, size_t idx);
    void readSettings(std::shared_ptr<QSettings> settings, size_t idx);

public slots:
    void startAudioRecorder(void);
    void stopAudioRecorder(void);
    void setRxFrequency(qint64 freq);
    void setWfColormap(const QString &cmap);

signals:
    /*! \brief Signal emitted when audio gain has changed. Gain is in dB. */
    void audioGainChanged(float gain);

    /*! \brief Audio streaming over UDP has started. */
    void audioStreamingStarted(const QString host, int port, bool stereo);

    /*! \brief Audio streaming stopped. */
    void audioStreamingStopped();

    /*! \brief Signal emitted when audio recording is started. */
    void audioRecStarted(const QString filename);

    /*! \brief Signal emitted when audio recording is stopped. */
    void audioRecStopped();

    /*! \brief Signal emitted when audio playback is started. */
    void audioPlayStarted(const QString filename);

    /*! \brief Signal emitted when audio playback is stopped. */
    void audioPlayStopped();

    /*! \brief FFT rate changed. */
    void fftRateChanged(int fps);

private slots:
    void on_audioGainSlider_valueChanged(int value);
    void on_audioStreamButton_clicked(bool checked);
    void on_audioRecButton_clicked(bool checked);
    void on_audioPlayButton_clicked(bool checked);
    void on_audioConfButton_clicked();
    void on_audioMuteButton_clicked(bool checked);
    void setNewPandapterRange(int min, int max);
    void setNewWaterfallRange(int min, int max);
    void setNewRecDir(const QString &dir);
    void setNewUdpHost(const QString &host);
    void setNewUdpPort(int port);
    void setNewUdpStereo(bool enabled);


private:
    Ui::DockAudio *ui;
    CAudioOptions *audioOptions; /*! Audio options dialog. */
    QString        rec_dir;      /*! Location for audio recordings. */
    QString        last_audio;   /*! Last audio recording. */

    QString        udp_host;     /*! UDP client host name. */
    int            udp_port;     /*! UDP client port number. */
    bool           udp_stereo;   /*! Enable stereo streaming for UDP. */
    bool           muted;        /*! Is the audio currently muted? */

    bool           autoSpan;     /*! Whether to allow mode-dependent auto span. */

    qint64         rx_freq;      /*! RX frequency used in filenames. */

    QList<QMetaObject::Connection> shortcutConnections;
    QList<QShortcut*> shortcuts;

    void           recordToggleShortcut();
    void           muteToggleShortcut();
    void           increaseAudioGainShortcut();
    void           decreaseAudioGainShortcut();
};

#endif // DOCKAUDIO_H
