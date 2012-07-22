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


/*! \brief Get current FFT rate setting.
 *  \return The current FFT rate in frames per second (always non-zero)
 */
int DockFft::fftRate()
{
    bool ok;
    int fps = 10;
    QString strval = ui->fftRateComboBox->currentText();

    strval.remove(" fps");
    fps = strval.toInt(&ok, 10);

    if (!ok) {
        qDebug() << "DockFft::fftRate : Could not convert" <<
                    strval << "to number.";
    }

    if (fps == 0) {
        qDebug() << "Ups... somehow we ended up with FFT rate = 0 (using 10)";
        fps = 10;
    }

    return fps;
}

/*! \brief FFT size changed. */
void DockFft::on_fftSizeComboBox_currentIndexChanged(const QString &text)
{
    int value = text.toInt();
    emit fftSizeChanged(value);
}

/*! \brief FFT rate changed. */
void DockFft::on_fftRateComboBox_currentIndexChanged(const QString & text)
{
    int fps = fftRate();
    Q_UNUSED(text);

    emit fftRateChanged(fps);
}

/*! \brief Split between waterfall and pandapter changed.
 *  \param value The percentage of the waterfall.
 */
void DockFft::on_fftSplitSlider_valueChanged(int value)
{
    emit fftSplitChanged(value);
}

void DockFft::on_fftZoomSlider_valueChanged(int value)
{
    emit fftZoomChanged(value);
}
