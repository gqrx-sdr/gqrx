/* -*- c++ -*- */
/*
 * Copyright 2011 Alexandru Csete OZ9AEC.
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

#include <QDockWidget>

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

    void setAudioGain(int gain);
    int  audioGain();

signals:
    /*! \brief Signal emitted when audio gain has changed. Gain is in dB. */
    void audioGainChanged(float gain);

    /*! \brief Signal emitted when audio recording is started. */
    void audioRecordingStarted(const QString filename);

    /*! \brief Signal emitted when audio recording is stopped. */
    void audioRecordingStopped();


private slots:
    void on_audioGainSlider_valueChanged(int value);


private:
    Ui::DockAudio *ui;
};

#endif // DOCKAUDIO_H
