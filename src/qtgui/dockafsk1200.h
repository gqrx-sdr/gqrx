/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#ifndef DOCKAFSK1200_H
#define DOCKAFSK1200_H

#include <QFrame>
#include <QVarLengthArray>
#include "dsp/afsk1200/cafsk12.h"

namespace Ui {
class DockAFSK1200;
}

class DockAFSK1200 : public QFrame
{
    Q_OBJECT

public:
    explicit DockAFSK1200(QWidget *parent = nullptr);
    ~DockAFSK1200();

    void process_samples(float *buffer, int length);

signals:
    void afskDecoderToggled(bool checked);

private slots:
    void on_actionClear_triggered();
    void on_actionSave_triggered();
    void on_actionInfo_triggered();
    void on_cbEnabled_toggled(bool checked);

private:
    Ui::DockAFSK1200 *ui;

    CAfsk12                         *decoder;   /*! The AFSK1200 decoder object. */
    QVarLengthArray<float, 16384>   tmpbuf;     /*! Needed to remember "overlap" samples. */
};

#endif // DOCKAFSK1200_H
