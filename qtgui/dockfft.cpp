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
#include <QDebug>
#include "dockfft.h"
#include "ui_dockfft.h"

DockFft::DockFft(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DockFft)
{
    ui->setupUi(this);
}

DockFft::~DockFft()
{
    delete ui;
}


/*! \brief FFT size changed. */
void DockFft::on_fftSizeComboBox_currentIndexChanged(const QString &text)
{
    int value = text.toInt();
    emit fftSizeChanged(value);
}

/*! \brief Lower limit of Y-axis changed. */
void DockFft::on_fftMinComboBox_currentIndexChanged(const QString &text)
{
    int value = text.toInt();
    emit fftYminChanged(value);
}

/*! \brief Upper limit of Y-axis changed. */
void DockFft::on_fftMaxComboBox_currentIndexChanged(const QString &text)
{
    int value = text.toInt();
    emit fftYmaxChanged(value);
}

/*! \brief Split between waterfall and pandapter changed.
 *  \param value The percentage of the waterfall.
 */
void DockFft::on_fftSplitSlider_valueChanged(int value)
{
    emit fftSplitChanged(value);
}
