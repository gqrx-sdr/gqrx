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
#ifndef DOCKIQPLAYER_H
#define DOCKIQPLAYER_H

#include <QDockWidget>

namespace Ui {
    class DockIqPlayer;
}


/*! \brief I/Q playback widget. */
class DockIqPlayer : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockIqPlayer(QWidget *parent = 0);
    ~DockIqPlayer();

private slots:
    void on_openButton_clicked();
    void on_playButton_clicked(bool checked);

private:  
    Ui::DockIqPlayer *ui;

    /** FIXME: use state? **/

};

#endif // DOCKIQPLAYER_H
