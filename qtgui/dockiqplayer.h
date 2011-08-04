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
#ifndef DOCKIQRECORDER_H
#define DOCKIQRECORDER_H

#include <QDockWidget>

namespace Ui {
    class DockIqRecorder;
}


/*! \brief I/Q recorder and playback widget. */
class DockIqRecorder : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockIqRecorder(QWidget *parent = 0);
    ~DockIqRecorder();

private slots:
    void on_iqRecButton_clicked(bool checked);
    void on_iqPlayButton_clicked(bool checked);

private:  
    Ui::DockIqRecorder *ui;

    /** FIXME: use state? **/

};

#endif // DOCKIQRECORDER_H
