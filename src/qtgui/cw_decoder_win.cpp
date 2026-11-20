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
#include <QCheckBox>
#include <QDebug>
#include <QFontDatabase>
#include <QLabel>
#include <QSpinBox>
#include <QTextCursor>
#include "cw_decoder_win.h"
#include "ui_cw_decoder_win.h"

#define CW_DEFAULT_WPM 20


CwDecoderWin::CwDecoderWin(unsigned int sample_rate, int tone_hz, QWidget *parent) : QMainWindow(parent),
    ui(new Ui::CwDecoderWin),
    decoder(sample_rate, tone_hz, CW_DEFAULT_WPM),
    decoding(false)
{
    ui->setupUi(this);

    /* select font for text viewer */
    ui->textView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    wpmSpinBox = new QSpinBox(this);
    wpmSpinBox->setRange(5, 60);
    wpmSpinBox->setValue(CW_DEFAULT_WPM);
    wpmSpinBox->setSuffix(tr(" WPM"));
    wpmSpinBox->setToolTip(tr("Manual Morse speed. Speeds within about 30% are decoded."));
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(wpmSpinBox);
    connect(wpmSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setWpm(int)));

    autoWpmBox = new QCheckBox(tr("Auto WPM"), this);
    autoWpmBox->setToolTip(tr("Estimate the Morse speed from the received signal"));
    ui->toolBar->addWidget(autoWpmBox);
    connect(autoWpmBox, SIGNAL(toggled(bool)), this, SLOT(setAutoWpm(bool)));

    estimateLabel = new QLabel(this);
    estimateLabel->setContentsMargins(5, 0, 5, 0);
    ui->toolBar->addWidget(estimateLabel);

    updateStatus();
}

CwDecoderWin::~CwDecoderWin()
{
    qDebug() << "CW decoder destroyed.";

    delete ui;
}


/*! \brief Process new set of demodulated audio samples. */
void CwDecoderWin::process_samples(const float *buffer, int length)
{
    if (!decoding || length <= 0)
        return;

    std::string text = decoder.process(buffer, (std::size_t) length);
    updateEstimate();
    if (text.empty())
        return;

    ui->textView->moveCursor(QTextCursor::End);
    ui->textView->insertPlainText(QString::fromStdString(text));
    ui->textView->moveCursor(QTextCursor::End);
}


/*! \brief Set the audio tone (CW offset) to decode. */
void CwDecoderWin::setTone(int tone_hz)
{
    decoder.set_tone(tone_hz);
    updateStatus();
}


/*! \brief Enable decoding (CW modes) or pause it (other modes). */
void CwDecoderWin::setDecoding(bool enabled)
{
    decoding = enabled;
    decoder.reset();
    updateStatus();
}


/*! \brief Discard any partially received character and speed evidence. */
void CwDecoderWin::resetDecoder()
{
    decoder.reset();
    updateEstimate();
}


/*! \brief Catch window close events and emit signal so that main application can destroy us. */
void CwDecoderWin::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev);

    emit windowClosed();
}


/*! \brief User clicked on the Clear button. */
void CwDecoderWin::on_actionClear_triggered()
{
    ui->textView->clear();
}


/*! \brief User changed the manual speed. */
void CwDecoderWin::setWpm(int wpm)
{
    decoder.set_wpm(wpm);
    updateStatus();
}


/*! \brief User switched between manual and estimated speed.
 *
 * The manual setting is kept in the spin box and the decoder, so switching
 * back to manual restores it.
 */
void CwDecoderWin::setAutoWpm(bool enabled)
{
    decoder.set_auto_wpm(enabled);
    wpmSpinBox->setEnabled(!enabled);
    updateStatus();
}


void CwDecoderWin::updateStatus()
{
    if (decoding)
        ui->statusBar->showMessage(tr("Decoding"));
    else
        ui->statusBar->showMessage(tr("Paused: select CW-L or CW-U mode"));

    updateEstimate();
}


void CwDecoderWin::updateEstimate()
{
    QString text;

    if (decoder.auto_wpm())
    {
        const double wpm = decoder.estimated_wpm();
        if (wpm > 0.0)
            text = tr("\u2248 %1 WPM").arg(qRound(wpm));
        else
            text = tr("Estimating\u2026");
    }

    if (estimateLabel->text() != text)
        estimateLabel->setText(text);
}
