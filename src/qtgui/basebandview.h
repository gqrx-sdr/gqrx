/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2021 Doug Hammond.
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
#ifndef BASEBANDVIEW_H
#define BASEBANDVIEW_H

#include <QWidget>

#include "qtgui/freqctrl.h"
#include "qtgui/meter.h"
#include "qtgui/plotter.h"

namespace Ui {
class BasebandView;
}

class BasebandView : public QWidget
{
    Q_OBJECT

public:
    explicit BasebandView(QWidget *parent = nullptr);
    ~BasebandView();

    CFreqCtrl* freqCtrl();
    CMeter* sMeter();
    CPlotter* plotter();

private:
    Ui::BasebandView *ui;
};

#endif // BASEBANDVIEW_H
