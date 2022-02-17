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

#include <QColor>
#include <QDockWidget>
#include <QSettings>
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
class DockAudio : public QDockWidget
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
    void setGainEnabled(bool state);

    void setAudioRecButtonState(bool checked);
    void setAudioPlayButtonState(bool checked);

    void setFftColor(QColor color);
    void setFftFill(bool enabled);

    bool getSquelchTriggered();
    void setSquelchTriggered(bool mode);
    void setRecDir(const QString &dir);
    void setRecMinTime(int time_ms);
    void setRecMaxGap(int time_ms);



    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

public slots:
    void setRxFrequency(qint64 freq);
    void setWfColormap(const QString &cmap);
    void setAudioMuted(bool muted);
    void audioRecStarted(const QString filename);
    void audioRecStopped();

signals:
    /*! \brief Signal emitted when audio gain has changed. Gain is in dB. */
    void audioGainChanged(float gain);

    /*! \brief Audio streaming over UDP has started. */
    void audioStreamingStarted(const QString host, int port, bool stereo);

    /*! \brief Audio streaming stopped. */
    void audioStreamingStopped();

    /*! \brief Signal emitted when audio recording is started. */
    void audioRecStart();

    /*! \brief Signal emitted when audio recording is stopped. */
    void audioRecStop();

    /*! \brief Signal emitted when audio playback is started. */
    void audioPlayStarted(const QString filename);

    /*! \brief Signal emitted when audio playback is stopped. */
    void audioPlayStopped();

    /*! \brief FFT rate changed. */
    void fftRateChanged(int fps);
    
    /*! \brief Audio mute chenged. */
    void audioMuted(bool muted);

    /*! \brief Signal emitted when audio mute has changed. */
    void audioMuteChanged(bool mute);

    /*! \brief Signal emitted when recording directory has changed. */
    void recDirChanged(const QString dir);

    /*! \brief Signal emitted when squelch triggered recording mode is changed. */
    void recSquelchTriggeredChanged(const bool enabled);

    /*! \brief Signal emitted when squelch triggered recording min time is changed. */
    void recMinTimeChanged(int time_ms);

    /*! \brief Signal emitted when squelch triggered recording max gap time is changed. */
    void recMaxGapChanged(int time_ms);

    /*! \brief Signal emitted when toAllVFOs button is clicked. */
    void copyRecSettingsToAllVFOs();

private slots:
    void on_audioGainSlider_valueChanged(int value);
    void on_audioStreamButton_clicked(bool checked);
    void on_audioRecButton_clicked(bool checked);
    void on_audioPlayButton_clicked(bool checked);
    void on_audioConfButton_clicked();
    void on_audioMuteButton_clicked(bool checked);
    void pandapterRange_changed(int min, int max);
    void waterfallRange_changed(int min, int max);
    void recDir_changed(const QString &dir);
    void udpHost_changed(const QString &host);
    void udpPort_changed(int port);
    void udpStereo_changed(bool enabled);
    void squelchTriggered_changed(bool enabled);
    void recMinTime_changed(int time_ms);
    void recMaxGap_changed(int time_ms);
    void copyRecSettingsToAllVFOs_clicked();


private:
    Ui::DockAudio *ui;
    CAudioOptions *audioOptions; /*! Audio options dialog. */
    QString        rec_dir;      /*! Location for audio recordings. */
    QString        last_audio;   /*! Last audio recording. */

    QString        udp_host;     /*! UDP client host name. */
    int            udp_port;     /*! UDP client port number. */
    bool           udp_stereo;   /*! Enable stereo streaming for UDP. */
    bool           squelch_triggered; /*! Enable squelch-triggered recording */
    int            recMinTime;   /*! Minimum squelch-triggered recording time */
    int            recMaxGap;    /*! Maximum gap time in squelch-triggered mode*/

    bool           autoSpan;     /*! Whether to allow mode-dependent auto span. */

    qint64         rx_freq;      /*! RX frequency used in filenames. */

    void           recordToggleShortcut();
    void           muteToggleShortcut();
    void           increaseAudioGainShortcut();
    void           decreaseAudioGainShortcut();
};

#endif // DOCKAUDIO_H
