/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           http://gqrx.dk/
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
#ifndef IQ_TOOL_H
#define IQ_TOOL_H

#include <QCloseEvent>
#include <QDialog>
#include <QString>

namespace Ui {
    class CIqTool;
}

/*! \brief User interface for I/Q recording and playback. */
class CIqTool : public QDialog
{
    Q_OBJECT
    
public:
    explicit CIqTool(QWidget *parent = 0);
    ~CIqTool();
    
    void closeEvent(QCloseEvent *event);

signals:
    void start_recording(const QString filename);
    void stop_recording();

public slots:
    void cancelRecording();

private slots:
    void on_recButton_clicked(bool checked);


private:
    Ui::CIqTool *ui;

    QString recfile;

    bool is_recording;
    bool is_playing;
    int  bytes_per_sample;
    int  sample_rate;
};

#endif // IQ_TOOL_H
