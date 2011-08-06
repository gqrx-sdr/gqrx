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
#include <QFileDialog>
#include <QTime>
#include <QDebug>
#include "dockiqplayer.h"
#include "ui_dockiqplayer.h"



DockIqPlayer::DockIqPlayer(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockIqPlayer),
    d_samprate(96000),
    d_bps(8),     /* sizeof(gr_complex) */
    d_pos(0)
{
    ui->setupUi(this);
}

DockIqPlayer::~DockIqPlayer()
{
    delete ui;
}


/*! \brief Open button clicked. Select a new file. */
void DockIqPlayer::on_openButton_clicked()
{
    QString newFile;
    qDebug() << "Open button clicked.";

    newFile = QFileDialog::getOpenFileName(this, tr("Open I/Q recording"), "", tr("I/Q recordings (*.bin *.raw)"));

    if (newFile.isEmpty()) {
        /* user cancelled */
        return;
    }

    /* store new file name */
    d_fileName = newFile;

    /* estimate duration */
    QFile f(d_fileName);
    QTime zero(0, 0, 0, 0);
    QTime dur;

    d_duration = f.size() / (d_samprate*d_bps);
    ui->seekSlider->setRange(0, (int)d_duration);

    /* show duration */
    dur = zero.addSecs((int)d_duration);
    ui->durLabel->setText(dur.toString("HH:mm:ss"));

    /* update widgets */
    setWindowTitle(tr("I/Q Player: %1").arg(d_fileName));
    ui->playButton->setEnabled(true);


    emit fileOpened(d_fileName);
}


/*! \brief Play button clicked. Start or stop playback, depending on button state. */
void DockIqPlayer::on_playButton_clicked(bool checked)
{
    if (checked) {
        qDebug() << "Start playback";
        ui->openButton->setEnabled(false);
        ui->seekSlider->setEnabled(true);
    }
    else {
        qDebug() << "Stop playback";
        ui->openButton->setEnabled(true);
        ui->seekSlider->setEnabled(false);
    }

    emit playbackToggled(checked, d_fileName);
}


/*! \brief Seek slider position changed.
 *  \param new_pos The new position in secionds.
 *
 * This slot is triggered when the seek slider position changes. This can
 * either be due to user moving the slider or a new position set via setPos().
 * To prevent the latter from triggering emission of posChanged() the new
 * position is compared to the last one known. If they are equal, the position
 * was set using setPos(). If they are different the position change was a
 * result of user action.
 */
void DockIqPlayer::on_seekSlider_valueChanged(int new_pos)
{
    if (new_pos != d_pos) {
        /* This was a user action */
        d_pos = new_pos;
        emit posChanged(new_pos);
    }

    /* we need to update label in any case */
    QTime zero(0, 0, 0, 0);
    QTime t;

    t = zero.addSecs(d_pos);
    ui->posLabel->setText(t.toString("HH:mm:ss"));
}


/*! \brief Set new playback position.
 *  \param pos The new position in seconds.
 */
void DockIqPlayer::setPos(int pos)
{
    if (pos != d_pos) {
        d_pos = pos;
        ui->seekSlider->setValue(pos);
    }
}
