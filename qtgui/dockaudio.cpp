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
#include "dockaudio.h"
#include "ui_dockaudio.h"

DockAudio::DockAudio(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockAudio)
{
    ui->setupUi(this);
}

DockAudio::~DockAudio()
{
    delete ui;
}


/*! \brief Set new audio gain.
 *  \param gain the new audio gain in dB (0 dB = 1.0)
 */
void DockAudio::setAudioGain(int gain)
{
    ui->audioGainSlider->setValue(gain);
}


/*! \brief Get current audio gain.
 *  \returns The current audio gain in dB (0 dB = 1.0).
 */
int  DockAudio::audioGain()
{
    return ui->audioGainSlider->value();
}


/*! \brief Audio gain changed.
 *  \param value The new audio gain value in dB (0 dB = 1.0)
 */
void DockAudio::on_audioGainSlider_valueChanged(int value)
{
    emit audioGainChanged(value);
}
