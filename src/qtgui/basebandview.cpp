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

#include "basebandview.h"
#include "ui_basebandview.h"

BasebandView::BasebandView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BasebandView)
{
    ui->setupUi(this);
}

BasebandView::~BasebandView()
{
    delete ui;
}

CFreqCtrl* BasebandView::freqCtrl() {
    return ui->freqCtrl;
}

CPlotter* BasebandView::plotter() {
    return ui->plotter;
}
