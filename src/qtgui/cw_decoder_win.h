/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2026 Lukas Kohlfürst.
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
#ifndef CW_DECODER_WIN_H
#define CW_DECODER_WIN_H

#include <QMainWindow>
#include "dsp/cw/cw_decoder.h"

class QCheckBox;
class QLabel;
class QSpinBox;

namespace Ui
{
    class CwDecoderWin;
}


/*! \brief CW (Morse code) decoder window. */
class CwDecoderWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit CwDecoderWin(unsigned int sample_rate, QWidget *parent = nullptr);
    ~CwDecoderWin();

    void process_samples(const float *buffer, int length);

    void restart(int tone_hz, bool enabled);

protected:
    void closeEvent(QCloseEvent *ev);

signals:
    void windowClosed(); /*! Signal we emit when window is closed. */

private slots:
    void on_actionClear_triggered();
    void setWpm(int wpm);
    void setAutoWpm(bool enabled);

private:
    void updateStatus();
    void updateEstimate();

    Ui::CwDecoderWin *ui; /*! Qt Designer form. */
    QSpinBox *wpmSpinBox; /*! Manual Morse speed. */
    QCheckBox *autoWpmBox; /*! Use the estimated speed. */
    QLabel *estimateLabel; /*! Estimated speed in automatic mode. */
    CwDecoder decoder; /*! The CW decoder. */
    bool decoding; /*! False when the receiver is not in a CW mode. */
};

#endif // CW_DECODER_WIN_H
